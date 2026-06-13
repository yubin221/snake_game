// speeditem.cpp
// SpeedItem 클래스 구현 - 속도 아이템의 출현/수명/획득 및 속도 효과 처리

#include "speeditem.h"
#include "board.h"
#include <cstdlib>
#include <vector>

// 아이템이 맵 위에 머무는 기본 수명 (tick)
static const int ITEM_LIFETIME = 30;
// Speed 효과 지속 시간 (tick)
static const int SPEED_EFFECT_DURATION = 30;

SpeedItem::SpeedItem()
    : y(0), x(0), active(false), timer(0),
      effectTimer(0), tickCounter(0), wasOnMap(false)
{
}

void SpeedItem::spawn(Board &board)
{
    std::vector<Position> empties;
    const int H = board.getHeight();
    const int W = board.getWidth();

    // 맵 전체를 돌며 빈 칸 수집
    for (int cy = 0; cy < H; cy++)
    {
        for (int cx = 0; cx < W; cx++)
        {
            if (board.getCell(cy, cx) == EMPTY)
            {
                const Position p = {cy, cx};
                empties.push_back(p);
            }
        }
    }
    if (empties.empty())
        return;

    const int idx = rand() % (int)empties.size();
    y = empties[idx].y;
    x = empties[idx].x;
    active = true;
    timer = ITEM_LIFETIME;
    board.setCell(y, x, SPEED_ITEM);
}

void SpeedItem::update(Board &board)
{
    if (active == false)
    {
        spawn(board);
        return;
    }

    if (board.getCell(y, x) != SPEED_ITEM)
    {
        spawn(board);
        return;
    }

    timer--;
    if (timer <= 0)
    {
        board.setCell(y, x, EMPTY);
        spawn(board);
    }
}

void SpeedItem::prepareSpeed(const Board &board)
{
    wasOnMap = active && (board.getCell(y, x) == SPEED_ITEM);
}

bool SpeedItem::updateSpeed(Board &board)
{
    tickCounter++;
    const bool shouldUpdate = (effectTimer <= 0) || (tickCounter % 2 == 0);

    if (shouldUpdate)
    {
        if (wasOnMap && board.getCell(y, x) != SPEED_ITEM)
        {
            effectTimer = SPEED_EFFECT_DURATION;
        }
        update(board);
    }

    if (effectTimer > 0)
    {
        effectTimer--;
    }

    return shouldUpdate;
}

bool SpeedItem::isSpeedActive() const
{
    return effectTimer > 0;
}

void SpeedItem::reset()
{
    y = 0;
    x = 0;
    active = false;
    timer = 0;
    effectTimer = 0;
    tickCounter = 0;
    wasOnMap = false;
}
