CC = clang
CP = clang++
OPTIM = -O3
FLAGS = --std=c++17 -Wall

all: opt

opt: repre.o scanner.o parser.o util.o optim.o driver.o
	$(CP) $(OPTIM) -o opt repre.o scanner.o parser.o util.o optim.o driver.o

driver.o: parser.o source/driver.cc parser.h headers/struct.h headers/optim.h
	$(CP) $(OPTIM) -c source/driver.cc $(FLAGS)

parser.o: parser.c parser.h headers/repre.h
	$(CC) $(OPTIM) -c source/parser.c

parser.h: source/iloc.y
	bison -o source/parser.c -d source/iloc.y

parser.c: source/iloc.y
	bison -o source/parser.c -d source/iloc.y

scanner.o: parser.o scanner.c parser.h
	$(CC) $(OPTIM) -c source/scanner.c

scanner.c: source/iloc.l
	flex -o source/scanner.c source/iloc.l

optim.o: source/optim.cc headers/optim.h headers/struct.h headers/util.h
	$(CP) $(OPTIM) -c source/optim.cc $(FLAGS)

util.o: source/util.cc headers/util.h
	$(CP) $(OPTIM) -c source/util.cc $(FLAGS)

repre.o: source/repre.cc headers/repre.h
	$(CP) $(OPTIM) -c source/repre.cc $(FLAGS)

clean:
	rm -rf *.o opt source/scanner.c source/parser.c source/parser.h

wc:
	wc -l ./*/*.h ./*/*.cc