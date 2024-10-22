#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

struct shmbuf {
    bool flag[2];
    int turn;
    int table[2];
};

const size_t MEM_SIZE = sizeof(bool[2]) + sizeof(int), + sizeof(int[2]);

int main(int argc, char *argv[]){
    int memFD = 0;
    const char* SHM_NAME = "/table_memory";

    memFD = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(memFD == -1){
        std::cerr << "Shared memory creation error" << strerror(errno) << std::endl;
        return 1;
    }

    if(ftruncate(memFD, MEM_SIZE) == -1){
        std::cerr << "Shared memory truncation error: " << strerror(errno) << std::endl;
        shm_unlink(SHM_NAME);
        return 1;
    }

    void* ptr = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFD, 0);
    if(ptr == MAP_FAILED){
        std::cerr << "Shared memory mapping error: " << strerror(errno) << std::endl;
        shm_unlink(SHM_NAME);
        return 1;
    }
}