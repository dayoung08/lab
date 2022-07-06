#Makefile
CC = g++

try : main.o initialization.o algorithm_placement.o algorithm_migration.o infomation_file_create.o
	g++ -g main.o initialization.o algorithm_placement.o algorithm_migration.o infomation_file_create.o -o erslab.out

main.o : main.cpp
	g++ -c header.h main.cpp

initialization.o : initialization.cpp
	g++ -c header.h initialization.cpp

algorithm_placement.o : algorithm_placement.cpp
	g++ -c header.h algorithm_placement.cpp

algorithm_migration.o : algorithm_migration.cpp
	g++ -c header.h algorithm_migration.cpp

infomation_file_create.o : infomation_file_create.cpp
	g++ -c header.h infomation_file_create.cpp

all : try

clean :
	rm -rf *.o *.out

