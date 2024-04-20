#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <semaphore.h>

const int numFighters = 6;

struct Fighter {
    int strength;
    bool defeated;

    explicit Fighter(int str) : strength(str), defeated(false) {}
};

void fight(Fighter& fighter1, Fighter& fighter2, sem_t& semaphore) {
    std::this_thread::sleep_for(std::chrono::milliseconds (fighter2.strength / fighter1.strength * 1000));

    sem_wait(&semaphore);
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
    sem_post(&semaphore);
}

void finalFight(std::vector<Fighter>& fighters, sem_t& semaphore) {
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
        fight(fighters[winner1_idx], fighters[winner2_idx], semaphore);

        for (auto& fighter : fighters) {
            if (!fighter.defeated && &fighter != &fighters[winner1_idx] && &fighter != &fighters[winner2_idx]) {
                fight(fighters[winner1_idx], fighter, semaphore);
                break;
            }
        }
    }
}


int main() {
    sem_t semaphore;
    sem_init(&semaphore, 0, 1);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    std::vector<Fighter> fighters;
    for (int i = 0; i < numFighters; ++i) {
        fighters.emplace_back(dis(gen));
    }

    printf("\nИнформация об энергиях Ций бойцов:\n");
    for (int i = 0; i < numFighters; ++i) {
        printf("Боец %d: %d\n", i + 1, fighters[i].strength);
    }
    printf("\n");

    std::thread fights[3];
    for (int i = 0; i < 3; ++i) {
        fights[i] = std::thread(fight, std::ref(fighters[i * 2]), std::ref(fighters[i * 2 + 1]), std::ref(semaphore));
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

    sem_destroy(&semaphore);

    return 0;
}

