// speeditem.h
// 속도 아이템(Speed Item)을 관리하는 클래스.
// 속도 아이템의 출현, 수명, 재출현 및 속도 효과 적용 상태를 담당한다.

#ifndef SPEEDITEM_H
#define SPEEDITEM_H

#include "common.h"

class Board;

class SpeedItem
{
public:
    SpeedItem();

    // 매 tick 호출하여 출현/수명/재출현을 관리
    void update(Board &board);

    // move() 직전에 호출하여 "맵에 있었는지"를 스냅샷
    void prepareSpeed(const Board &board);

    // move() 직후에 호출.
    // 반환값: 이번 tick 에 Growth/Poison 아이템도 갱신해야 하면 true
    bool updateSpeed(Board &board);

    // Speed 효과가 지속 중인지 여부
    bool isSpeedActive() const;

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
    int effectTimer; // Speed 효과 지속 tick
    int tickCounter; // 아이템 갱신 주기 카운터
    bool wasOnMap;   // move() 직전 맵 위 존재 여부 스냅샷

    // 빈 칸 하나를 무작위로 골라 아이템을 새로 배치
    void spawn(Board &board);
};

#endif
