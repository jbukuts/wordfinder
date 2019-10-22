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


    if (cid > 0) {  
        // parent
        close(sv[1]);

        int fd = open(fpath, O_RDONLY);
        if (fd == -1) {
            perror("Open!");
            kill(cid, SIGKILL);
            exit(-1);
        }
        
        sem_post(mutex_sem);

        /* Number of bytes returned by read() and write() */
        ssize_t ret_in, ret_out;   
        char buffer[BUF_SIZE]; 
        while((ret_in = read (fd, &buffer, 1)) > 0){
            
            ret_out = write (sv[0], &buffer, 1);
            if(ret_out != ret_in){
                /* Write error */
                perror("write");
                kill(cid, SIGKILL);
                exit(-1);
            }
        }

        write (sv[0], "\0", 1);

        sem_wait(mutex_sem);
        //sem_wait(mutex_sem_two);

        int found_count = 0;
        char read_buffer[BUF_SIZE];
        ssize_t socket_in;
        while((socket_in = read (sv[0], &read_buffer, 1)) > 0) {
            sem_post(mutex_sem_two);
            write (1, &read_buffer, (ssize_t) socket_in);
            /*The special character '\0' is encountered*/
            if(read_buffer[0] == '\0')
                break;

            if(read_buffer[0] == '\n')
                found_count++;
        }

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

        close(sv[0]);
        close(sv[1]);

        // to reap the child zombie process
        wait(NULL);
    } 
    else { 
        close(sv[0]);

        ssize_t socket_in;
        char buffer[BUF_SIZE]; 

        vector<string> all_lines;

        int line_count = 0;
        all_lines.push_back("");

        while((socket_in = read (sv[1], &buffer, 1)) > 0) {

            if (buffer[0] == '\n'){
                line_count++;
                all_lines.push_back("");
            }
            else
                all_lines.at(line_count) += buffer[0];
            // conver to string
            string line(buffer);

            //write (1, &buffer, (ssize_t) socket_in);

            /*The special character '\0' is encountered*/
            if(buffer[0] == '\0')
                break;
        }

        vector<string> ret_lines;
        for (int i=0;i<all_lines.size();++i) {
            string line = all_lines.at(i);
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);

            // find word on line
            size_t pos = line.find(word);
            if (pos != string::npos) {
                if (!(isalpha(line[pos - 1])) && !(isalpha(line[pos + word.size()]))){
                    ret_lines.push_back(all_lines.at(i)+"\n");
                }
            }
        }

        sort(ret_lines.begin(),ret_lines.end(),compare_strings);

        all_lines.clear();


        for (int i=0;i<ret_lines.size();++i) {
            string line = ret_lines.at(i);
            for (int j=0;j<line.size();++j) {

                // cout << line[j];
                write(sv[1],&line[j],1);
                sem_post(mutex_sem_two);
            }
        }

        write(sv[1],"\0",1);

        sem_post(mutex_sem);  
    }

    return 0;
}

bool compare_strings(string a, string b) {
    return a<b;
}