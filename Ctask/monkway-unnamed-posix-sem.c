//
//compilation:
//gcc monkway-unnamed-posix-sem.c -o monkway-unnamed-posix-sem -Wall
//
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
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

#define RIGHT (i+1)%MAXMONKS

const char memn[] = "shared-memory-unnamed"; //  имя объекта
const char sem_mem[] = "sem-memory";
sem_t *sem_p;   // адрес семафора

//Shared memory
struct shared_memory{
      int energy[MAXMONKS];
      int monk_state[MAXMONKS];
};

void begin_fight(int);
void find_rival(int);
void define_winner(struct shared_memory*, int, int);
int find(struct shared_memory*, int);

// IPC functions (for readibility)
struct shared_memory* mem_attach(int*);
void mem_detach(int);

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

  struct shared_memory *shared = NULL;
  int shmid, shmid_shared;

  // Создание объекта памяти
  if ( (shmid = shm_open(sem_mem, O_CREAT|O_RDWR, 0666)) == -1 ) {
    perror("shm_open: object is already open");
    exit(-1);
  } 

  // Задание размера объекта памяти
  if (ftruncate(shmid, sizeof (sem_t)) == -1) {
    perror("ftruncate: memory sizing error");
    exit(-1);
  } 

  // Получение доступа к разделяемой памяти
  sem_p = mmap(0, sizeof (sem_t), PROT_WRITE|PROT_READ, MAP_SHARED, shmid, 0);

  // Инициализация семафора в разделяемой памяти
  if (sem_init(sem_p, 1, 1) == -1) {
    perror("sem_init");
    exit(-1);
  }


    if ((shmid_shared = shm_open(memn, O_CREAT|O_RDWR|O_EXCL, 0666)) == -1 ) {
      perror("shm_open: object is already open");
      exit(-1);
    }
   
    if (ftruncate(shmid_shared, sizeof (struct shared_memory)) == -1) {
        perror("ftruncate: count memory sizing error");
        exit(-1);
      }

    // Получение доступа к разделяемой памяти счетчика
    shared = mmap(0, sizeof (struct shared_memory), PROT_WRITE|PROT_READ, MAP_SHARED, shmid_shared, 0);

    memset((struct shared_memory*)shared, 0, sizeof(struct shared_memory));

    //Initialize
    for(int step = 0; step < monks; step++){

        //every monk got from 10 to 59 points of energy
        shared->energy[step] = (rand() % 50) + 10;
        //waiting state
        shared->monk_state[step] = READY;
    }

    // Storing PIDs
    pid_t monks_pids[monks];
    int monks_count = 0;
    pid_t pid;

    short sum;
    sum = 0;

    for(int t = 0; t < monks; t++) sum += shared->energy[t];

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

    //закрыть открытый объект
    close(shmid);
    close(shmid_shared);

    // Remove memory
    if(shm_unlink(sem_mem) == -1) {
    printf("Shared memory is absent\n");
    perror("shm_unlink");
    return 1;
  }
  if(shm_unlink(memn) == -1){
    printf("Shared memory is absent\n");
    perror("shm_unlink");
    return 1;
  }

	return 0;
}

//+---------------------------------------------------------------------------+

void begin_fight(int monk_id){

    int i = monk_id;
     int shm = -2;

    // Get shared memory
    struct shared_memory* shared = mem_attach(&shm);


    // семафор ожидает, когда его поднимут, чтобы вычесть 1
  if(sem_wait(sem_p) == -1) {
    perror("sem_wait: Incorrect wait of posix semaphore");
    exit(-1);
  };

        // Be ready
        if(shared->monk_state[i] == READY)printf("Monk %d is ready to fight\n", i);
        else if(shared->monk_state[i] == WINNER)printf("Monk %d is winner!\n", i);

        find_rival(i);
        
   // Увеличение значения семафора на 1
  if(sem_post(sem_p) == -1) {
    perror("sem_post: Incorrect post of posix semaphore");
    exit(-1);
  };

  mem_detach(shm);

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
   
    int rival = -1;
    int shm = -2;

    // Get shared memory
    struct shared_memory* shared = mem_attach(&shm);


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
 

    //закрыть открытый объект
    mem_detach(shm);
    return;
}

//+---------------------------------------------------------------------------+

struct shared_memory* mem_attach(int *shm){
  char memn[] = "shared-memory-unnamed"; //  имя объекта
  struct shared_memory *shared;
  int shmem;

  //открыть объект
  if ( (shmem = shm_open(memn, O_RDWR, 0666)) == -1 ) {
    printf("Opening error\n");
    perror("shm_open");
    return NULL;
  } 

  *shm = shmem;

  //получить доступ к памяти
  shared = mmap(0, sizeof(struct shared_memory), PROT_WRITE|PROT_READ, MAP_SHARED, shmem, 0);
  if (shared == (struct shared_memory*)-1 ) {
    printf("Error getting pointer to shared memory\n");
    return NULL;
  }
    return shared;
}

//+---------------------------------------------------------------------------+

void mem_detach(int shm){
    //закрыть открытый объект
    close(shm);
}