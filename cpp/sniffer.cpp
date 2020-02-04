#include <stdio.h>
#include <pcap.h>
#include <string>

static int pktCount = 0;


// do somthing with the packet
void getHTTPRequest(u_char *data, const struct pcap_pkthdr* header, const u_char* packet)
{
	//print out when a packet is received for now
	std::string packetS(reinterpret_cast<const char*>(packet));
	
	printf("%s\n", packetS);
}



int main(int argc, char *argv[])
{
	pcap_t *handle;			// pcap session handle
	char *dev;			// the device
	char errbuf[PCAP_ERRBUF_SIZE];	// error string
	struct pcap_pkthdr header;	// the packet header
	const u_char *packet;		// the packet
	char filter[] = "port 80";      // the filter
	struct bpf_program fp;		// compiled filter expression
	bpf_u_int32 mask;		// the netmask of the sniffing device
	bpf_u_int32 net;		// the ip of sniffing device

	// assign the device
	dev = pcap_lookupdev(errbuf);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return(2);
	}
	
	//get the netmask
	pcap_lookupnet(dev, &net, &mask, errbuf);

	// open the packet capture session
	handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return(2);
	}

	// compile the filter
	pcap_compile(handle, &fp, filter, 0, net);

	// set the filter
	pcap_setfilter(handle, &fp);
	
	printf("listening on %s\n", filter);
	//loop 10 times using the above function
	pcap_loop(handle, 20, getHTTPRequest, NULL);	

	
	// close
	pcap_close(handle);
	return(0);
}
