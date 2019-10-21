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


#include "main_1.h"

#define BUF_SIZE 256
#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_MUTEX_NAME_TWO "/sem-mutex-two"



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

    // tell user the word being looked for
    //cout << "word is " << word << endl;
    
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


    sem_post(mutex_sem);

        
    if (cid > 0) {  
        // parent

        // get the file
        ifstream in(fpath);

        string line = "";
        char str[BUF_SIZE];
 
        // read the file line by line into str
        while(getline(in,line)) {

        	sem_wait(mutex_sem);

            // copy string into str and null terminate
        	line.copy(str,line.size()+1);
        	str[line.size()] = '\0';

        	//cout << str << endl;

            // pass to child through the socket
        	write(sv[0],&str,BUF_SIZE);

        	sem_post(mutex_sem_two);
        }

        // cout << "write done" << endl;

        // this tell the child that the file is over
        write (sv[0], "\0\0", 2);
        sem_post(mutex_sem_two);

        // this will count the amout of times the word is found
        int find_count = 0;

        // recieve lines from the child that contain the word
        while (1) {
            // read the line
        	read(sv[0],&str,BUF_SIZE);

        	// check if file over
        	if (str[0] == '\0' && str[1] == '\0'){
        		break;
        	}

            // increment counter and print the line
        	find_count++;
        	cout << str << endl;
        }

        cout << "\n" << word << " WAS FOUND " << find_count << " TIMES!\n" << endl;

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

        // to reap the child zombie process
        wait(NULL);
    } 
    else { 
        // child
        char str[BUF_SIZE];
        vector<string> all_lines;

        while(1) {
        	sem_wait(mutex_sem_two);

        	// read from socket
        	read(sv[1],&str,BUF_SIZE);

        	// conver to string
        	string line(str);

        	// find word on line
        	if (line.find(" "+word) != string::npos ||
        		line.find(" "+word+",") != string::npos ||
        		line.find(" "+word+";") != string::npos ||
        		line.find(" "+word+":") != string::npos ||
        		line.find(" "+word+".") != string::npos ||
        		line.find(word) != string::npos){
        		all_lines.push_back(line);
        	}

      		// check if file over
        	if (str[0] == '\0' && str[1] == '\0'){
        		break;
        	}

            // release to allow for another line to be passed
        	sem_post(mutex_sem);
        }

        // sort the lines aplhabetically
        sort(all_lines.begin(),all_lines.end(),compare_strings);


        // return lines where file is found to the parent
        for (int i=0;i<(int)all_lines.size();++i) {
            // turn string into char array
        	all_lines.at(i).copy(str,all_lines.at(i).size()+1);

            // null terminate line
        	str[all_lines.at(i).size()] = '\0';

            // pass the line through the socket
        	write(sv[1],&str,BUF_SIZE);
        }

        // end of return lines to parent
        write (sv[1], "\0\0", 2);
    }

    return 0;
}

bool compare_strings(string a, string b) {
    return a<b;
}