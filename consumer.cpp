#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <semaphore.h>

struct shmbuf {
    int table[100];
};

const size_t MEM_SIZE = sizeof(int[100]);

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

    const char* SEM_NAME = "/p&c";

    sem_t* s = sem_open(SEM_NAME, 0);
    if(s == SEM_FAILED){
        fprintf(stderr, "Semaphore open error");
        shm_unlink(SHM_NAME);
        return 1;
    }

    int consumed = 0;

    for(int i = 0; i < 100; ++i){
        sem_wait(s);
        
        if(shared->table[i] != 0){
            consumed = shared->table[i];
            printf("Consumed table item: %d\n", consumed);
            shared->table[i] = 0;
        }

        sem_post(s);
        consumed = 0;
    }

    sem_unlink(SEM_NAME);
    munmap(shared, MEM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}

void consumption(shmbuf* shared){
    
}