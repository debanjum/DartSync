all: peer/peer tracker/tracker etags

common/pkt.o: common/pkt.c common/pkt.h common/constant.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
peer/datacompress.o: peer/datacompress.c peer/datacompress.h
	gcc -Wall -pedantic -std=c99 -Llib -Ilib -g -c peer/datacompress.c -o peer/datacompress.o -lz
peer/monitor.o: peer/monitor.c 
	gcc -Wall -pedantic -std=c99 -g -c peer/monitor.c -o peer/monitor.o
peer/p2p.o: peer/p2p.c peer/p2p.h
	gcc -Wall -pedantic -std=c99 -g -c peer/p2p.c -o peer/p2p.o
peer/peer: peer/peer.c common/pkt.o peer/p2p.o peer/monitor.o peer/datacompress.o
	gcc -Wall -pedantic -std=c99 -g -pthread peer/peer.c common/pkt.o peer/monitor.o peer/p2p.o peer/datacompress.o -o peer/peer -Llib -Ilib -lz
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
