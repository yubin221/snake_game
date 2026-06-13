// BlockWall.h
// 4단계 추가 동작: 테트리스 블록 모양 벽
// - 맵 안쪽에 테트리스 도형 한 개가 무작위 위치/모양으로 나타남
// - 나타나기 1초 전에는 예고(BLOCK_WARN) 표시 → 통과 가능
// - 예고가 끝나면 진짜 벽(BLOCK_WALL)으로 굳음 → 충돌하면 게임오버
// - 일정 시간 뒤 사라지고, 잠시 후 다른 모양/위치에 다시 나타남

#ifndef BLOCK_WALL_H
#define BLOCK_WALL_H

#include "common.h"

class Board;
class Snake;

class BlockWall {
public:
    BlockWall();

    // 매 tick 호출.
    // 예고 -> 출현 -> 소멸 -> 재출현 흐름을 관리
    void update(Board& board, const Snake& snake);

private:
    static const int MAX_CELLS = 5;

    int phase;      // 0 = 대기, 1 = 예고 중, 2 = 벽 출현 중
    int timer;      // 현재 phase 가 끝나기까지 남은 tick
    Position cells[MAX_CELLS];
    int cellCount;

    // 무작위 도형/위치를 골라 예고(BLOCK_WARN) 로 표시. 성공 시 true
    bool trySpawnWarn(Board& board, const Snake& snake);

    // 예고 칸(BLOCK_WARN)을 실제 벽(BLOCK_WALL)으로 굳힘
    void hardenToWall(Board& board);

    // 블록 칸을 맵에서 지움 (지정한 셀 값인 칸만 EMPTY 로)
    void clearCells(Board& board, const int onlyIfCell);
};

#endif
