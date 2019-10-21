
# wordfinder
C++ program that will find given word within a given file

## Compilation
included makefile has multiple options
- `make` will run the `test`  option in the makefile which will clean the directory and then proceed to run a test case on both of the sub projects
- `one` will only compile the first sub project and nothing else
- `two` will only compile the second sub project and nothing else
- `clean` of course will clean the working the directory of compiled files

## Usage
Once compiled each sub project will have its own executable either by the name of `main_1` or `main_2` with them relating to sub project 1 and sub project 2 respectively. 

In order to properly execute the program the proper usage is:
`./main_1 [word] [file_path]`

with the command line variables with self explanatory. Order is the same for both sub projects.

## Principles
This program makes us of various form of IPC to pass information between a child and parent process. Those that are made use of are:
- Pipes
- Unix Domain Socket
- Shared Memory

## Sub Project 1
This primarily makes use of one the Unix Domain Socket to pass information from child to parent and vice versa. The general design is that first the parent will read the file line by line and pass each line to the child process. As each line is passed the child will then check whether the word of interest is contained. If so then the line is added to a vector. After all line have been read from the file the child then sorts the vector and proceeds to return all lines of interest to the parent process with UDS. As each line is passed from the child the parent will print it to the terminal.

## Sub Project 2
This primarily makes use of  POSIX shared memory, however, pipes were also used to allow for the size of the shared memory to be passed from the parent to the child and vice versa. 

What occurs first is the parent gets the size of the file and passes it to the child via pipe. The parent also opens the shared memory and truncates with the size. A semaphore is made us of as well to ensure that the parent finishes reading all data from the file into shared memory before the child is able to read from it. 

After the parent finishes putting all data into share memory the child then reads in all line into a vector. 4 threads are started that allow the each to parse one fourth of the vector in search of lines containing the word of interest. The child joins each threads as to wait for all to be done before continuing. After all are joined the child acts to reduce the 4 separate vectors created by the threads into one and sorts it into one. The current shared memory is emptied and this new data from the sorted vector is store.

Lastly, the child pipes the new shared memory size to the parent and the parent opens and displays all data currently in the shared memory.







