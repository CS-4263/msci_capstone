#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "header/TcpReassembly.h"
#include "header/PcapLiveDeviceList.h"
#include "header/PcapFileDevice.h"
#include "header/PlatformSpecificUtils.h"
#include "header/SystemUtils.h"
#include "header/PcapPlusPlusVersion.h"
#include "header/LRUList.h"
#include <getopt.h>

using namespace pcpp;


// unless the user chooses otherwise - default number of concurrent used file descriptors is 500
#define DEFAULT_MAX_NUMBER_OF_CONCURRENT_OPEN_FILES 500


/**
 * This class contains all the flags indicated by the user
 */
class GlobalConfig
{
private:

	/**
	 * A private constructor
	 */
	GlobalConfig() { outputDir = ""; writeToConsole = false; separateSides = false; maxOpenFiles = DEFAULT_MAX_NUMBER_OF_CONCURRENT_OPEN_FILES; m_RecentConnsWithActivity = NULL; }

	// A least-recently-used (LRU) list of all connections seen so far. Each connection is represented by its flow key. This LRU list is used to decide which connection was seen least
	// recently in case we reached max number of open file descriptors and we need to decide which files to close
	LRUList<uint32_t>* m_RecentConnsWithActivity;

public:

	// the directory to write files to (default is current directory)
	std::string outputDir;

	// a flag indicating whether to write TCP data to actual files or to console
	bool writeToConsole;

	// a flag indicating whether to write both side of a connection to the same file (which is the default) or write each side to a separate file
	bool separateSides;

	// max number of allowed open files in each point in time
	size_t maxOpenFiles;


	/**
	 * A method getting connection parameters as input and returns a filename and file path as output.
	 * The filename is constructed by the IPs (src and dst) and the TCP ports (src and dst)
	 */
	std::string getFileName(ConnectionData connData, int side, bool separareSides)
	{
		std::stringstream stream;

		// if user chooses to write to a directory other than the current directory - add the dir path to the return value
		if (outputDir != "")
			stream << outputDir << '/';

		std::string sourceIP = connData.srcIP->toString();
		std::string destIP = connData.dstIP->toString();

		// for IPv6 addresses, replace ':' with '_'
		std::replace(sourceIP.begin(), sourceIP.end(), ':', '_');
		std::replace(destIP.begin(), destIP.end(), ':', '_');

		// side == 0 means data is sent from client->server
		if (side <= 0 || separareSides == false)
			stream << sourceIP << "." << connData.srcPort << "-" << destIP << "." << connData.dstPort;
		else // side == 1 means data is sent from server->client
			stream << destIP << "." << connData.dstPort << "-" << sourceIP << "." << connData.srcPort;

		// return the file path
		return stream.str();
	}


	/**
	 * Open a file stream. Inputs are the filename to open and a flag indicating whether to append to an existing file or overwrite it.
	 * Return value is a pointer to the new file stream
	 */
	std::ostream* openFileStream(std::string fileName, bool reopen)
	{
		// if the user chooses to write only to console, don't open anything and return std::cout
		if (writeToConsole)
			return &std::cout;

		// open the file on the disk (with append or overwrite mode)
		if (reopen)
			return new std::ofstream(fileName.c_str(), std::ios_base::binary | std::ios_base::app);
		else
			return new std::ofstream(fileName.c_str(), std::ios_base::binary);
	}


	/**
	 * Close a file stream
	 */
	void closeFileSteam(std::ostream* fileStream)
	{
		// if the user chooses to write only to console - do nothing and return
		if (!writeToConsole)
		{
			// close the file stream
			std::ofstream* fstream = (std::ofstream*)fileStream;
			fstream->close();

			// free the memory of the file stream
			delete fstream;
		}
	}


	/**
	 * Return a pointer to the least-recently-used (LRU) list of connections
	 */
	LRUList<uint32_t>* getRecentConnsWithActivity()
	{
		if (m_RecentConnsWithActivity == NULL)
			m_RecentConnsWithActivity = new LRUList<uint32_t>(maxOpenFiles);

		// return the pointer
		return m_RecentConnsWithActivity;
	}


	/**
	 * The singleton implementation of this class
	 */
	static GlobalConfig& getInstance()
	{
		static GlobalConfig instance;
		return instance;
	}
	
	/**
	 * destructor
	 */
	~GlobalConfig()
	{
		delete m_RecentConnsWithActivity;
	}
};


/**
 * A struct to contain all data save on a specific connection. It contains the file streams to write to and also stats data on the connection
 */
struct TcpReassemblyData
{
	// pointer to 2 file stream - one for each side of the connection. If the user chooses to write both sides to the same file (which is the default), only one file stream is used (index 0)
	std::ostream* fileStreams[2];

	// flags indicating whether the file in each side was already opened before. If the answer is yes, next time it'll be opened in append mode (and not in overwrite mode)
	bool reopenFileStreams[2];

	// a flag indicating on which side was the latest message on this connection
	int curSide;

	// stats data: num of data packets on each side, bytes seen on each side and messages seen on each side
	int numOfDataPackets[2];
	int numOfMessagesFromSide[2];
	int bytesFromSide[2];

