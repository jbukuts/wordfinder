#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <pthread.h>
#include <vector> 
#include <unistd.h>
#include <cstring>
#include <string>
using std::string;
#include <fstream>
using namespace std;
#include <iostream>
using std::cin;
using std::cout;
using std::endl;
#include <algorithm> 
#include <cctype>


#include "parent_2.h"

#define NUM_THREADS 4

#define SEM_MUTEX_NAME "/sem-mutex"
#define CHILD_MUTEX_NAME "/child-sem-mutex"
#define SHARED_MEM_NAME "/posix-shared-mem"


// holds the arguments for 
struct arg_struct {
    long thread_id;
    vector<string> v;
    vector<string> found;
    string word;
};

// this will read in the file
void parent(const char* path) {

    sem_t *mutex_sem;
    sem_t *child_mutex_sem;
    int fd_shm;
    char *src, *dest;

    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        perror ("sem_open");

    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((child_mutex_sem = sem_open (CHILD_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        perror ("sem_open");

    int fd = open(path, O_RDWR);
    size_t filesize = lseek(fd, 0, SEEK_END);

    //printf("File size: %zd\n", filesize);

    // source file
    src = (char *)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);

    // created shared memory 
    if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0666)) == -1)
        perror ("shm_open");

    // set size of shared memory
    ftruncate(fd_shm, filesize);

    // shared mem
    dest = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

    // copy file to shared memory
    memcpy(dest,src,filesize);

    // unmap files  
    munmap(src, filesize);
    munmap(dest, filesize);

    // close input file
    close(fd);

    //printf("PARENT TO CHILD.\n");


    // increment sem to 1 to unlock child as all data has been read in
    if (sem_post(mutex_sem) == -1)
        perror("sem_post");

    // forces to wait until mutex is 1
    if (sem_wait(child_mutex_sem) == -1)
        perror("sem_wait");


    //printf("PARENT DONE.\n");
}

// this will look for the word in the given text
int child(string word, int size) {
    //cout << "ENTERED CHILD" << endl;
    sem_t *mutex_sem;
    sem_t *child_mutex_sem;
    int fd_shm;

    // open semaphore
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED)
        perror ("sem_open");

    if ((child_mutex_sem = sem_open (CHILD_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED)
        perror ("sem_open");

    // forces to wait until mutex is 1
    if (sem_wait(mutex_sem) == -1)
        perror("sem_wait");

    // Get shared memory 
    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR, 0)) == -1)
        perror ("shm_open");

    // map shared memory
    char *shm = (char *)mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,fd_shm, 0);

    // vector of all lines the file
    vector<string> v;

    // add lines to the vector
    string line = "";
    for (int i=0;i<size;++i) {

        if (shm[i] == '\n') {
            // if new line is found add running line string to the vector
            // and empty line string
            v.push_back(line);
            line = "";
        }
        else if (i == size-1) {
            // if at the end add last character
            // and push to end of vector
            line += shm[i];
            v.push_back(line);
        }
        else 
            line += shm[i]; // else add to running line string
    }

    // test to print the entire file
    // print from the vector
    for (int i=0;i<(int)v.size();++i) {
        // cout << v.at(i) << endl;
    }

    // the amount of lines in the file
    //printf("there are %d lines in the file\n", (int)v.size());

    // array of threads and structs to hold arguments to pass
    pthread_t threads[NUM_THREADS];
    struct arg_struct args_arr[NUM_THREADS];
    int rc;

    // creates the threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        // cout << "child(): creating thread " << i << endl;
        // creates thread in the mapper
        args_arr[i].thread_id = i;
        args_arr[i].v = v;
        args_arr[i].word = word;
        rc = pthread_create(&threads[i],NULL,&Mapper,(void *)&args_arr[i]);

        if (rc) {
            cout << "ERROR : there was a problem creating thread " << i << " : " << rc << endl;
            exit(-1);
        }
    }

    // join the threads and wait for each to end
    void *status;
    for (int i=0;i< NUM_THREADS;++i){
        pthread_join(threads[i],&status);
    }

    // time to reduce and return to the parent

    
    // add lines found in threads to vector to be sorted
    vector<string> found_lines;
    for (int i=0;i<NUM_THREADS;++i) {
        // run through the vector for that thread
        for (int j=0;j<(int)args_arr[i].found.size();++j) {
            found_lines.push_back(args_arr[i].found.at(j));
        }
    }

    // sort the vector
    sort(found_lines.begin(),found_lines.end(),compare_strings);

    // add found vector lines to the string
    string end_string = "";
    for (int i=0;i<(int)found_lines.size();++i) {
        end_string += found_lines.at(i)+"\n";
    }

    // convet from string to char array to be copied to shared memory
    char *final_results = new char[end_string.length() + 1];
    strcpy(final_results, end_string.c_str());

    // print out the final sorted results
    // printf("%s\n",final_results);

    // unmap the shared memory
    munmap(shm, size);
    shm_unlink(SHARED_MEM_NAME);

    // created shared memory 
    if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0666)) == -1)
        perror ("shm_open");

    // set size of shared memory
    int final_size = strlen(final_results);
    //cout << "final size is " << final_size << endl;

    // size of the final results for the shared memory
    ftruncate(fd_shm, final_size);

    // shared mem
    char *dest = (char *)mmap(NULL, final_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

    // copy final results into the shared memory to shared memory
    memcpy(dest,final_results,final_size);

    // unmap the shared memory
    munmap(dest,final_size);

    //printf("CHILD TO PARENT.\n");

    if (sem_post(child_mutex_sem) == -1)
        perror("sem_post");

    // unlink mutex sem
    if (sem_unlink (SEM_MUTEX_NAME) == -1) {
        perror ("sem_unlink"); 
        exit (1);
    }

    // unlink mutex sem
    if (sem_unlink (CHILD_MUTEX_NAME) == -1) {
        perror ("sem_unlink"); 
        exit (1);
    }

    return final_size;
}


bool compare_strings(string a, string b) {
    return a<b;
}


// thread created by the child processes
// will search through input vector for specific word
void *Mapper(void *arguments) {
    struct arg_struct *args = (struct arg_struct *)arguments;

    vector<string> v = (args->v);

    // this determines starting point in the vector
    int offset = (v.size()/NUM_THREADS) * (args->thread_id);

    // this determines how long the loop with run
    int run = (v.size()/NUM_THREADS);

    for (int i=0;i<run;++i) {
        // get line and turn to lower case
        string line = v.at(offset+i);
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        // if the word of interest is found add to the found vector
        size_t pos = line.find((args->word));
        if (pos != std::string::npos) {
            if (!(isalpha(line[pos - 1])) && !(isalpha(line[pos + (args->word).size()]))) {
                (args->found).push_back(v.at(offset+i));
            }
        }
    }

    // exit the thread
    pthread_exit(NULL);
}