#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <cstring>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

//shared memory structure
struct shmbuf {
    sem_t s;
    int table[2];
};

const size_t MEM_SIZE = sizeof(sem_t) + sizeof(int[2]); //holds shared memory size

//holds the production algorithm to be called by the thread
void* production(void*);

int main(){
    srand(time(nullptr)); //for the random numbers

    //defines and opens shared memory (if not opened already)
    int memFD = 0;
    const char* SHM_NAME = "/table_memory";

    memFD = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(memFD == -1){ //if shm_open() failed
        fprintf(stderr, "Shared memory creation error");
        return 1;
    }

    //defines size of the memory
    if(ftruncate(memFD, MEM_SIZE) == -1){ //if the sizing fails
        fprintf(stderr, "Shared memory truncation error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    //maps the shared memory to support only the shmbuf structure
    shmbuf* shared = (shmbuf*)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFD, 0);
    if(shared == MAP_FAILED){ //if mapping failed
        fprintf(stderr, "Shared memory mapping error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    //initalizes shared memory
    std::memset(shared, 0, MEM_SIZE);

    //initalizes the semaphore in shared
    if(sem_init(&shared->s, 1, 1) == -1){ //if failed
        fprintf(stderr, "Semaphore initalization error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    //initalizes the table
    shared->table[0] = 0;
    shared->table[1] = 0;

    //opens, creates, and runs the thread that runs production()
    pthread_t thread;
    if(pthread_create(&thread, NULL, production, (void*)shared) != 0){ //if thread creation fails
        fprintf(stderr, "Production thread creation error\n");
        shm_unlink(SHM_NAME);
        return 1;
    }
    pthread_join(thread, NULL);

    sem_close(&shared->s); //deallocates resources used on the semaphore
    munmap(shared, MEM_SIZE); //unmaps the mamory
    shm_unlink(SHM_NAME); //elimates access to the shared memory
    return 0;
}

//runs production algorithm on the shared table
void* production(void* arg){
    shmbuf* shared = (shmbuf*)arg; //converts void* shared to shmbuf* shared to be used

    int produced[2] = {rand() % 1000 + 1, rand() % 1000 + 1}; //intializes produced values
    int size = 0; //holds the amount of full elements in the table
    bool full = false; //denotes if the table is full
    int originalI = 0; //holds i value when full becomes true

    //checks size contraints and inserts elements into table if control has been given to the process
    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s); //waits for semaphore lock

        //size calculation
        if(shared->table[0] != 0) ++size;
        if(shared->table[1] != 0) ++size;

        //size safety check
        if(size >= 2 && !full) { //full but full has not been denoted yet
            sem_post(&shared->s); //passes control to consumer
            size = 0; //resets size
            full = true; //denotes table is full now
            originalI = i; //hold original i value in case it still full during next iteration
            --i; //sends i back to current i after post-statement
            continue;
        } else if (size >= 2) { //still full
            sem_post(&shared->s); //passes control
            size = 0; //resets size
            i = originalI - 1; //resets i to when the table was first full (- 1 due to post-statement)
            continue;
        } else { //not full
            full = false;
        }

        //inserts into table if element is "empty" (== 0), prints item placed in table
        if(shared->table[i % 2] == 0){
            shared->table[i % 2] = produced[i % 2];
            printf("Produced table item %d", i + 1);
            printf(": %d\n", shared->table[i % 2]);
        }

        sem_post(&shared->s); //passes control to consumer

        //creates new values to pass to table
        produced[0] = rand() % 1000 + 1;
        produced[1] = rand() % 1000 + 1;
    }

    return NULL;
}