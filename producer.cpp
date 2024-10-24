#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cstring>

struct shmbuf {
    bool lock;
    int table[2];
};

const size_t MEM_SIZE = sizeof(bool) + sizeof(int[2]);

bool test_and_set(bool*);

int main(int argc, char *argv[]){
    srand(time(nullptr));

    int memFD = 0;
    const char* SHM_NAME = "/table_memory";

    memFD = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(memFD == -1){
        fprintf(stderr, "Shared memory creation error");
        return 1;
    }

    if(ftruncate(memFD, MEM_SIZE) == -1){
        fprintf(stderr, "Shared memory truncation error: ");
        shm_unlink(SHM_NAME);
        return 1;
    }

    shmbuf* shared = (shmbuf*)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFD, 0);
    if(shared == MAP_FAILED){
        fprintf(stderr, "Shared memory mapping error: ");
        shm_unlink(SHM_NAME);
        return 1;
    }

    int produced[2] = {rand() % 1000, rand() % 1000};
    bool item0 = false;
    do {
        while(test_and_set(&shared->lock));
        if(shared->table[0] == NULL){
            shared->table[0] = produced[0];
            printf("Produced table item 0: ", shared->table[0]);
            item0 = true;
        } else if (shared->table[1] == NULL){
            shared->table[1] = produced[1];
            printf("Produced table item 1: ", shared->table[0]);
        }
        shared->lock = false;
        if(item0) {produced[0] = rand(); item0 = false;}
        else if(!item0) produced[1] = rand();
    } while (true);
}

bool test_and_set(bool *target){
    bool retVal = *target;
    *target = true;
    return retVal;
}