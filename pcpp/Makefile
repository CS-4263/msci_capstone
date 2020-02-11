ADRIENNE = /home/adrienne_elise_p/pcapplusplus-19.12-source-linux/PcapPlusPlus/mk/PcapPlusPlus.mk
NATHAN = /home/ncvncv97/pcapplusplus-19.12-ubuntu-18.04-gcc-7/mk/PcapPlusPlus.mk
JAMES = /home/james/PcapPP/pcapplusplus-19.12-ubuntu-16.04-gcc-5.4.0/mk/PcapPlusPlus.mk

include ${ADRIENNE}

# All Target
all:
	g++ $(PCAPPP_INCLUDES) -c -o main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -o HTTPApp main.o $(PCAPPP_LIBS)

# Clean Target
clean:
	rm main.o
	rm HTTPApp
