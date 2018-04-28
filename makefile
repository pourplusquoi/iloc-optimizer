CC = clang
CP = clang++
OPTIM = -O2
FLAGS = --std=c++11 -Wall -g

all: opt

opt: repre.o scanner.o parser.o util.o optim.o driver.o
	$(CP) $(OPTIM) -o opt repre.o scanner.o parser.o util.o optim.o driver.o

driver.o: parser.o driver.cc parser.h struct.h optim.h
	$(CP) $(OPTIM) -c driver.cc $(FLAGS)

parser.o: parser.c parser.h repre.h
	$(CC) $(OPTIM) -c parser.c

parser.h: iloc.y
	bison -o parser.c -d iloc.y

parser.c: iloc.y
	bison -o parser.c -d iloc.y

scanner.o: parser.o scanner.c parser.h
	$(CC) $(OPTIM) -c scanner.c

scanner.c: iloc.l
	flex -o scanner.c iloc.l

optim.o: optim.cc optim.h struct.h util.h
	$(CP) $(OPTIM) -c optim.cc $(FLAGS)

util.o: util.cc util.h
	$(CP) $(OPTIM) -c util.cc $(FLAGS)

repre.o: repre.cc repre.h
	$(CP) $(OPTIM) -c repre.cc $(FLAGS)

clean:
	rm -rf *.o opt scanner.c parser.c parser.h

wc:
	wc -l ./*.h ./*.cc