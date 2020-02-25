#include "stdlib.h"
#include "header/Packet.h"
#include "header/EthLayer.h"
#include "header/IPv4Layer.h"
#include "header/TcpLayer.h"
#include "header/HttpLayer.h"
#include "header/PcapFileDevice.h"
#include "header/PcapLiveDeviceList.h"
#include "header/PlatformSpecificUtils.h"
#include <iostream>

/*
* This method returns the Http method as a string
*/
std::string printHttpMethod(pcpp::HttpRequestLayer::HttpMethod httpMethod)
{
    switch (httpMethod)
    {
    case pcpp::HttpRequestLayer::HttpGET:
        return "GET";
    case pcpp::HttpRequestLayer::HttpPOST:
        return "POST";
    case pcpp::HttpRequestLayer::HttpHEAD:
	return "HEAD";
    case pcpp::HttpRequestLayer::HttpPUT:
	return "PUT";
    case pcpp::HttpRequestLayer::HttpDELETE:
	return "DELETE";
    case pcpp::HttpRequestLayer::HttpTRACE:
	return "TRACE";
    case pcpp::HttpRequestLayer::HttpOPTIONS:
	return "OPTIONS";
    case pcpp::HttpRequestLayer::HttpCONNECT:
	return "CONNECT";
    case pcpp::HttpRequestLayer::HttpPATCH:
	return "PATCH";
    case pcpp::HttpRequestLayer::HttpMethodUnknown:
    default:
        return "HTTP METHOD UNKNOWN";
    }
}

/*
* This is where all the packet parsing will be done
* Most of the work to be done is in this function
*/
static void packetCallback(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
	//initialize the packet from the raw packet
	pcpp::Packet parsedPacket(packet);
	
	//attempt to get the http layer from the current packet 
	pcpp::HttpRequestLayer* httpLayer = parsedPacket.getLayerOfType<pcpp::HttpRequestLayer>();
	//if the http layer exists then print out all the http info
	if(httpLayer != NULL)
	{
		printf("##################################\n");
		printf("Layer data: %d [bytes]; Layer payload: %d [bytes]\n", 
				(int)httpLayer->getDataLen(), 
				(int)httpLayer->getLayerPayloadSize());
		printf("HTTP method: %s\n", printHttpMethod(httpLayer->getFirstLine()->getMethod()).c_str());
		printf("HTTP URI: %s\n", httpLayer->getFirstLine()->getUri().c_str());
		printf("HTTP host: %s\n", httpLayer->getFieldByName(PCPP_HTTP_HOST_FIELD)->getFieldValue().c_str());
		printf("HTTP user-agent: %s\n", httpLayer->getFieldByName(PCPP_HTTP_USER_AGENT_FIELD)->getFieldValue().c_str());
		printf("HTTP full URL: %s\n", httpLayer->getUrl().c_str());
		printf("##################################\n");

		// now we are going to store the method
		std::string str = "curl -X POST \"localhost:9200/packet/_doc?pretty\" -H 'Content-Type: application/json' -d'\n{\n\"method\" : \"";
		str = str + printHttpMethod(httpLayer->getFirstLine()->getMethod()).c_str() + "\"\n}\n'";
		const char *command = str.c_str();
		//sending it to the system
		system(command);
	}

	
}

/*
* This is the main where the device will be initialized
* as well as the filter set and capturing will begin.
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
	
	//start capture on the device with the packetCallback function
	dev->startCapture(packetCallback, NULL);

	//The main will continue running so make it sleep
	//while we parse packets in packetCallback
	PCAP_SLEEP(10);
	
	//finally stop the capture
	dev->stopCapture();

}
