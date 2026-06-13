// gate.cpp
// 4단계 Gate 한 쌍 관리

#include "gate.h"
#include "board.h"
#include "snake.h"
#include <cstdlib>
#include <vector>

// 동작 파라미터
static const int GATE_FIRST_WAIT     = 30;   // 게임 시작 후 첫 출현까지 tick
static const int GATE_LIFETIME       = 100;  // 한 쌍이 살아 있는 tick 수
static const int GATE_RESPAWN_WAIT   = 30;   // 사라진 뒤 다음 쌍까지 대기 tick

// 작은 헬퍼들

static void dirToDelta(const Direction d, int& dy, int& dx) {
    dy = 0;
    dx = 0;
    if (d == DIR_UP)         dy = -1;
    else if (d == DIR_DOWN)  dy = 1;
    else if (d == DIR_LEFT)  dx = -1;
    else if (d == DIR_RIGHT) dx = 1;
}

static Direction rotateCW(const Direction d) {
    if (d == DIR_UP)    return DIR_RIGHT;
    if (d == DIR_RIGHT) return DIR_DOWN;
    if (d == DIR_DOWN)  return DIR_LEFT;
    if (d == DIR_LEFT)  return DIR_UP;
    return DIR_NONE;
}

static Direction rotateCCW(const Direction d) {
    if (d == DIR_UP)    return DIR_LEFT;
    if (d == DIR_LEFT)  return DIR_DOWN;
    if (d == DIR_DOWN)  return DIR_RIGHT;
    if (d == DIR_RIGHT) return DIR_UP;
    return DIR_NONE;
}

static Direction reverseDir(const Direction d) {
    if (d == DIR_UP)    return DIR_DOWN;
    if (d == DIR_DOWN)  return DIR_UP;
    if (d == DIR_LEFT)  return DIR_RIGHT;
    if (d == DIR_RIGHT) return DIR_LEFT;
    return DIR_NONE;
}

// 셀이 "통과 가능"한지
static bool isPassable(const int cell) {
    if (cell == EMPTY)        return true;
    if (cell == GROWTH_ITEM)  return true;
    if (cell == POISON_ITEM)  return true;
    if (cell == SPEED_ITEM)   return true;
    return false;
}

// 생성자

Gate::Gate() {
    active = false;
    waitTimer = GATE_FIRST_WAIT;
    aliveTimer = 0;
    pos[0].y = 0; pos[0].x = 0;
    pos[1].y = 0; pos[1].x = 0;
    inUse[0] = false;
    inUse[1] = false;
    useCount = 0;
}

// 가장자리 / 진출 방향

bool Gate::isEdge(const Board& board, const int y, const int x) const {
    if (y == 0)                     return true;
    if (y == board.getHeight() - 1)   return true;
    if (x == 0)                     return true;
    if (x == board.getWidth() - 1)    return true;
    return false;
}

Direction Gate::edgeExitDir(const Board& board, const int y, const int x) const {
    // 가장자리 Gate는 항상 맵 안쪽으로 진출
    if (y == 0)                     return DIR_DOWN;
    if (y == board.getHeight() - 1)   return DIR_UP;
    if (x == 0)                     return DIR_RIGHT;
    if (x == board.getWidth() - 1)    return DIR_LEFT;
    return DIR_NONE;
}

Direction Gate::interiorExitDir(const Board& board, const int y, const int x, const Direction inDir) const {
    // 우선순위: 직진 -> 시계방향 -> 반시계방향 -> 반대방향
    const Direction cands[4] = {
        inDir,
        rotateCW(inDir),
        rotateCCW(inDir),
        reverseDir(inDir)
    };

    for (int i = 0; i < 4; i++) {
        int dy, dx;
        dirToDelta(cands[i], dy, dx);
        const int ny = y + dy;
        const int nx = x + dx;
        if (isPassable(board.getCell(ny, nx))) {
            return cands[i];
        }
    }
    return DIR_NONE;
}

// Gate 한 쌍 출현 / 제거

