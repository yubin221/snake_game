// ScoreBoard.cpp
// 스코어보드 구현부

#include "ScoreBoard.h"
#include "curses_compat.h"
#include <cstdlib>
#include <locale.h>

ScoreBoard::ScoreBoard(const int stageNum, const int totalInternalWalls)
{
    stage = stageNum;

    currentLength = 0;
    maxLength = 0;
    growthCount = 0;
    poisonCount = 0;
    speedCount = 0;
    gateCount = 0;

    // 현재 내부 벽 개수 초기화 추가
    currentInternalWalls = totalInternalWalls;

    missionLength = false;
    missionGrowth = false;
    missionPoison = false;
    missionSpeed = false;
    missionGate = false;

    // 스테이지 초기화 시 미션 자동 생성
    generateMissions(totalInternalWalls);
}

void ScoreBoard::generateMissions(const int totalInternalWalls)
{
    // 스테이지가 높아질수록 목표치가 커지도록 설정
    const int base = stage * 2;

    targetLength = base + (rand() % 3) + 3;        // 스테이지 1: 5~7, 스테이지 4: 11~13
    targetGrowth = base + (rand() % 3);            // 스테이지 1: 2~4, 스테이지 4: 8~10
    targetPoison = (stage / 2) + (rand() % 2) + 1; // 독 아이템 목표
    targetSpeed = base / 2 + (rand() % 2) + 1;     // 스피드 아이템 목표

    // Gate 미션: 통과한 게이트의 개수를 미션으로 사용하되 전체 벽의 30% 이하로 난이도 조절
    int maxGate = totalInternalWalls * 0.3;
    if (maxGate < 1)
        maxGate = 1; // 최소 1번은 통과해야 함

    int passes = (rand() % maxGate) + 1;
    // 너무 많아지지 않도록 스테이지 보정 (스테이지당 1~2회 추가 목표)
    if (passes > stage + 1)
        passes = stage + 1;

    targetGate = passes;
}

void ScoreBoard::addGrowth()
{
    growthCount++;
    checkMissions();
}

void ScoreBoard::addPoison()
{
    poisonCount++;
    checkMissions();
}

void ScoreBoard::addSpeed()
{
    speedCount++;
    checkMissions();
}

// 게이트 통과 횟수 증가
void ScoreBoard::addGate()
{
    gateCount++;
    checkMissions();
}

// 게이트 미션이 달성 가능한지 판단하는 로직
bool ScoreBoard::canCompleteGateMission(const bool isGateActive) const
{
    if (missionGate)
        return true; // 이미 달성했으면 OK

    // 앞으로 생성 가능한 게이트 쌍의 수 = (남은 벽 / 2) + (현재 떠있는 게이트 ? 1 : 0)
    const int potentialGates = (currentInternalWalls / 2) + (isGateActive ? 1 : 0);
    const int needed = targetGate - gateCount;

    return potentialGates >= needed;
}

void ScoreBoard::updateLength(const int len)
{
    currentLength = len;
    if (currentLength > maxLength)
        maxLength = currentLength;
    checkMissions();
}

void ScoreBoard::checkMissions()
{
    // 매번 현재 값으로 재평가한다(양방향)
    // 길이(Length)는 Poison 등으로 줄어들 수 있으므로, 목표 미달이면
    // 달성 표시도 해제되어야 한다. 
    // Growth/Poison/Speed/Gate 는 누적값이라 줄지 않으므로 사실상 한 번 달성하면 유지
    missionLength = (currentLength >= targetLength);
    missionGrowth = (growthCount >= targetGrowth);
    missionPoison = (poisonCount >= targetPoison);
    missionSpeed  = (speedCount >= targetSpeed);
    missionGate   = (gateCount >= targetGate);
}

bool ScoreBoard::isAllMissionComplete() const
{
    return missionLength && missionGrowth && missionPoison && missionSpeed && missionGate;
}

