all: ftp.o download

ftp.o: ftp.c
	gcc -c ftp.c

download: ftp.h ftp.o download.c
	gcc -o download ftp.o download.c

splint:
	splint ftp.c download.c
	
run:
	download
	
clean:
	rm -f download *.o
