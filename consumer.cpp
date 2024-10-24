#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <cstring>

struct shmbuf {
    bool lock = false;
    int table[2];
};

const size_t MEM_SIZE = sizeof(bool) + sizeof(int[2]);

bool test_and_set(bool*);

int main(int argc, char *argv[]){
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

    int consumed[2];
    bool item0 = false;
    do {
        while(test_and_set(&shared->lock));
        if(shared->table[0] != NULL){
            consumed[0] = shared->table[0];
            printf("Consumed table item 0: ", consumed[0]);
            shared->table[0] = NULL;
            item0 = true;
        } else if (shared->table[1] != NULL){
            consumed[1] = shared->table[1];
            printf("Consumed table item 1: ", consumed[1]);
            shared->table[1] = NULL;
        }
        shared->lock = false;
        if(item0) {consumed[0] = NULL; item0 = false;}
        else if(!item0) {consumed[1] = NULL;}
    } while (true);
}

bool test_and_set(bool *target){
    bool retVal = *target;
    *target = true;
    return retVal;
}