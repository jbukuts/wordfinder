CC = g++  # use g++ compiler

FLAGS = -std=c++11  # compile with C++ 11 standard
FLAGS += -Wall      # compile with all warnings

INCLUDE = -I inc  # add include and test dirs to include path
FLAGS += $(INCLUDE)

LINK = $(CC) $(FLAGS) -o  # final linked build to binary executable

COMPILE = $(CC) $(FLAGS) -c  # compilation to inter	mediary .o files


test: clean main_1 main_2
	./main_1 test text_files/test.txt
	./main_2 test text_files/test.txt  

one: clean main_1

two: clean main_2

build: main_2
	
main_1: main_1.o
	$(LINK) $@ $^ -lpthread -lrt
main_1.o: subproject_1/main_1.cc 
	$(COMPILE) $<


main_2: main_2.o parent_2.o
	$(LINK) $@  $^ -lpthread -lrt
main_2.o: subproject_2/main_2.cc 
	$(COMPILE) $<

parent_2.o: subproject_2/parent_2.cc subproject_2/parent_2.h
	$(COMPILE) $< -o $@

clean:
	rm -f main_1
	rm -f main_2
	rm -f *.o
