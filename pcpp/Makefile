include /home/ncvncv97/pcapplusplus-19.12-ubuntu-18.04-gcc-7/mk/PcapPlusPlus.mk

# All Target
all:
	g++ $(PCAPPP_INCLUDES) -c -o main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -o HTTPApp main.o $(PCAPPP_LIBS)

# Clean Target
clean:
	rm main.o
	rm HTTPApp
