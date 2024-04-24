//
//compilation:
//gcc monkway-system-v-sem.c -o monkway-system-v-sem -Wall
//

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define     __DEBUG__

//the maximum number of processes
#define     MAXMONKS   30

//time delay
#define     DELAY            2

//Processes states
#define     READY       10
#define     FIGHTING    11
#define     LOSER       12
#define     WINNER      ~LOSER

//Semaphore keys
#define     CRITKEY   0x7cd
#define     SHMKEY    0x7ce

#define RIGHT (i+1)%MAXMONKS

//Shared memory
struct shared_memory{
      int energy[MAXMONKS];
      int monk_state[MAXMONKS];
};

//
void begin_fight(int);
void find_rival(int);
void define_winner(struct shared_memory*, int, int);
int find(struct shared_memory*, int);

// IPC functions (for readibility)
void sem_getids(int*);
struct shared_memory* mem_attach(void);
void mem_detach(struct shared_memory*);

// Semaphore operations
struct sembuf down = {0, -1, SEM_UNDO};
struct sembuf up = {0, +1, SEM_UNDO};

void m_down(int*);
void m_up(int*);

//+---------------------------------------------------------------------------+

int main(int argc, char const **argv){

    if(argc < 2){
        printf("*****************************\n"
               "./monkway number_of_processes\n"
               "*****************************\n");
        exit(0);
    }

    long monks = strtol(argv[1], NULL, 10);
    if(monks <= 0 || monks > MAXMONKS){
        printf("Incorrect number of processes!\n");
        exit(-3);
    }

    srand(time(NULL));

    // Get shared memory
    int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660 | IPC_CREAT);
    if(shmid < 0){
        perror("SHMGET error\n");
        exit(1);
    }

    // Attach
    struct shared_memory* shared;
    shared = shmat(shmid, NULL, 0);
    if(shared == (void*) -1){
        perror("SHMAT error\n");
        exit(1);
    }

    memset((struct shared_memory*)shared, 0, sizeof(struct shared_memory));

    //Initialize
    for(int step = 0; step < monks; step++){

        //every monk got from 10 to 59 points of energy
        shared->energy[step] = (rand() % 50) + 10;
        //waiting state
        shared->monk_state[step] = READY;
    }

    // semun definition
    union semun {
        int val;
    } arg;

    // Semaphores
    int state_sem;

    // Creating semaphores
    state_sem = semget(CRITKEY, 1, 0660 | IPC_CREAT);
    if(state_sem < 0){
        perror("SEMGET error");
        exit(1);
    }

    // Semaphore init
    arg.val = 1;
    if(semctl(state_sem, 0, SETVAL, arg)  < 0){
        perror("SEMCTL ERROR (state semaphore). Exiting.");
        exit(1);
    }

    // Storing PIDs
    pid_t monks_pids[monks];
    int monks_count = 0;
    pid_t pid;

    short sum;
    sum = 0;
    for(int t = 0; t < monks; t++){
        sum += shared->energy[t];
    }
#ifdef __DEBUG__
    printf("Total energy = %d\n", sum);
#endif

    for(int i = 0; i < monks; i++){

    	pid = fork();
        // Fork failed
        if(pid < 0){

            perror("Fork failed. Terminating processes and exiting.\n");
            for(int j = 0; j < monks_count; j++) kill(monks_pids[j], SIGTERM);

            exit(1);
        }

        //Parent process
    	else if(pid > 0){
    		monks_pids[i] = pid;
            monks_count++;
    	}

        //Child process
    	else if(pid == 0){
            while(1){
#ifdef __DEBUG__
                if(shared->monk_state[i] != LOSER){
                    printf("Monk %d energy = %d\n", i, shared->energy[i]);
                }
#endif
                if(shared->energy[i] == sum){
                    shared->monk_state[i] = WINNER;
                }
                begin_fight(i);
                sleep(DELAY);
    
            }
    	}
    } 

    // Waiting to finish
    printf("[+] Press ENTER for exit [+]\n");
    getchar();
    printf("Competition over: terminating processes and exiting.\n");
    for(int i = 0; i < monks; i++) kill(monks_pids[i], SIGTERM);

    // Detach memory
    mem_detach(shared);
    // Remove memory
    if(shmctl(shmid, IPC_RMID, 0) < 0){
        perror("SHMCTL error\n");
        exit(1);
    }
    // Remove semaphore
    if(semctl(state_sem, 0, IPC_RMID, 0) < 0){
        perror("SEMCTL error\n");
        exit(1);
    }

	return 0;
}

