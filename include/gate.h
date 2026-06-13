// gate.h
// 4단계: Gate 한 쌍을 관리하는 클래스
// - 시작 후 일정 시간이 지나면 한 쌍이 출현
// - 한 쌍은 일정 시간이 지나면 다른 위치로 이동
// - Snake 머리가 Gate 칸에 도착하면 다른 Gate로 텔레포트

#ifndef GATE_H
#define GATE_H

#include "common.h"

class Board;   // 전방 선언
class Snake;   // 전방 선언

class Gate
{
public:
    Gate();

    // 매 tick 한 번 호출.
    // 출현/수명/Wall 무작위 삭제 처리
    void update(Board &board, const Snake &snake);

    // 머리가 다음 칸 (newY, newX) = GATE 셀에 진입하려 할 때 호출
    // 진입 가능하면 텔레포트 후 새 머리 좌표/방향을 out 매개변수로 돌려주고 true
    // 막혀 있으면 false
    bool tryTeleport(const int newY, const int newX, const Direction inDir, Board &board,
                     int &outY, int &outX, Direction &outDir);

    // 게이트 통과 횟수 반환
    int getUseCount() const { return useCount; }

    // 게이트가 현재 활성화(맵에 존재) 상태인지 반환
    bool isActive() const { return active; }

private:
    bool active;     // 한 쌍이 맵에 떠 있는가
    int waitTimer;   // 다음 출현까지 남은 tick, active=false 일 때만 사용
    int aliveTimer;  // 현재 쌍의 남은 수명, active=true 일 때 사용
    Position pos[2]; // Gate 두 개 위치
    bool inUse[2];   // 진입 중 플래그, 사라짐/이동 방지
    int useCount;    // 통과 횟수 누적

    // 새 한 쌍을 맵에 배치. 
    // 자리 못 잡으면 false
    bool spawnPair(Board &board, const Snake &snake);

    // 현재 쌍을 맵에서 지우고 active=false 로
    void clearPair(Board &board);

    // 가장자리 wall 인지 검사
    bool isEdge(const Board &board, const int y, const int x) const;

    // 가장자리 Gate 의 진출 방향
    // 맵 안쪽으로 고정
    Direction edgeExitDir(const Board &board, const int y, const int x) const;

    // 내부(island) Gate 의 진출 방향. 우선순위:
    //  1) 진입 방향과 일치 직진
    //  2) 시계방향 회전
    //  3) 반시계방향 회전
    //  4) 반대방향
    // 다 막히면 DIR_NONE 반환
    Direction interiorExitDir(const Board &board, const int y, const int x, const Direction inDir) const;
};

#endif