void ScoreBoard::draw(const int offsetY, const int offsetX) const
{
    // Draw Box for Score Board
    // Header (No emojis)
    attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));
    mvprintw(offsetY, offsetX, "╔══════════════════════════╗");
    mvprintw(offsetY + 1, offsetX, "║    STAGE %d SCOREBOARD    ║", stage);
    mvprintw(offsetY + 2, offsetX, "╠══════════════════════════╣");
    attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));

    // Stats
    mvprintw(offsetY + 3, offsetX, "║  Length  : %2d / %-2d       ║", currentLength, maxLength);

    // Growth (Green Box)
    mvprintw(offsetY + 4, offsetX, "║  ");
    attron(COLOR_PAIR(GROWTH_ITEM));
    mvprintw(offsetY + 4, offsetX + 3, "  ");
    attroff(COLOR_PAIR(GROWTH_ITEM));
    mvprintw(offsetY + 4, offsetX + 5, " Growth : %-2d          ║", growthCount);

    // Poison (Red Box)
    mvprintw(offsetY + 5, offsetX, "║  ");
    attron(COLOR_PAIR(POISON_ITEM));
    mvprintw(offsetY + 5, offsetX + 3, "  ");
    attroff(COLOR_PAIR(POISON_ITEM));
    mvprintw(offsetY + 5, offsetX + 5, " Poison : %-2d          ║", poisonCount);

    // Speed (Cyan Box)
    mvprintw(offsetY + 6, offsetX, "║  ");
    attron(COLOR_PAIR(SPEED_ITEM));
    mvprintw(offsetY + 6, offsetX + 3, "  ");
    attroff(COLOR_PAIR(SPEED_ITEM));
    mvprintw(offsetY + 6, offsetX + 5, " Speed  : %-2d          ║", speedCount);

    // Gate (Magenta Box)
    mvprintw(offsetY + 7, offsetX, "║  ");
    attron(COLOR_PAIR(GATE));
    mvprintw(offsetY + 7, offsetX + 3, "  ");
    attroff(COLOR_PAIR(GATE));
    mvprintw(offsetY + 7, offsetX + 5, " Gate   : %-2d (Max:%2d) ║", gateCount, currentInternalWalls / 2);
    
    mvprintw(offsetY + 8, offsetX, "╚══════════════════════════╝");

    // Draw Box for Missions
    const int mOffset = offsetY + 10;
    attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE));
    mvprintw(mOffset, offsetX, "╔══════════════════════════╗");
    mvprintw(mOffset + 1, offsetX, "║         MISSIONS         ║");
    mvprintw(mOffset + 2, offsetX, "╠══════════════════════════╣");
    attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE));

    auto printMissionRow = [&](const int row, const int cellType, const char* const labelStr, const int current, const int target, const bool complete) {
        mvprintw(mOffset + row, offsetX, "║  ");
        if (cellType > 0) {
            attron(COLOR_PAIR(cellType));
            mvprintw(mOffset + row, offsetX + 3, "  ");
            attroff(COLOR_PAIR(cellType));
            mvprintw(mOffset + row, offsetX + 5, " %s : %2d / %-2d", labelStr, current, target);
        } else {
            mvprintw(mOffset + row, offsetX + 3, "Length : %2d / %-2d   ", current, target);
        }

        if (complete) {
            attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
            mvprintw(mOffset + row, offsetX + 23, "[V]");
            attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
        } else {
            attron(COLOR_PAIR(COLOR_PAIR_TEXT_POISON));
            mvprintw(mOffset + row, offsetX + 23, "[ ]");
            attroff(COLOR_PAIR(COLOR_PAIR_TEXT_POISON));
        }
        mvprintw(mOffset + row, offsetX + 27, "║");
    };

    printMissionRow(3, 0, "Length", currentLength, targetLength, missionLength);
    printMissionRow(4, GROWTH_ITEM, "Growth", growthCount, targetGrowth, missionGrowth);
    printMissionRow(5, POISON_ITEM, "Poison", poisonCount, targetPoison, missionPoison);
    printMissionRow(6, SPEED_ITEM, "Speed ", speedCount, targetSpeed, missionSpeed);
    printMissionRow(7, GATE, "Gate  ", gateCount, targetGate, missionGate);
    mvprintw(mOffset + 8, offsetX, "╚══════════════════════════╝");

    // Draw Box for Controls/Help
    const int hOffset = mOffset + 10;
    attron(A_DIM);
    mvprintw(hOffset, offsetX, "╔══════════════════════════╗");
    mvprintw(hOffset + 1, offsetX, "║      조작법 및 정보      ║");
    mvprintw(hOffset + 2, offsetX, "╠══════════════════════════╣");
    mvprintw(hOffset + 3, offsetX, "║  방향키: 스네이크 이동   ║");
    mvprintw(hOffset + 4, offsetX, "║  Q 키  : 스테이지 종료   ║");
    mvprintw(hOffset + 5, offsetX, "║                          ║");
    mvprintw(hOffset + 6, offsetX, "║  ! 충돌 또는 길이 3 미만 ║");
    mvprintw(hOffset + 7, offsetX, "║    시 게임오버가 됩니다  ║");
    mvprintw(hOffset + 8, offsetX, "╚══════════════════════════╝");
    attroff(A_DIM);
}