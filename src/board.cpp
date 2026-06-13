// board.cpp
// 맵 데이터를 보관하고, 파일에서 읽고, 화면에 그리는 Board 클래스 구현

#include "board.h"
#include "curses_compat.h" // OS별 curses 호환 레이어
#include <fstream>
#include <string>

// 생성자: 맵 전체를 빈칸(0)으로 초기화
Board::Board()
{
  height = 0;
  width = 0;
  for (int y = 0; y < MAP_MAX_SIZE; y++)
  {
    for (int x = 0; x < MAP_MAX_SIZE; x++)
    {
      data[y][x] = EMPTY;
    }
  }
}

// 텍스트 파일에서 맵 읽기
// 파일 형식: 첫 줄에 "행 열", 그 다음부터 한 줄에 한 행씩 숫자 나열
bool Board::loadFromFile(const char * const filename)
{
  std::ifstream fin(filename);
  if (fin.is_open() == false)
  {
    return false;
  }

  // 첫 줄: 맵 크기
  fin >> height >> width;

  // 크기 검사 (명세서 요구: 최소 21x21)
  if (height < MAP_MIN_SIZE || width < MAP_MIN_SIZE)
    return false;
  if (height > MAP_MAX_SIZE || width > MAP_MAX_SIZE)
    return false;

  // 한 줄씩 읽어서 각 글자를 정수로 바꿔서 저장
  int rowsRead = 0;
  for (int y = 0; y < height; y++)
  {
    std::string line;
    if (!(fin >> line)) break; // 읽기 실패 시 중단
    for (int x = 0; x < width; x++)
    {
      if (x < (int)line.length()) {
        // '0' ~ '9' 문자를 정수로 변환
        data[y][x] = line[x] - '0';
      } else {
        data[y][x] = EMPTY;
      }
    }
    rowsRead++;
  }

  // 실제로 읽은 행 수로 height 를 보정한다.
  // (헤더의 행 수가 실제 데이터보다 크면, 보이는 맵 아래에 빈 행이 생겨 그 자리에 아이템/블록 벽이 스폰되는 버그가 생기므로 방지)
  height = rowsRead;
  return true;
}

// 한 칸 값 읽기. 범위를 벗어나면 WALL 로 처리해서 충돌 검사가 안전하게 됨
int Board::getCell(const int y, const int x) const
{
  if (y < 0 || y >= height)
    return WALL;
  if (x < 0 || x >= width)
    return WALL;
  return data[y][x];
}

// 한 칸 값 바꾸기
void Board::setCell(const int y, const int x, const int value)
{
  if (y < 0 || y >= height)
    return;
  if (x < 0 || x >= width)
    return;
  data[y][x] = value;
}

// 5단계 - 벽 개수 파악
// 가장자리인 IMMUNE_WALL(2)은 제외하고 게이트가 될 수 있는 WALL(1)만 카운트
int Board::countInternalWalls() const
{
  int count = 0;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      if (data[y][x] == WALL)
      {
        count++;
      }
    }
  }
  return count;
}

// ncurses 색 페어 초기화. 게임 시작할 때 한 번만 부르면 됨.
void Board::initColors() const
{
  // 맵 요소 드로잉용
  // 오리지널과 동일하게 글자색과 배경색을 통일하여 꽉 찬 블록으로 표현
  init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
  init_pair(IMMUNE_WALL, COLOR_WHITE, COLOR_WHITE);
  init_pair(SNAKE_HEAD, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(SNAKE_BODY, COLOR_RED, COLOR_RED);
  init_pair(GROWTH_ITEM, COLOR_GREEN, COLOR_GREEN);
  init_pair(POISON_ITEM, COLOR_RED, COLOR_RED);
  init_pair(GATE, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(SPEED_ITEM, COLOR_CYAN, COLOR_CYAN);
  init_pair(USED_GATE_WALL, COLOR_YELLOW, COLOR_YELLOW);
  // 스코어보드 및 각종 메뉴 텍스트 출력용
  init_pair(COLOR_PAIR_TEXT_GROWTH, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_PAIR_TEXT_POISON, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_PAIR_TEXT_SPEED, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_PAIR_TEXT_GATE, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_PAIR_TEXT_USED_GATE, COLOR_YELLOW, COLOR_BLACK);

  // 테트리스 블록 벽
  // 굳은 벽은 회색, 출현 1초 전 예고는 빨강
  init_pair(BLOCK_WALL, COLOR_WHITE, COLOR_WHITE);
  init_pair(BLOCK_WARN, COLOR_RED, COLOR_RED);
}

// 맵 전체를 화면에 그리기
// (offsetY, offsetX) 부터 시작해서, 한 칸은 화면에서 두 글자 폭으로 표시
void Board::draw(const int offsetY, const int offsetX) const
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      const int cell = data[y][x];
      const int screenY = offsetY + y;
      const int screenX = offsetX + x * CELL_WIDTH;

      if (cell == EMPTY)
      {
        // 빈칸은 공백 두 글자
        mvprintw(screenY, screenX, "  ");
      }
      else
      {
        attron(COLOR_PAIR(cell));
        mvprintw(screenY, screenX, "  ");
        attroff(COLOR_PAIR(cell));
      }
    }
  }
}
