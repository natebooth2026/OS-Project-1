#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <cstring>
#include <vector>
#include <mutex>

struct shmbuf {
    int semaphore = 1;
    int table[100];
    std::vector<std::mutex> buffer;
};

std::mutex mtx;

const size_t MEM_SIZE = sizeof(int) + sizeof(int[100]) + sizeof(std::vector<std::mutex>);

void wait(int&, shmbuf*);
void signal(int&, shmbuf*);

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

    int produced = rand() % 1000;
    int i = 0; //tracks how many times it is run

    do {
        wait(shared->semaphore, shared);
        if(shared->table[i] == NULL){
            shared->table[i] = produced;
            printf("Produced table item: " + shared->table[i]);
        }
        signal(shared->semaphore, shared);
        produced = rand() % 1000;
        ++i;
    } while (i < 100);

    shm_unlink(SHM_NAME);
    return 0;
}

void wait(int& s, shmbuf* shared){
    --s;
    if(s < 0){
        shared->buffer.push_back(mtx);
        mtx.lock();
    }
}

void signal(int& s, shmbuf* shared){
    ++s;
    if(s >= 0){
        shared->buffer.front().unlock();
        shared->buffer.erase(shared->buffer.begin());
    }
}