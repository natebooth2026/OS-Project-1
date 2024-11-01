# Nate's Producer-Consumer Problem Solution

## Introduction
This project was to solve the producer-consumer problem using shared memory and semaphores. The program generates and consumes 100 items, producing 1-2 at a time and then consuming 1-2 at a time.

The following include statements were used in both files, these libraries are REQUIRED to run:
```
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <cstring>
#include <time.h> //NOTE: not in consumer
#include <stdlib.h> //NOTE: not in consumer
#include <pthread.h>
```

Some challenges I faced when solving this problem was trying to set up the shared memory as well as setting up and using the semaphores for/in my algorithms. Later on, I experienced issues with my producer and consumer algorithms, specifically finding out why semaphore waiting and signaling was happening when I thought it shouldn't have been.

## Running this Project
Fork this repository into a Linux/Unix-compatible interface (ex. VSCode) and run the following lines in sequence for optimal performance:
```
gcc producer.cpp -pthread -lrt -o producer
gcc consumer.cpp -pthread -lrt -o consumer
./producer & ./consumer &
```

## Usage/Output Examples to Keep in Mind
This program randomly generates integers as the produced and consumed items. Two items are produced before the two items are consumed. The dialog printed out (49 more times that is) will look something like this:

<img width="160" alt="exampleOutput" src="https://github.com/user-attachments/assets/677a245f-3ad9-49e7-83b4-2e2fa04475aa">


You may also have an output like this:

<img width="160" alt="exampleEndOfOutput" src="https://github.com/user-attachments/assets/34fa76b5-3a0c-45cb-b20c-6e2590185196">

This means the program doesn't finish automatically, the terminal may start looking for an input before the program is done. Just press enter.


There may also be cases where only one element is produced and then consumed, like this:

<img width="160" alt="singleProduceConsumeExample" src="https://github.com/user-attachments/assets/7c16d7e0-5d96-4576-9f3e-77dcede9e8ac">

This is only because the producer reaches the limit of items to be produced only after items 98 and 99 are consumed, this is a special case that should be expected due to the safety guards in both algorithms and the randomness of who gains control of the semaphore once released. This may occur before item 100.


## Thank You
Thank you for checking out my project! :D
