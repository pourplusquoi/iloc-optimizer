CC = clang
CP = clang++

all: opt

opt: repre.o scanner.o parser.o util.o optim.o driver.o
	$(CP) -O2 -o opt repre.o scanner.o parser.o util.o optim.o driver.o

driver.o: parser.o driver.cc parser.h struct.h optim.h
	$(CP) -O2 -c driver.cc --std=c++11

parser.o: parser.c parser.h repre.h
	$(CC) -O2 -c parser.c

parser.h: iloc.y
	bison -o parser.c -d iloc.y

parser.c: iloc.y
	bison -o parser.c -d iloc.y

scanner.o: parser.o scanner.c parser.h
	$(CC) -O2 -c scanner.c

scanner.c: iloc.l
	flex -o scanner.c iloc.l

optim.o: optim.cc optim.h struct.h util.h
	$(CP) -O2 -c optim.cc --std=c++11

util.o: util.cc util.h
	$(CP) -O2 -c util.cc --std=c++11

repre.o: repre.cc repre.h
	$(CP) -O2 -c repre.cc --std=c++11

clean:
	rm -rf *.o opt scanner.c parser.c parser.h

wc:
	wc -l ./*.h ./*.cc