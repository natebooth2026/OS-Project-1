#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <cstring>
#include <time.h>
#include <stdlib.h>

struct shmbuf {
    int table[100];
};

const size_t MEM_SIZE = sizeof(int[100]);

void production(shmbuf*);

int main(){
    srand(time(nullptr));

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

    std::memset(shared, 0, MEM_SIZE);

    const char* SEM_NAME = "/p&c";

    sem_t* s = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 1);
    if(s == SEM_FAILED){
        fprintf(stderr, "Semaphore creation error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    for(int i = 0; i < 100; ++i){
        shared->table[i] = 0;
    }

    int produced[2] = {rand() % 1000 + 1, rand() % 1000 + 1};

    for(int i = 0; i < 50; ++i){
        sem_wait(s);

        for(int j = 0; j < 2; ++j){
            if(shared->table[i + j] == 0){
                shared->table[i + j] = produced[j];
                printf("Produced table item: %d\n", shared->table[i + j]);
            }
        }
        
        sem_post(s);
        produced[0] = rand() % 1000 + 1;
        produced[1] = rand() % 1000 + 1;
    }

    sem_unlink(SEM_NAME);
    munmap(shared, MEM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}