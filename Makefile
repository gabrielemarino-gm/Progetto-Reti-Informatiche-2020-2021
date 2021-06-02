all: ds peer

ds: ds.o 
	gcc -Wall ds.o -o ds

peer: peer.o 
	gcc -Wall peer.o -o peer

clean:
	rm *o ds peer
