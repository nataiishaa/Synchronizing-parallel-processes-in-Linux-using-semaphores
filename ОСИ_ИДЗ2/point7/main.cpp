#include <iostream>
#include <thread>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <chrono>

const int numFighters = 6;
const char* SEMAPHORE_NAME = "/fighter_semaphore";

struct Fighter {
    int strength;
    bool defeated;

    explicit Fighter(int str) : strength(str), defeated(false) {}
};

void fight(Fighter& fighter1, Fighter& fighter2, sem_t* semaphore) {
    std::this_thread::sleep_for(std::chrono::milliseconds (fighter2.strength / fighter1.strength * 1000));

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

    sem_post(semaphore);
}

void finalFight(const std::vector<Fighter>& fighters, sem_t* semaphore) {
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
        std::thread fight_thread(fight, std::ref(const_cast<Fighter&>(fighters[winner1_idx])), std::ref(const_cast<Fighter&>(fighters[winner2_idx])), semaphore);
        fight_thread.join();
        sem_wait(semaphore);
        std::cout << "Финальный бой: Боец " << winner1_idx + 1 << " против Бойца " << winner2_idx + 1 << std::endl;
    }
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    std::vector<Fighter> fighters;
    for (int i = 0; i < numFighters; ++i) {
        int strength;
        printf("Введите энергию Ций бойца %d: ", i + 1);
        std::cin >> strength;
        fighters.emplace_back(strength);
    }

    printf("\nИнформация об энергиях Ций бойцов:\n");
    for (int i = 0; i < numFighters; ++i) {
        printf("Боец %d: %d\n", i + 1, fighters[i].strength);
    }
    printf("\n");

    sem_t* semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED) {
        std::cerr << "Ошибка создания семафора." << std::endl;
        return 1;
    }

    std::thread fights[3];
    for (int i = 0; i < 3; ++i) {
        fights[i] = std::thread(fight, std::ref(fighters[i * 2]), std::ref(fighters[i * 2 + 1]), semaphore);
    }

    for (int i = 0; i < 3; ++i) {
        sem_wait(semaphore);
    }

    for (int i = 0; i < 3; ++i) {
        fights[i].join();
    }

    finalFight(fighters, semaphore);

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

    sem_close(semaphore);
    sem_unlink(SEMAPHORE_NAME);

    return 0;
}