bool Gate::spawnPair(Board& board, const Snake& /*snake*/) {
    // 후보 wall 셀 수집: 일반 WALL이면서 진출 가능한 방향이 하나 이상 있는 자리
    std::vector<Position> cands;
    const int H = board.getHeight();
    const int W = board.getWidth();

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (board.getCell(y, x) != WALL) continue;  // Immune Wall 제외

            bool ok = false;
            if (isEdge(board, y, x)) {
                // 가장자리: 안쪽 방향 한 칸이 통과 가능해야 함
                const Direction d = edgeExitDir(board, y, x);
                int dy, dx;
                dirToDelta(d, dy, dx);
                ok = isPassable(board.getCell(y + dy, x + dx));
            } else {
                // 내부 island wall: 4방향 중 하나라도 통과 가능하면 됨
                const int dy[4] = {-1, 1, 0, 0};
                const int dx[4] = {0, 0, -1, 1};
                for (int k = 0; k < 4; k++) {
                    if (isPassable(board.getCell(y + dy[k], x + dx[k]))) {
                        ok = true;
                        break;
                    }
                }
            }
            if (ok) {
                Position p;
                p.y = y; p.x = x;
                cands.push_back(p);
            }
        }
    }

    if ((int)cands.size() < 2) return false;

    // 두 자리 무작위 선택. 같으면 다시 뽑기 - 최대 50회
    const int i1 = rand() % (int)cands.size();
    int i2 = rand() % (int)cands.size();
    int tries = 0;
    while (i1 == i2 && tries < 50) {
        i2 = rand() % (int)cands.size();
        tries++;
    }
    if (i1 == i2) return false;

    // 맵에 표시
    pos[0] = cands[i1];
    pos[1] = cands[i2];
    board.setCell(pos[0].y, pos[0].x, GATE);
    board.setCell(pos[1].y, pos[1].x, GATE);
    inUse[0] = false;
    inUse[1] = false;
    return true;
}

void Gate::clearPair(Board& board) {
    if (active == false) return;
    // 워프 흔적: GATE 셀을 일반 벽이 아니라 USED_GATE_WALL 로 복원 다음 spawnPair 후보 검사(== WALL)에서 자동 제외되어 같은 자리에 Gate 가 다시 안 생김
    if (board.getCell(pos[0].y, pos[0].x) == GATE) {
        board.setCell(pos[0].y, pos[0].x, USED_GATE_WALL);
    }
    if (board.getCell(pos[1].y, pos[1].x) == GATE) {
        board.setCell(pos[1].y, pos[1].x, USED_GATE_WALL);
    }
    active = false;
    waitTimer = GATE_RESPAWN_WAIT; // 다음 한 쌍이 다른 자리에 뜨기까지의 대기
}

// 매 tick 업데이트

void Gate::update(Board& board, const Snake& snake) {
    if (active == false) {
        // 다음 출현까지 대기
        waitTimer--;
        if (waitTimer <= 0) {
            if (spawnPair(board, snake)) {
                active = true;
                aliveTimer = GATE_LIFETIME;
            } else {
                // 자리를 못 찾았으면 다음 시도까지 잠깐 더 대기
                waitTimer = GATE_RESPAWN_WAIT;
            }
        }
        return;
    }

    // active 상태: 수명 차감 - Snake 가 통과하지 않아도 결국 자연 만료
    aliveTimer--;
    if (aliveTimer <= 0) {
        clearPair(board); // 내부에서 waitTimer 도 같이 리셋
    }
}

// 텔레포트 시도

bool Gate::tryTeleport(const int newY, const int newX, const Direction inDir, Board& board,
                       int& outY, int& outX, Direction& outDir) {
    if (active == false) return false;

    // 어느 쪽 Gate에 진입했는지 식별
    int idx = -1;
    if (newY == pos[0].y && newX == pos[0].x) idx = 0;
    else if (newY == pos[1].y && newX == pos[1].x) idx = 1;
    else return false;

    const int otherIdx = 1 - idx;
    const int oy = pos[otherIdx].y;
    const int ox = pos[otherIdx].x;

    // 출구 진출 방향 결정
    Direction d;
    if (isEdge(board, oy, ox)) {
        d = edgeExitDir(board, oy, ox);
    } else {
        d = interiorExitDir(board, oy, ox, inDir);
    }
    if (d == DIR_NONE) {
        // 출구가 모두 막혀 있으면 통과 불가 -> 게임오버 처리
        return false;
    }

    int dy, dx;
    dirToDelta(d, dy, dx);
    outY = oy + dy;
    outX = ox + dx;
    outDir = d;

    inUse[idx] = true;
    useCount++;

    // 워프 흔적: Snake 가 한 번 통과한 즉시 두 자리 모두 USED_GATE_WALL 로 바꿈
    // 동시에 다음 쌍 출현 대기 타이머도 자동으로 시작
    clearPair(board);
    return true;
}
