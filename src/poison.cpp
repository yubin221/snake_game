// poison.cpp
// Poison 클래스 구현 - 독 아이템의 출현/수명/획득 처리

#include "poison.h"
#include "board.h"
#include <cstdlib>
#include <vector>

// 아이템이 맵 위에 머무는 기본 수명 (tick)
static const int ITEM_LIFETIME = 30;

Poison::Poison()
    : y(0), x(0), active(false), timer(0)
{
}

void Poison::spawn(Board &board)
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
    board.setCell(y, x, POISON_ITEM);
}

void Poison::update(Board &board)
{
    if (active == false)
    {
        spawn(board);
        return;
    }

    if (board.getCell(y, x) != POISON_ITEM)
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

void Poison::reset()
{
    y = 0;
    x = 0;
    active = false;
    timer = 0;
}
