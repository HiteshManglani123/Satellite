all: satellite ground-station

satellite: src/satellite/satellite.c build/tftp.o
	gcc src/satellite/satellite.c -o build/satellite

ground-station: build/ground-station.o build/tftp.o
	gcc build/ground-station.o build/tftp.o -o build/ground-station

build/ground-station.o: src/ground-station/ground-station.c
	gcc -c src/ground-station/ground-station.c -o build/ground-station.o

build/tftp.o: src/tftp.c
	gcc -c src/tftp.c -o build/tftp.o

clean:
	rm -rf ./build/*
