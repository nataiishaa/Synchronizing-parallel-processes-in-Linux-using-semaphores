#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <vector>

// Number of fighters
const int numFighters = 6;
// Name of the named semaphore
const char* SEMAPHORE_NAME = "/fighter_semaphore";

int main() {
    // Create shared memory to store data about fighters
    key_t shm_key = ftok(".", 's');
    int shmid = shmget(shm_key, sizeof(int) * numFighters, 0666 | IPC_CREAT);

    // Access the shared memory
    int* fighter_strengths = (int*)shmat(shmid, NULL, 0);

    // Create and initialize an array of fighters
    std::vector<int> fighters;
    for (int i = 0; i < numFighters; ++i) {
        int strength;
        std::cout << "Enter the Qi energy of fighter " << i + 1 << ": ";
        std::cin >> strength;
        fighters.push_back(strength);
    }

    // Copy data into shared memory
    for (int i = 0; i < numFighters; ++i) {
        fighter_strengths[i] = fighters[i];
    }

    // Create a semaphore to control access to the shared memory
    int semid = semget(shm_key, 1, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 1);

    // Sleep to give time for the first program to read data
    sleep(3);

    // Release resources
    shmdt(fighter_strengths);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}

