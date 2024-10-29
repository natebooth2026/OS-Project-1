#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <semaphore.h>
#include <pthread.h>

struct shmbuf {
    sem_t s;
    int table[100];
};

const size_t MEM_SIZE = sizeof(sem_t) + sizeof(int[100]);

void consumption(shmbuf*);

int main(){
    int memFD = 0;
    const char* SHM_NAME = "/table_memory";

    memFD = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(memFD == -1){
        fprintf(stderr, "Shared memory creation error");
        return 1;
    }

    if(ftruncate(memFD, MEM_SIZE) == -1){
        fprintf(stderr, "Shared memory truncation error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    shmbuf* shared = (shmbuf*)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFD, 0);
    if(shared == MAP_FAILED){
        fprintf(stderr, "Shared memory mapping error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    if(sem_init(&shared->s, 1, 1) == -1){
        fprintf(stderr, "Semaphore initalization error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    for(int i = 0; i < 100; ++i){
        shared->table[i] = 0;
    }

    //NEEDS THREADING

    munmap(shared, MEM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}

void consumption(shmbuf* shared){
    int consumed[100];

    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s);

        if(shared->table[i] != 0){
            consumed[i] = shared->table[i];
            printf("Consumed table item: %d\n", consumed[i]);
            shared->table[i] = 0;
        }

        sem_post(&shared->s);
    }
}