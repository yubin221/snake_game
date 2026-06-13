// snake.h
// 뱀의 위치/방향/이동을 관리하는 클래스

#ifndef SNAKE_H
#define SNAKE_H

#include "common.h"

class Board;   // 전방 선언
class Gate;    // 전방 선언 

class Snake {
public:
    Snake();

    // 맵을 훑어서 SNAKE_HEAD/SNAKE_BODY 가 있는 위치를 찾아 뱀 정보 등록
    bool initFromBoard(const Board& board);

    // 사용자가 누른 방향키를 다음 진행 방향으로 예약
    void requestDirection(const Direction d);

    // 한 tick 만큼 이동.
    // 반환값: -1 = 게임 오버, 그 외 = 머리가 도착한 칸의 원래 종류(Cell)
    int move(Board& board, Gate* const gate = nullptr);

    int getLength() const { return length; }
    Direction getDirection() const { return dir; }

private:
    Position body[SNAKE_MAX_LENGTH];  // body[0] = 머리, body[length-1] = 꼬리
    int length;
    Direction dir;       // 현재 진행 방향
    Direction nextDir;   // 다음 tick 에 적용할 방향 (사용자가 키 누른 결과)

    // a 와 b 가 서로 정반대 방향이면 true
    bool isOpposite(const Direction a, const Direction b) const;
};

#endif