	/**
	 * the default constructor
	 */
	TcpReassemblyData() { fileStreams[0] = NULL; fileStreams[1] = NULL; clear(); }

	/**
	 * destructor
	 */
	~TcpReassemblyData()
	{
		// close files on both sides if open
		if (fileStreams[0] != NULL)
			GlobalConfig::getInstance().closeFileSteam(fileStreams[0]);

		if (fileStreams[1] != NULL)
			GlobalConfig::getInstance().closeFileSteam(fileStreams[1]);
	}

	/**
	 * Clear all data (put 0, false or NULL - whatever relevant for each field)
	 */
	void clear()
	{
		// for the file stream - close them if they're not null
		if (fileStreams[0] != NULL)
		{
			GlobalConfig::getInstance().closeFileSteam(fileStreams[0]);
			fileStreams[0] = NULL;
		}

		if (fileStreams[1] != NULL)
		{
			GlobalConfig::getInstance().closeFileSteam(fileStreams[1]);
			fileStreams[1] = NULL;
		}

		reopenFileStreams[0] = false;
		reopenFileStreams[1] = false;
		numOfDataPackets[0] = 0;
		numOfDataPackets[1] = 0;
		numOfMessagesFromSide[0] = 0;
		numOfMessagesFromSide[1] = 0;
		bytesFromSide[0] = 0;
		bytesFromSide[1] = 0;
		curSide = -1;
	}
};


// typedef representing the connection manager and its iterator
typedef std::map<uint32_t, TcpReassemblyData> TcpReassemblyConnMgr;
typedef std::map<uint32_t, TcpReassemblyData>::iterator TcpReassemblyConnMgrIter;


/**
 * The callback being called by the TCP reassembly module whenever new data arrives on a certain connection
 */
static void tcpReassemblyMsgReadyCallback(int sideIndex, const TcpStreamData& tcpData, void* userCookie)
{
	// extract the connection manager from the user cookie
	TcpReassemblyConnMgr* connMgr = (TcpReassemblyConnMgr*)userCookie;

	// check if this flow already appears in the connection manager. If not add it
	TcpReassemblyConnMgrIter iter = connMgr->find(tcpData.getConnectionData().flowKey);
	if (iter == connMgr->end())
	{
		connMgr->insert(std::make_pair(tcpData.getConnectionData().flowKey, TcpReassemblyData()));
		iter = connMgr->find(tcpData.getConnectionData().flowKey);
	}

	int side;

	// if the user wants to write each side in a different file - set side as the sideIndex, otherwise write everything to the same file ("side 0")
	if (GlobalConfig::getInstance().separateSides)
		side = sideIndex;
	else
		side = 0;

	// if the file stream on the relevant side isn't open yet (meaning it's the first data on this connection)
	if (iter->second.fileStreams[side] == NULL)
	{
		// add the flow key of this connection to the list of open connections. If the return value isn't NULL it means that there are too many open files
		// and we need to close the connection with least recently used file(s) in order to open a new one.
		// The connection with the least recently used file is the return value
		uint32_t flowKeyToCloseFiles;
		int result = GlobalConfig::getInstance().getRecentConnsWithActivity()->put(tcpData.getConnectionData().flowKey, &flowKeyToCloseFiles);

		// if result equals to 1 it means we need to close the open files in this connection (the one with the least recently used files)
		if (result == 1)
		{
			// find the connection from the flow key
			TcpReassemblyConnMgrIter iter2 = connMgr->find(flowKeyToCloseFiles);
			if (iter2 != connMgr->end())
			{
				// close files on both sides (if they're open)
				for (int index = 0; index < 2; index++)
				{
					if (iter2->second.fileStreams[index] != NULL)
					{
						// close the file
						GlobalConfig::getInstance().closeFileSteam(iter2->second.fileStreams[index]);
						iter2->second.fileStreams[index] = NULL;

						// set the reopen flag to true to indicate that next time this file will be opened it will be opened in append mode (and not overwrite mode)
						iter2->second.reopenFileStreams[index] = true;
					}
				}
			}
		}

		// get the file name according to the 5-tuple etc.
		std::string fileName = GlobalConfig::getInstance().getFileName(tcpData.getConnectionData(), sideIndex, GlobalConfig::getInstance().separateSides) + ".txt";

		// open the file in overwrite mode (if this is the first time the file is opened) or in append mode (if it was already opened before)
		iter->second.fileStreams[side] = GlobalConfig::getInstance().openFileStream(fileName, iter->second.reopenFileStreams[side]);
	}

	// if this messages comes on a different side than previous message seen on this connection
	if (sideIndex != iter->second.curSide)
	{
		// count number of message in each side
		iter->second.numOfMessagesFromSide[sideIndex]++;

		// set side index as the current active side
		iter->second.curSide = sideIndex;
	}

	// count number of packets and bytes in each side of the connection
	iter->second.numOfDataPackets[sideIndex]++;
	iter->second.bytesFromSide[sideIndex] += (int)tcpData.getDataLength();

	// write the new data to the file
	iter->second.fileStreams[side]->write((char*)tcpData.getData(), tcpData.getDataLength());
}


