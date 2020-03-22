include /home/ncvncv97/pcapplusplus-19.12-ubuntu-18.04-gcc-7/mk/PcapPlusPlus.mk

# All Target
all:
	g++ $(PCAPPP_INCLUDES) -c -o main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -o HTTPEcho main.o $(PCAPPP_LIBS)
	g++ $(PCAPPP_INCLUDES) -c -o main_old.o main_old.cpp
	g++ $(PCAPPP_LIBS_DIR) -o HTTPEcho_old main_old.cpp $(PCAPPP_LIBS)

# Clean Target
clean:
	rm main.o
	rm HTTPEcho
	rm main_old.o
	rm HTTPEcho_old
