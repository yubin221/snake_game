// BlockWall.cpp
// 테트리스 블록 모양 벽 -> 예고 -> 출현 -> 소멸 -> 재출현

#include "BlockWall.h"
#include "board.h"
#include "snake.h"
#include <cstdlib>

// 동작 파라미터
static const int FIRST_WAIT     = 40;  // 게임 시작 후 첫 블록까지
static const int WARN_DURATION  = 5;   // 예고 표시 시간 - 약 1초
static const int ALIVE_DURATION = 60;  // 블록 벽이 유지되는 시간 - 12초
static const int RESPAWN_WAIT   = 25;  // 사라진 뒤 다음 블록까지 - 5초

// 머리에서 이 거리안에는 블록을 안 띄움
static const int SAFE_DIST = 4;

// 테트리스 도형 정의 
// 각 도형은 기준점 (0,0) 에서의 상대좌표 (dy, dx) 목록
// 한 도형은 최대 5칸
static const int SHAPE_MAX = 5;
struct Shape {
    int count;
    int dy[SHAPE_MAX];
    int dx[SHAPE_MAX];
};

static const int SHAPE_COUNT = 6;
static const Shape SHAPES[SHAPE_COUNT] = {
    { 4, {0, 0, 0, 0}, {0, 1, 2, 3} },        // I
    { 4, {0, 0, 1, 1}, {0, 1, 0, 1} },        // O
    { 4, {0, 1, 1, 1}, {1, 0, 1, 2} },        // T
    { 4, {0, 1, 2, 2}, {0, 0, 0, 1} },        // L
    { 4, {0, 1, 2, 2}, {1, 1, 1, 0} },        // J
    { 5, {0, 0, 0, 1, 2}, {0, 1, 2, 2, 2} },  // ㄱ
};

// 생성자

BlockWall::BlockWall() {
    phase = 0;             // 대기부터 시작
    timer = FIRST_WAIT;
    cellCount = 0;
    for (int i = 0; i < MAX_CELLS; i++) {
        cells[i].y = 0;
        cells[i].x = 0;
    }
}

// 예고 배치

bool BlockWall::trySpawnWarn(Board& board, const Snake& /*snake*/) {
    const int H = board.getHeight();
    const int W = board.getWidth();

    // 뱀 머리 위치 찾기
    int headY = -1;
    int headX = -1;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (board.getCell(y, x) == SNAKE_HEAD) {
                headY = y;
                headX = x;
            }
        }
    }

    // 자리를 못 잡을 수 있으니 여러 번 시도
    for (int attempt = 0; attempt < 40; attempt++) {
        const Shape& sh = SHAPES[rand() % SHAPE_COUNT];

        // 도형이 차지하는 최대 크기
        int maxDy = 0;
        int maxDx = 0;
        for (int i = 0; i < sh.count; i++) {
            if (sh.dy[i] > maxDy) maxDy = sh.dy[i];
            if (sh.dx[i] > maxDx) maxDx = sh.dx[i];
        }

        // 가장자리 벽(0행/열, H-1/W-1)을 피해 안쪽에만 배치
        const int rangeY = H - 2 - maxDy; 
        const int rangeX = W - 2 - maxDx;
        if (rangeY < 1 || rangeX < 1) continue;
        const int oy = 1 + rand() % rangeY;
        const int ox = 1 + rand() % rangeX;

        // 모든 칸이 빈 칸이고 머리에서 충분히 떨어졌는지 검사
        bool ok = true;
        for (int i = 0; i < sh.count; i++) {
            const int cy = oy + sh.dy[i];
            const int cx = ox + sh.dx[i];
            if (board.getCell(cy, cx) != EMPTY) {
                ok = false;
                break;
            }
            if (headY >= 0) {
                const int dist = abs(cy - headY) + abs(cx - headX);
                if (dist < SAFE_DIST) {
                    ok = false;
                    break;
                }
            }
        }
        if (ok == false) continue;

        // 배치 — 예고로 표시
        cellCount = sh.count;
        for (int i = 0; i < sh.count; i++) {
            cells[i].y = oy + sh.dy[i];
            cells[i].x = ox + sh.dx[i];
            board.setCell(cells[i].y, cells[i].x, BLOCK_WARN);
        }
        return true;
    }
    return false;
}

// 예고를 실제 벽으로 굳히기

void BlockWall::hardenToWall(Board& board) {
    // 아직 BLOCK_WARN 인 칸만 굳힘
    // 예고 동안 뱀이 지나가 덮은 칸은 BLOCK_WARN 이 아니므로 자동 스킵
    for (int i = 0; i < cellCount; i++) {
        if (board.getCell(cells[i].y, cells[i].x) == BLOCK_WARN) {
            board.setCell(cells[i].y, cells[i].x, BLOCK_WALL);
        }
    }
}

// 블록 칸 제거

void BlockWall::clearCells(Board& board, const int onlyIfCell) {
    // 지정한 셀 값인 칸만 EMPTY로 뱀/아이템이 덮은 칸은 건드리지 않음
    for (int i = 0; i < cellCount; i++) {
        if (board.getCell(cells[i].y, cells[i].x) == onlyIfCell) {
            board.setCell(cells[i].y, cells[i].x, EMPTY);
        }
    }
}

// 매 tick 업데이트

void BlockWall::update(Board& board, const Snake& snake) {
    timer--;
    if (timer > 0) return;

    if (phase == 0) {
        // 대기 끝 -> 예고 배치 시도
        if (trySpawnWarn(board, snake)) {
            phase = 1;
            timer = WARN_DURATION;
        } else {
            timer = 10; // 자리를 못 찾았으면 잠깐 뒤 재시도
        }
    } else if (phase == 1) {
        // 예고 끝 -> 실제 벽으로 굳힘
        hardenToWall(board);
        phase = 2;
        timer = ALIVE_DURATION;
    } else {
        // 출현 끝 ->  블록 제거 후 다시 대기
        clearCells(board, BLOCK_WALL);
        phase = 0;
        timer = RESPAWN_WAIT;
    }
}
