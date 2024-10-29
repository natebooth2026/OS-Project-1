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
    sem_t s;
    int table[100];
};

const size_t MEM_SIZE = sizeof(sem_t) + sizeof(int[100]);

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

void production(shmbuf* shared){
    int produced = rand() % 1000 + 1;

    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s);

        if(shared->table[i] == 0){
            shared->table[i] = produced;
            printf("Produced table item: %d\n", shared->table[i]);
        }
        sem_post(&shared->s);

        produced = rand() % 1000 + 1;
    }
}