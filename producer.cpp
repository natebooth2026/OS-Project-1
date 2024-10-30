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

struct shmbuf {
    sem_t s;
    int table[100];
};

const size_t MEM_SIZE = sizeof(sem_t) + sizeof(int[100]);

void* production(void*);

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

    pthread_t thread;
    if(pthread_create(&thread, NULL, production, (void*)shared) != 0){
        fprintf(stderr, "Production thread creation error\n");
        shm_unlink(SHM_NAME);
        return 1;
    }
    pthread_join(thread, NULL);

    munmap(shared, MEM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}

void* production(void* arg){
    shmbuf* shared = (shmbuf*)arg;

    int produced[2] = {rand() % 1000 + 1, rand() % 1000 + 1};
    int size = 0;
    bool full = false;
    int originalI = 0;

    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s);

        for(int j = 0; j < 100; ++j){
            if(shared->table[j] != 0){
                ++size;
            }           
        }

        if(size >= 2 && !full) {
            sem_post(&shared->s);
            size = 0;
            full = true;
            originalI = i;
            --i;
            continue;
        } else if (size >= 2){
            sem_post(&shared->s);
            size = 0;
            i = originalI - 1;
            continue;
        } else {
            full = false;
        }

        if(shared->table[i] == 0){
            shared->table[i] = produced[i % 2];
            printf("Produced table item %d", i + 1);
            printf(": %d\n", shared->table[i]);
        }

        sem_post(&shared->s);
        produced[0] = rand() % 1000 + 1;
        produced[1] = rand() % 1000 + 1;
    }

    return NULL;
}