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

#include "parent_2.h"

#define SHARED_MEM_NAME "/posix-shared-mem"
#define SEM_MUTEX_NAME "/sem-mutex"
#define CHILD_MUTEX_NAME "/child-sem-mutex"

int main( int argc, const char* argv[] ) {
	// check arguments to ensure proper amount
	if (argc < 3) {
		cout << "need two arguments in form of ./a.out [word] [filepath]" << endl;
		return 1;
	}

    // unlink both sems to ensure they are fresh when created
    sem_unlink (SEM_MUTEX_NAME);
    sem_unlink (CHILD_MUTEX_NAME);

	// store arguments
	string word = argv[1];

    // create pipe for parent to pass size through
    int p[2];
    if (pipe(p) < 0) 
        exit(1);

	// Creating first child 
    int f = fork(); 
    
    if (f<0){
    	printf("FORKED FAILED!\n");
    	return 1;
    }
    else if (f > 0) { 
        // parent

        // get the size of the file
        int fd = open(argv[2], O_RDWR);
        size_t filesize = lseek(fd, 0, SEEK_END);
        int size = (int)filesize;

        // pass size to the child
        write(p[1],&size,sizeof(int));

        // passes the file name
        parent(argv[2]);

        // get the size from the child
        int pipe_size;
        read(p[0],&pipe_size,sizeof(int));
        //cout << "final size from child pipe is " << pipe_size << endl;

        // open the shared memory
        int fd_shm; 
        if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR, 0)) == -1)
            perror ("shm_open");

        
        // map shared memory with size from the child pipe write
        char *shm  = (char *)mmap (NULL, pipe_size, PROT_READ | PROT_WRITE, MAP_SHARED,fd_shm, 0);

        // if none found then pip size will be 0
        // this would cause seg fault so take into account
        if (pipe_size == 0) {
            shm_unlink(SHARED_MEM_NAME);
            munmap(shm,pipe_size);
            printf("FOUND 0 TIMES!\n\n");
            exit(0);
        }

        // print the final results
        printf("%s\n", shm);

        int found_count = 0;
        int return_size = strlen(shm);
        for(int i=0;i<return_size;++i){
            if (shm[i] == '\n')
                found_count++;
        }

        printf("FOUND %d TIMES!\n\n",found_count);

        shm_unlink(SHARED_MEM_NAME);
        munmap(shm,pipe_size);
    } 
    else if (f == 0) 
    { 
        //child

        // read size passes from parent
        int size;
        read(p[0],&size,sizeof(int));
        //cout << "from pipe" << size;

        // passes the word and size
        // will return the final size for the char array with results
        int shm_size = child(word,size);

        // pipe the final size to parent
        //cout << "final size from child is " << shm_size << endl;
        write(p[1],&shm_size,sizeof(int));
    } 
    

	return 0;
}