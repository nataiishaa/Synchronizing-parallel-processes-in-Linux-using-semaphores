#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

const int numFighters = 6;
const char* FIGHT_CHANNEL = "/fight_channel";

struct Fighter {
    int strength;
    bool defeated;

    explicit Fighter(int str) : strength(str), defeated(false) {}
};

void fight(Fighter& fighter1, Fighter& fighter2, int fight_channel) {
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
    write(fight_channel, "1", 1);
}

void finalFight(const std::vector<Fighter>& fighters, int fight_channel) {
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
        printf("Финальный бой: Боец %d против Бойца %d\n", winner1_idx + 1, winner2_idx + 1);
        fight(const_cast<Fighter&>(fighters[winner1_idx]), const_cast<Fighter&>(fighters[winner2_idx]), fight_channel);
    }
}

int main() {
    mkfifo(FIGHT_CHANNEL, 0666);

    int fight_channel = open(FIGHT_CHANNEL, O_WRONLY);
    if (fight_channel == -1) {
        std::cerr << "Ошибка открытия канала для боев." << std::endl;
        return 1;
    }

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

    for (int i = 0; i < numFighters; ++i) {
        printf("Боец %d: %d\n", i + 1, fighters[i].strength);
    }

    fight(fighters[0], fighters[1], fight_channel);
    fight(fighters[2], fighters[3], fight_channel);
    fight(fighters[4], fighters[5], fight_channel);

    finalFight(fighters, fight_channel);

    close(fight_channel);
    unlink(FIGHT_CHANNEL);

    return 0;
}
