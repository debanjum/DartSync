all: tracker/tracker etags

common/pkt.o: common/pkt.c common/pkt.h common/constant.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
peer/monitor.o: peer/monitor.c 
	gcc -Wall -pedantic -std=c99 -g -c peer/monitor.c -o peer/monitor.o
peer/peer: peer/peer.c peer/monitor.o common/pkt.o
	gcc -Wall -pedantic -std=c99 -g -pthread peer/peer.c peer/monitor.o common/pkt.o -o peer/peer
tracker/tracker: tracker/tracker.c common/pkt.o
	gcc -Wall -pedantic -std=c99 -g -pthread tracker/tracker.c common/pkt.o -o tracker/tracker
etags:
	find . -name '*.[ch]' | xargs etags
clean:
	rm -rf common/*.o
	rm -rf peer/*.o
	rm -rf tracker/*.o
	rm -rf peer/peer
	rm -rf tracker/tracker
