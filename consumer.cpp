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

void* consumption(void*);

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

    pthread_t thread;
    if(pthread_create(&thread, NULL, consumption, (void*)shared) != 0){
        fprintf(stderr, "Consumption thread creation error\n");
        shm_unlink(SHM_NAME);
        return 1;
    }
    pthread_join(thread, NULL);

    munmap(shared, MEM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}

void* consumption(void* arg){
    shmbuf* shared = (shmbuf*)arg;

    int consumed = 0;
    int size = 0;
    bool empty = false;
    int originalI = 0;
    
    for(int i = 0; i < 100; ++i){
        sem_wait(&shared->s);

        for(int j = 0; j < 100; ++j){
            if(shared->table[j] != 0){
                ++size;
            }
        }

        if(size == 0 && !empty) {
            sem_post(&shared->s);
            empty = true;
            originalI = i;
            --i;
            continue;
        } else if (size == 0){
            sem_post(&shared->s);
            i = originalI - 1;
            continue;
        } else {
            size = 0;
            empty = false;
        }
        
        if(shared->table[i] != 0){
            consumed = shared->table[i];
            printf("Consumed table item %d", i + 1);
            printf(": %d\n", consumed);
            shared->table[i] = 0;
        }
        
    
        sem_post(&shared->s);
        consumed = 0;
    }

    return NULL;
}