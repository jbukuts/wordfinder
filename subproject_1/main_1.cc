/*
** spair.c -- a simple socketpair() example program for sending a file between
** parent and child
*/

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
using std::cin;
using std::cout;
using std::endl;
#include <string>
using std::string;
#include <fstream>
using namespace std;
#include <algorithm> 
#include <sys/time.h>
#include <sys/resource.h>
#include <cctype>        
#include <stdio.h>
#include <string.h>
#include <cctype>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"

#define BUF_SIZE 4096
#define SEM_MUTEX_NAME "/sem-mutex-one"
#define SEM_MUTEX_NAME_TWO "/sem-mutex-two"

#include "main_1.h"



int main(int argc, char* argv[])
{
    // ensure all args are there
    if(argc != 3) {
        perror("Please specify the file name");
        exit(-1);
    }

    // set the word and the file path
    string word = argv[1];
    char* fpath = argv[2];
    
    // the pair of the socket descriptors
    int sv[2];

    /* Here I used SOCK_STREAM, but you can also consider SOCK_DGRAM which preserves message boundaries. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair!");
        exit(1);
    }

    // fork the program into parent and child
    int cid = fork();
    
    if(cid < 0) {
        perror("fork!");
        exit(-1);   
    }

    // create semaphores to properly mutex
    sem_t *mutex_sem;
    sem_t *mutex_sem_two;

    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0666, 0)) == SEM_FAILED)
        perror ("sem_open");

    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem_two = sem_open (SEM_MUTEX_NAME_TWO, O_CREAT, 0666, 0)) == SEM_FAILED)
        perror ("sem_open");


    if (cid > 0) {  
        // parent
        close(sv[1]);

        int fd = open(fpath, O_RDONLY);
        if (fd == -1) {
            perror("Open!");
            exit(-1);
        }
            
        /* Number of bytes returned by read() and write() */
        ssize_t ret_in;   
        char buffer[BUF_SIZE]; 
        while((ret_in = read (fd, &buffer, BUF_SIZE)) > 0){
            write(sv[0], &buffer, BUF_SIZE);
        }

        // end of write signifier
        write (sv[0], "\0", BUF_SIZE);

        int found_count = 0;
        char read_buffer[BUF_SIZE];
        ssize_t socket_in;


        while((socket_in = read (sv[0], &read_buffer, 1)) > 0) {
            // decrement
            sem_post(mutex_sem_two);
            // write to terminal
            //write (1, &read_buffer, (ssize_t) socket_in);

            cout << &read_buffer[0];
            
            // end of file signifier
            if(read_buffer[0] == '\0')
                break;

            // if new line found
            if(read_buffer[0] == '\n')
                found_count++;
        }

        // amount of times found
        printf("\nFOUND %d TIMES!\n\n",found_count);
       
        // unlink mutex sem
        if (sem_unlink(SEM_MUTEX_NAME) == -1) {
            perror ("sem_unlink"); 
            exit (1);
        }

        // unlink mutex sem
        if (sem_unlink(SEM_MUTEX_NAME_TWO) == -1) {
            perror ("sem_unlink"); 
            exit (1);
        }

        // close the sockets
        close(sv[0]);
        close(sv[1]);

        // to reap the child zombie process
        wait(NULL);
    } 
    else { 
    	// child
        close(sv[0]);

        ssize_t socket_in;
        char buffer[BUF_SIZE]; 

        // holds all line from the file
        vector<string> all_lines;

        int line_count = 0;
        all_lines.push_back("");

        // read in file char by char
        while((socket_in = read (sv[1], &buffer, BUF_SIZE)) > 0) {
        	/*The special character '\0' is encountered*/
            if (buffer[0] == '\0') {
                break;
            }
        	
        	for (int i=0;i<BUF_SIZE;++i) {
	            // if new line found push running line to vector
	            if (buffer[i] == '\n'){
	                line_count++;
	                all_lines.push_back("");
	            }
	            else
	                all_lines.at(line_count) += buffer[i];
        	}
        }

        // check all lines for the word
        vector<string> ret_lines;
        for (int i=0;i<(int)all_lines.size();++i) {
            // conv line to all lower case
            string line = all_lines.at(i);
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);

            // find word on line
            size_t pos = line.find(word);
            while(pos != string::npos){
                // if next to find are not letters
                if (!(isalpha(line[pos - 1])) && !(isalpha(line[pos + word.size()]))) {
                    
                    string begin = all_lines.at(i).substr(0,pos);
                    string middle = all_lines.at(i).substr(pos,word.size());
                    string end = all_lines.at(i).substr(pos+word.size());

                    ret_lines.push_back(begin+KRED+middle+KNRM+end+'\n');
                    break;
                }
                pos = line.find(word,pos+word.size());
            }
        }

        // sort the lines
        sort(ret_lines.begin(),ret_lines.end(),compare_strings);

        // empty large vector
        all_lines.clear();



        // through each line
        for (int i=0;i<(int)ret_lines.size();++i) {
            // through the line
            string line = ret_lines.at(i);
            for (int j=0;j<(int)line.size();++j) {
                // write each char of the line to the socket
                write(sv[1],&line[j],1);
                sem_post(mutex_sem_two);
            }
        }

        // write and release to denate end of pass
        write(sv[1],"\0",1);
        sem_post(mutex_sem);  
    }

    return 0;
}

// for sorting strings
bool compare_strings(string a, string b) {
    string new_a = a;
    string new_b = b;

    size_t a_pos = 0;
    if ((a_pos = a.find(KRED)) != string::npos && a_pos == 0)
        new_a = a.substr(8);

    size_t b_pos = 0;
    if ((b_pos = b.find(KRED)) != string::npos && b_pos == 0)
        new_b = b.substr(8);

    return new_a<new_b;
}