//+---------------------------------------------------------------------------+

void begin_fight(int monk_id){

    int i = monk_id;
    // Semaphores
    int state_sem;

    // Getting semaphores
    sem_getids(&state_sem);

    // Get shared memory
    struct shared_memory* shared = mem_attach();

    // Enter critical section
    m_down(&state_sem);

        // Be ready
        if(shared->monk_state[i] == READY)printf("Monk %d is ready to fight\n", i);
        else if(shared->monk_state[i] == WINNER)printf("Monk %d is winner!\n", i);

        find_rival(i);
        
    // Leave critical section
    m_up(&state_sem);

    // Detach memory
    mem_detach(shared);
}

//+---------------------------------------------------------------------------+

void define_winner(struct shared_memory* shared, int a, int b){
        int fight_time = 0;

        if(shared->energy[a] < shared->energy[b]){
            fight_time = (shared->energy[a] / shared->energy[b]) * 1000;
            sleep(fight_time);
            shared->monk_state[a] = LOSER;
            shared->monk_state[b] = READY;
            shared->energy[b] += shared->energy[a];
            printf("Monk %i is won. Monk %i is losed\n", b, a);
            return;
        }
        else if(shared->energy[a] > shared->energy[b]){
            fight_time = (shared->energy[b] / shared->energy[a]) * 1000;
            sleep(fight_time);
            shared->monk_state[b] = LOSER;
            shared->monk_state[a] = READY;
            shared->energy[a] += shared->energy[b];
            printf("Monk %i is won. Monk %i is losed\n", a, b);
        }

}

//+---------------------------------------------------------------------------+

int find(struct shared_memory* shared, int index){
    
    for(int i = 0; i < MAXMONKS; i++){
        if(shared->monk_state[i] == READY && i != index)
            return i;
    }
    return -1;
}

//+---------------------------------------------------------------------------+

void find_rival(int monk_id){

    int i = monk_id;
    // Semaphores
    int state_sem;
    int rival = -1;


    // Creating semaphores
    sem_getids(&state_sem);

    // Get shared memory
    struct shared_memory* shared = mem_attach();


    rival = find(shared, i);


    if(shared->monk_state[i] == READY && shared->monk_state[RIGHT] == READY){
        printf("Monk %i fighting with monk %i\n", i, RIGHT);
        // Begin pair fight
        shared->monk_state[i] = FIGHTING;
        shared->monk_state[RIGHT] = FIGHTING;
        //duration of fight
        define_winner(shared, i, RIGHT);
    }
    else if(shared->monk_state[i] == READY && rival != -1){
        printf("Monk %i fighting with monk %i\n", i, rival);
        // Begin pair fight
        shared->monk_state[i] = FIGHTING;
        shared->monk_state[rival] = FIGHTING;
        //duration of fight
        define_winner(shared, i, rival);
    }
 

    // Detach memory
    mem_detach(shared);
    return;
}

//+---------------------------------------------------------------------------+

void sem_getids(int* state){

    *state = semget(CRITKEY, 1, 0660);
    if(state < 0)
    {
        perror("SEMGET error");
        exit(1);    
    }
    return;
}

//+---------------------------------------------------------------------------+

struct shared_memory* mem_attach(){
    // Get shared memory
    int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660);
    if(shmid < 0)
    {
        perror("SHMGET error\n");
        exit(1);
    }
    // Attach
    struct shared_memory* mem;
    mem = shmat(shmid, NULL, 0);
    if(mem == (void*) -1)
    {
        perror("SHMAT error\n");
        exit(1);
    }
    return mem;
}

//+---------------------------------------------------------------------------+

void mem_detach(struct shared_memory* mem){
    if(shmdt(mem) < 0)
    {
        perror("SHMDT error\n");
        exit(1);
    }
    return;
}

//+---------------------------------------------------------------------------+

void m_down(int* mutex){
    if(semop(*mutex, &down, 1) < 0)
    {
        perror("Semop error\n");
        exit(1);
    }
}

//+---------------------------------------------------------------------------+

void m_up(int* mutex){
    if(semop(*mutex, &up, 1) < 0)
    {
        perror("Semop error\n");
        exit(1);
    }
}
