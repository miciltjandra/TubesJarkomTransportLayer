all: receiver transmitter

receiver: receiver.o
	g++ receiver.o -o bin/receiver -lpthread

receiver.o: src/receiver.cpp
	g++ -c src/receiver.cpp -std=c++0x

clean:
	rm *o bin/*

transmitter: transmitter.o
	g++ transmitter.o -o bin/transmitter -lpthread

transmitter.o: src/transmitter.cpp
	g++ -c src/transmitter.cpp -std=c++0x
