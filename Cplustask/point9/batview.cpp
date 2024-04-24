#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char* FIGHT_CHANNEL = "/fight_channel";

int main() {
    mkfifo(FIGHT_CHANNEL, 0666);

    int fight_channel = open(FIGHT_CHANNEL, O_RDONLY);
    if (fight_channel == -1) {
        std::cerr << "Ошибка открытия канала для боев." << std::endl;
        return 1;
    }

    char buffer[2];
    while (read(fight_channel, buffer, 2) > 0) {
        std::cout << buffer[0] << std::endl;
    }

    close(fight_channel);
    unlink(FIGHT_CHANNEL);

    return 0;
}

