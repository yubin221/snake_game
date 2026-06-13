// food.h
// 먹이(Growth Item)를 관리하는 클래스.
// 먹이의 출현, 수명, 재출현을 담당한다.

#ifndef FOOD_H
#define FOOD_H

#include "common.h"

class Board;

class Food
{
public:
    Food();

    // 매 tick 호출하여 출현/수명/재출현을 관리
    void update(Board &board);

    // 아이템 상태 초기화
    void reset();

    int getY() const { return y; }
    int getX() const { return x; }
    bool isActive() const { return active; }

private:
    int y;           // 맵에서의 y 좌표
    int x;           // 맵에서의 x 좌표
    bool active;     // 현재 맵 위에 존재하는가
    int timer;       // 맵 위 수명 (남은 tick)

    // 빈 칸 하나를 무작위로 골라 아이템을 새로 배치
    void spawn(Board &board);
};

#endif
