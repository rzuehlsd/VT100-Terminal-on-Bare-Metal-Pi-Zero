all: buildfont font8x8.bin TRSfont8x24.bin TRSfont8x16.bin VT100-10x20.bin

buildfont: buildfont.cpp CImg.h
	g++ buildfont.cpp -o buildfont

font8x8.bin: Cbios_8x8_8x32.png buildfont
	./buildfont -i Cbios_8x8_8x32.png -o font8x8.bin -w 8 -h 8 -c 0	-q

TRSfont8x24.bin: trs80-8x24.png buildfont
	./buildfont -i trs80-8x24.png -o TRSfont8x24.bin -w 8 -h 24 -c 1 -q
	
TRSfont8x16.bin: trs80-8x16.png buildfont
	./buildfont -i trs80-8x16.png -o TRSfont8x16.bin -w 8 -h 16 -c 1 -q
	

VT100-10x20.bin: VT100-10x20.png buildfont
	./buildfont -i VT100-10x20.png -o VT100-10x20.bin -w 10 -h 20 -c 1 -q
