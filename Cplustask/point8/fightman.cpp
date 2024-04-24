#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

const int numFighters = 6;
const char* SEMAPHORE_NAME = "/fighter_semaphore";

struct Fighter {
    int strength;
    bool defeated;

    explicit Fighter(int str) : strength(str), defeated(false) {}
};

void fight(Fighter& fighter1, Fighter& fighter2, int semid, int* count_array) {
    struct sembuf mybuf;
    mybuf.sem_num = 0;
    mybuf.sem_flg = 0;
    mybuf.sem_op  = -1;
    semop(semid, &mybuf, 1);

    ++count_array[0]; // Increase count of fights
    std::this_thread::sleep_for(std::chrono::milliseconds(fighter2.strength / fighter1.strength * 1000));

    printf("Бой: Боец с силой %d против бойца с силой %d\n", fighter1.strength, fighter2.strength);
    if (fighter1.strength > fighter2.strength) {
        fighter1.strength += fighter2.strength;
        fighter2.defeated = true;
        printf("Победитель: Боец с силой %d; Новая сила: %d\n", fighter1.strength - fighter2.strength, fighter1.strength);
    } else {
        fighter2.strength += fighter1.strength;
        fighter1.defeated = true;
        printf("Победитель: Боец с силой %d; Новая сила: %d\n", fighter2.strength - fighter1.strength, fighter2.strength);
    }

    mybuf.sem_op  = 1;
    semop(semid, &mybuf, 1);
}

void finalFight(std::vector<Fighter>& fighters, int semid, int* count_array) {
    int winner1_idx = -1, winner2_idx = -1;
    for (int i = 0; i < numFighters; ++i) {
        if (!fighters[i].defeated) {
            if (winner1_idx == -1) {
                winner1_idx = i;
            } else {
                winner2_idx = i;
                break;
            }
        }
    }

    if (winner1_idx != -1 && winner2_idx != -1) {
        fight(fighters[winner1_idx], fighters[winner2_idx], semid, count_array);
        std::cout << "Финальный бой: Боец " << winner1_idx + 1 << " vs Бойца " << winner2_idx + 1 << std::endl;
    }
}

int main() {
    key_t sem_key = ftok(SEMAPHORE_NAME, 0);
    int semid = semget(sem_key, 1, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 1);

    const int array_size = 4;
    key_t shm_key = ftok(".", 'a');
    int shmid = shmget(shm_key, sizeof(int) * array_size, 0666 | IPC_CREAT);
    int* count_array = (int*)shmat(shmid, NULL, 0);

    std::vector<Fighter> fighters;
    for (int i = 0; i < numFighters; ++i) {
        int strength;
        std::cout << "Введите энергию Ций бойца " << i + 1 << ": ";
        std::cin >> strength;
        fighters.emplace_back(strength);
    }

    printf("\nИнформация об энергиях Ций бойцов:\n");
    for (int i = 0; i < numFighters; ++i) {
        printf("Боец %d: %d\n", i + 1, fighters[i].strength);
    }
    printf("\n");

    // Первый бой
    fight(fighters[0], fighters[1], semid, count_array);
    // Второй бой
    fight(fighters[2], fighters[3], semid, count_array);
    // Третий бой
    fight(fighters[4], fighters[5], semid, count_array);

    // Четвертый // Пятый бой
    finalFight(fighters, semid, count_array);

    int winner_strength = -1;
    for (const auto& fighter : fighters) {
        if (!fighter.defeated) {
            winner_strength = fighter.strength;
            break;
        }
    }
    if (winner_strength != -1) {
        std::cout << "Сила победителя соревнования: " << winner_strength << std::endl;
    } else {
        std::cout << "Все бойцы были побеждены." << std::endl;
    }

    shmdt(count_array);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