/**
 * The callback being called by the TCP reassembly module whenever a new connection is found. This method adds the connection to the connection manager
 */
static void tcpReassemblyConnectionStartCallback(const ConnectionData& connectionData, void* userCookie)
{
	// get a pointer to the connection manager
	TcpReassemblyConnMgr* connMgr = (TcpReassemblyConnMgr*)userCookie;

	// look for the connection in the connection manager
	TcpReassemblyConnMgrIter iter = connMgr->find(connectionData.flowKey);

	// assuming it's a new connection
	if (iter == connMgr->end())
	{
		// add it to the connection manager
		connMgr->insert(std::make_pair(connectionData.flowKey, TcpReassemblyData()));
	}
}


/**
 * The callback being called by the TCP reassembly module whenever a connection is ending. This method removes the connection from the connection manager and writes the metadata file if requested
 * by the user
 */
static void tcpReassemblyConnectionEndCallback(const ConnectionData& connectionData, TcpReassembly::ConnectionEndReason reason, void* userCookie)
{
	// get a pointer to the connection manager
	TcpReassemblyConnMgr* connMgr = (TcpReassemblyConnMgr*)userCookie;

	// find the connection in the connection manager by the flow key
	TcpReassemblyConnMgrIter iter = connMgr->find(connectionData.flowKey);

	// connection wasn't found - shouldn't get here
	if (iter == connMgr->end())
		return;

	// remove the connection from the connection manager
	connMgr->erase(iter);
}


/**
 * The callback to be called when application is terminated by ctrl-c. Stops the endless while loop
 */
static void onApplicationInterrupted(void* cookie)
{
	bool* shouldStop = (bool*)cookie;
	*shouldStop = true;
}


/**
 * packet capture callback - called whenever a packet arrives on the live device (in live device capturing mode)
 */
static void onPacketArrives(RawPacket* packet, PcapLiveDevice* dev, void* tcpReassemblyCookie)
{
	// get a pointer to the TCP reassembly instance and feed the packet arrived to it
	TcpReassembly* tcpReassembly = (TcpReassembly*)tcpReassemblyCookie;
	tcpReassembly->reassemblePacket(packet);
}


/**
 * The method responsible for TCP reassembly on live traffic
 */
void liveTcpReassembly(PcapLiveDevice* dev, TcpReassembly& tcpReassembly)
{

	printf("Starting packet capture\n");

	// start capturing packets. Each packet arrived will be handled by onPacketArrives method
	dev->startCapture(onPacketArrives, &tcpReassembly);

	// register the on app close event to print summary stats on app termination
	bool shouldStop = false;
	ApplicationEventHandler::getInstance().onApplicationInterrupted(onApplicationInterrupted, &shouldStop);

	// run in an endless loop until the user presses ctrl+c
	while(!shouldStop)
		PCAP_SLEEP(1);

	// stop capturing and close the live device
	dev->stopCapture();
	dev->close();

	// close all connections which are still opened
	tcpReassembly.closeAllConnections();

	printf("Finished capture\n");
}


/**
 * main method of this utility
 */
int main(int argc, char* argv[])
{
	
	//IMPORTANT: Change this to your own IP
	std::string devIP = "10.128.0.3";

	//initialize device
	pcpp::PcapLiveDevice* dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(devIP.c_str());
	
	//check to see if the device initialized correctly
	if(dev == NULL)
	{
		printf("dev is null\n");
		exit(1);
	}

	//print dev name
	printf("Interface name: %s\n", dev->getName());

	//attempt to open the device
	if(!dev->open())
	{
		printf("cannot open device\n");
		exit(1);
	}

	//create a port filter because we want to filter by port 80
	pcpp::PortFilter portFilter(80, pcpp::SRC_OR_DST);
	
	//create the 'ANDFilter'
	pcpp::AndFilter filter;
	//add the port filter to the ANDFilter
	filter.addFilter(&portFilter);

	//set the filter on the device to the filter we just created
	dev->setFilter(filter);

	std::string inputPcapFileName = "";
	std::string outputDir = "";
	bool writeToConsole = true;
	bool separateSides = false;
	size_t maxOpenFiles = DEFAULT_MAX_NUMBER_OF_CONCURRENT_OPEN_FILES;
	
	// set global config singleton with input configuration
	GlobalConfig::getInstance().outputDir = outputDir;
	GlobalConfig::getInstance().writeToConsole = writeToConsole;
	GlobalConfig::getInstance().separateSides = separateSides;
	GlobalConfig::getInstance().maxOpenFiles = maxOpenFiles;

	// create the object which manages info on all connections
	TcpReassemblyConnMgr connMgr;

	// create the TCP reassembly instance
	TcpReassembly tcpReassembly(tcpReassemblyMsgReadyCallback, &connMgr, tcpReassemblyConnectionStartCallback, tcpReassemblyConnectionEndCallback);

	// start capturing packets and do TCP reassembly
	liveTcpReassembly(dev, tcpReassembly);	
}
