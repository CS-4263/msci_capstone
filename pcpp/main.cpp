#include "stdlib.h"
#include "header/Packet.h"
#include "header/EthLayer.h"
#include "header/IPv4Layer.h"
#include "header/TcpLayer.h"
#include "header/HttpLayer.h"
#include "header/PcapFileDevice.h"
#include "header/PcapLiveDeviceList.h"
#include "header/PlatformSpecificUtils.h"

static void packetCallback(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
	printf("packet captured");


}


int main(int argc, char* argv[])
{
	std::string devIP = "10.128.0.3";

	pcpp::PcapLiveDevice* dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(devIP.c_str());
	if(dev == NULL)
	{
		printf("dev is null\n");
		exit(1);
	}


	printf("Interface name: %s\n", dev->getName());

	if(!dev->open())
	{
		printf("cannot open device\n");
		exit(1);
	}

	dev->startCapture(packetCallback, NULL);

	PCAP_SLEEP(10);

	dev->stopCapture();

}
