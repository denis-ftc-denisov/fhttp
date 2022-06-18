fhttp_debug: CFLAGS = -c -Wall -O2 -DFTC_DEBUG
fhttp_debug: fhttp

fhttp_release: CFLAGS = -c -O2
fhttp_release: fhttp

fhttp: fhttp.o configparser.o clientf.o httpparser.o libdate.o mimeparser.o libfs.o
	g++ -Wall -O2 -o fhttp.exe fhttp.o configparser.o clientf.o httpparser.o libdate.o mimeparser.o libfs.o

fhttp.o: fhttp.cpp constants.h clientf.h debuglog.h
	g++ $(CFLAGS) fhttp.cpp -o fhttp.o

configparser.o: configparser.cpp configparser.h debuglog.h
	g++ $(CFLAGS) configparser.cpp -o configparser.o
    
clientf.o: clientf.cpp clientf.h constants.h httpparser.h configparser.h libdate.h debuglog.h libfs.h
	g++ $(CFLAGS) clientf.cpp -o clientf.o

httpparser.o: httpparser.cpp httpparser.h constants.h configparser.h libdate.h mimeparser.h debuglog.h libfs.h
	g++ $(CFLAGS)  httpparser.cpp -o httpparser.o

libdate.o: libdate.cpp libdate.h constants.h debuglog.h
	g++ $(CFLAGS) libdate.cpp -o libdate.o

mimeparser.o: mimeparser.cpp mimeparser.h constants.h debuglog.h
	g++ $(CFLAGS) mimeparser.cpp -o mimeparser.o

libfs.o: libfs.cpp libfs.h
	g++ $(CFLAGS) libfs.cpp -o libfs.o

clean: 
	rm *.o
	rm *.exe

.PHONY: fhttp, fhttp_debug, fhttp_release, clean
