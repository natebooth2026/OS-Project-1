#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <semaphore.h>
#include <pthread.h>

//shared memory structure
struct shmbuf {
    sem_t s;
    int table[2];
};

const size_t MEM_SIZE = sizeof(sem_t) + sizeof(int[2]); //size of shared memory

//holds consumption algorithm to be run by thread
void* consumption(void*);

int main(){
    //defines and creates shared memory (if not created already)
    int memFD = 0;
    const char* SHM_NAME = "/table_memory";

    memFD = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(memFD == -1){ //if failure
        fprintf(stderr, "Shared memory creation error");
        return 1;
    }

    //defines memory size
    if(ftruncate(memFD, MEM_SIZE) == -1){ //if failed
        fprintf(stderr, "Shared memory truncation error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    //maps memory
    shmbuf* shared = (shmbuf*)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFD, 0);
    if(shared == MAP_FAILED){ //if failed
        fprintf(stderr, "Shared memory mapping error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    //defines, creates, and runs thread
    pthread_t thread;
    if(pthread_create(&thread, NULL, consumption, (void*)shared) != 0){ //f failed
        fprintf(stderr, "Consumption thread creation error\n");
        shm_unlink(SHM_NAME);
        return 1;
    }
    pthread_join(thread, NULL);

    sem_close(&shared->s); //deallocates resources used on the semaphore
    munmap(shared, MEM_SIZE); //unmaps the mamory
    shm_unlink(SHM_NAME); //elimates access to the shared memory
    return 0;
}

//runs consumption algorithm on the shared table
void* consumption(void* arg){
    shmbuf* shared = (shmbuf*)arg; //converts void* shared back to shmbuf* shared

    int consumed = 0; //holds consumed number
    int size = 0; //holds size of table
    bool empty = false; //denotes if taable is empty or not
    int originalI = 0; //holds at what iteration the table was denoted empty
    
    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s); //waits for control

        //size calculation
        if(shared->table[0] != 0) ++size;
        if(shared->table[1] != 0) ++size;

        //size safety check
        if(size == 0 && !empty) { //empty but not denoted empty yet
            sem_post(&shared->s); //passes control to producer
            empty = true; //denotes table is empty
            originalI = i; //in case table remains empty next iteration
            --i; //sends back to current i for next iteration
            continue;
        } else if (size == 0){ //still empty
            sem_post(&shared->s); //passes control
            i = originalI - 1; //resets i back to when originally empty
            continue;
        } else { //not empty
            size = 0; //resets size
            empty = false; //table is not empty
        }
        
        //takes item out of table if not "null" and makes table item "null", prints item taken out
        if(shared->table[i % 2] != 0){
            consumed = shared->table[i % 2];
            printf("Consumed table item %d", i + 1);
            printf(": %d\n", consumed);
            shared->table[i % 2] = 0;
        }
        
    
        sem_post(&shared->s); //passes control to producer
        consumed = 0; //erases old value
    }

    return NULL;
}