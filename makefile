all: ds peer

peer: peer.o
	gcc -g -Wall peer.o -o peer 

ds: ds.o
	gcc -g -Wall ds.o -o ds

clean:
	rm *o ds peer

	
