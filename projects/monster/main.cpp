#include <iostream>
#include <cstdlib>
#include <ctime>
#include "player.h"

void round();

int main() {
    std::srand(std::time(nullptr));

    Player a, b;
    a.add_monster(fireMon);
    a.add_monster(fireMon);
    a.add_monster(waterMon);
    b.add_monster(waterMon);
    b.add_monster(grassMon);
    b.add_monster(grassMon);

    for (int i = 1;; i++) {
        round();
        std::cout << "Round " << i << ": " << a.get_total_hp() << " " << b.get_total_hp() << std::endl;
        if (b.get_num_monsters() == 0) {
            std::cout << "Player a won the game!" << std::endl;
            break;
        } else if (a.get_num_monsters() == 0) {
            std::cout << "Player b won the game!" << std::endl;
            break;
        }
    }
    return 0;
}

void round(Player &a, Player &b) {
    Monster *a_mon = a.select_monster();
    Monster *b_mon = b.select_monster();

    int result = (*a_mon) * (*b_mon);
    if (result & 0b01) {
        a.delete_monster(a_mon);
    }
    if (result & 0b10) {
        b.delete_monster(b_mon);
    }

    return;
}
