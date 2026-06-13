// GameController.cpp
// GameController 구현 - 게임 흐름 전체를 담당한다.

#include "GameController.h"
#include "curses_compat.h"
#include <cstdlib>

static const int TICK_USEC = 200000;

// UTF-8 문자열의 시각적 셀 너비를 구하는 헬퍼 함수
static int getUtf8VisualWidth(const std::string &str)
{
    int width = 0;
    for (size_t i = 0; i < str.length();)
    {
        const unsigned char c = str[i];
        if (c < 0x80)
        {
            width += 1;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            width += 2;
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        { // 한글 및 <-, -> 등
            width += 2;
            i += 3;
        }
        else
        {
            i += 1;
        }
    }
    return width;
}

// 생성자 -> 스테이지 번호와 맵 파일 경로를 받아 멤버를 초기화 리스트로 설정
// 실제 맵 로드, 뱀 배치는 initialize() 에서 수행
GameController::GameController(const int stageNum, const std::string &mapFilePath)
    : stageNum(stageNum), mapFilePath(mapFilePath), scoreBoard(stageNum, 0),
      growthItem(), poisonItem(), speedItem(),
      isRunning(false), gameOver(false), stageClear(false), userQuit(false) {}

// 스테이지 시작 준비 -> 맵 로드, 색 초기화, 뱀 배치, 스코어보드/아이템/블록벽
// 초기화 및 상태 플래그 리셋. 맵 로드나 뱀 배치 실패 시 false 반환
bool GameController::initialize()
{
    if (board.loadFromFile(mapFilePath.c_str()) == false)
    {
        return false;
    }
    board.initColors();

    if (snake.initFromBoard(board) == false)
    {
        return false;
    }

    // 맵에서 내부 벽 개수를 세어 스코어보드 재생성 및 동기화
    scoreBoard = ScoreBoard(stageNum, board.countInternalWalls());
    scoreBoard.updateLength(snake.getLength());

    growthItem.reset();
    poisonItem.reset();
    speedItem.reset();
    blockWall = BlockWall();

    isRunning = true;
    gameOver = false;
    stageClear = false;
    userQuit = false;

    // 비동기 게임 루프용 폴링으로 확실히 복원
    nodelay(stdscr, TRUE);

    return true;
}

// 단일 스테이지의 메인 루프 -> 렌더 -> 입력 -> 갱신을 반복
// 반환: 사용자가 q 종료=QUIT, 충돌/실패=GAME_OVER, 미션 전부 달성=STAGE_CLEAR
GameResult GameController::run()
{
    while (isRunning && !gameOver && !stageClear)
    {
        render();
        waitAndProcessInput();
        if (userQuit)
        {
            return GameResult::QUIT;
        }
        update();
    }

    if (gameOver)
    {
        return GameResult::GAME_OVER;
    }
    return GameResult::STAGE_CLEAR;
}

// 한 tick 동안 키 입력을 폴링하며 대기
//   - 방향키: 다음 진행 방향 예약 / q: 종료
//   - Speed 효과 중에는 대기 시간을 절반으로 줄여 2배 빠르게 진행
void GameController::waitAndProcessInput()
{
    const int waitTime = speedItem.isSpeedActive() ? TICK_USEC / 2 : TICK_USEC;
    int elapsed = 0;
    const int pollInterval = 10000; // 10ms 단위 폴링

    while (elapsed < waitTime)
    {
        const int key = getch();
        if (key != ERR)
        {
            if (key == 'q' || key == 'Q')
            {
                isRunning = false;
                userQuit = true;
                break;
            }
            else if (key == KEY_UP)
                snake.requestDirection(DIR_UP);
            else if (key == KEY_DOWN)
                snake.requestDirection(DIR_DOWN);
            else if (key == KEY_LEFT)
                snake.requestDirection(DIR_LEFT);
            else if (key == KEY_RIGHT)
                snake.requestDirection(DIR_RIGHT);
        }
        sleep_usec(pollInterval);
        elapsed += pollInterval;
    }
}

// 한 tick의 상태 갱신 
// 1. 미션 달성 가능성 사전 점검(불가 시 게임오버)
// 2. Gate,블록벽 갱신 -> 뱀 이동 -> 충돌(-1)/아이템 획득 처리
// 3. 점수,아이템 갱신 후 모든 미션 달성 시 스테이지 클리어
void GameController::update()
{
    // 뱀의 이동 직전, 속도 아이템 수명 및 상태 정보의 스냅샷 준비
    speedItem.prepareSpeed(board);

    // 미션 가능 여부 실시간 체크
    scoreBoard.setInternalWalls(board.countInternalWalls());
    if (!scoreBoard.canCompleteGateMission(gate.isActive()))
    {
        gameOver = true;
        clear();

        // 상단 헤더: 작은 로고 및 스테이지 정보 표시 
        const int totalWidth = board.getWidth() * CELL_WIDTH + 4 + 28;

        // 상단 테두리 상자
        attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);
        mvprintw(0, 0, "╔");
        for (int x = 1; x < totalWidth - 1; x++)
        {
            mvprintw(0, x, "═");
        }
        mvprintw(0, totalWidth - 1, "╗");

        mvprintw(1, 0, "║");
        for (int x = 1; x < totalWidth - 1; x++)
        {
            mvprintw(1, x, " ");
        }
        mvprintw(1, totalWidth - 1, "║");

        mvprintw(2, 0, "╚");
        for (int x = 1; x < totalWidth - 1; x++)
        {
            mvprintw(2, x, "═");
        }
        mvprintw(2, totalWidth - 1, "╝");
        attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);

        // 텍스트 출력
        attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
        mvprintw(1, 2, "SNAKE GAME");
        attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);

        attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);
        mvprintw(1, totalWidth - 14, "[ STAGE %d ]", stageNum);
        attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);

        board.draw(3, 0);
        scoreBoard.draw(3, board.getWidth() * CELL_WIDTH + 4);
        mvprintw(3 + board.getHeight() + 1, 0, "Mission Impossible! 게이트 생성을 위한 벽이 부족합니다.");
        refresh();
        sleep_usec(1500000); // 사용자가 문구를 읽을 수 있도록 1.5초 대기
        return;
    }

    const int prevGateUseCount = gate.getUseCount();
    gate.update(board, snake);
    blockWall.update(board, snake);

    // 뱀 한 칸 이동 시도
    const int target = snake.move(board, &gate);

    if (gate.getUseCount() > prevGateUseCount)
    {
        scoreBoard.addGate();
    }

    if (target == -1)
    {
        gameOver = true;
    }
    else
    {
        if (target == GROWTH_ITEM)
            scoreBoard.addGrowth();
        else if (target == POISON_ITEM)
            scoreBoard.addPoison();
        else if (target == SPEED_ITEM)
            scoreBoard.addSpeed();

        scoreBoard.updateLength(snake.getLength());

        // 아이템 틱 갱신 및 스폰 진행
        const bool shouldUpdate = speedItem.updateSpeed(board);
        if (shouldUpdate)
        {
            growthItem.update(board);
            poisonItem.update(board);
        }

        if (scoreBoard.isAllMissionComplete())
        {
            stageClear = true;
        }
    }
}

// 화면 렌더링 - 상단 헤더 박스(SNAKE GAME / STAGE n)와 맵, 스코어보드를 그림
// 상태를 바꾸지 않으므로 const 멤버 함수
void GameController::render() const
{
    clear();

    const int totalWidth = board.getWidth() * CELL_WIDTH + 4 + 28;

    // 상단 테두리 상자
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);
    mvprintw(0, 0, "╔");
    for (int x = 1; x < totalWidth - 1; x++)
    {
        mvprintw(0, x, "═");
    }
    mvprintw(0, totalWidth - 1, "╗");

    mvprintw(1, 0, "║");
    for (int x = 1; x < totalWidth - 1; x++)
    {
        mvprintw(1, x, " ");
    }
    mvprintw(1, totalWidth - 1, "║");

    mvprintw(2, 0, "╚");
    for (int x = 1; x < totalWidth - 1; x++)
    {
        mvprintw(2, x, "═");
    }
    mvprintw(2, totalWidth - 1, "╝");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);

    // 텍스트 출력
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
    mvprintw(1, 2, "SNAKE GAME");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);

    attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);
    mvprintw(1, totalWidth - 14, "[ STAGE %d ]", stageNum);
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);

    // 맵과 스코어보드를 Y=3 위치부터 그리도록 아래로 이동
    board.draw(3, 0);
    scoreBoard.draw(3, board.getWidth() * CELL_WIDTH + 4);
    refresh();
}

// 비주얼 다듬기: 인트로, 규칙 설명, 랭킹 연동화면 메서드 구현

bool GameController::showIntroScreen(RankingManager &rankingManager)
{
    nodelay(stdscr, FALSE); // 동기식 키 입력으로 일시 전환
    bool needRedraw = true;
    while (true)
    {
        if (needRedraw)
        {
            clear();
            int H, W;
            getmaxyx(stdscr, H, W);

            const int startY = (H > 22) ? (H - 22) / 2 : 0;
            const int startX = (W > 82) ? (W - 82) / 2 : 0;

            // ASCII Art 로고 출력 녹색 테마
            attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
            mvprintw(startY, startX, "  ██████  ███    ██  █████  ██   ██ ███████      ██████   █████  ███    ███ ███████");
            mvprintw(startY + 1, startX, " ██       ████   ██ ██   ██ ██  ██  ██          ██       ██   ██ ████  ████ ██     ");
            mvprintw(startY + 2, startX, "  █████   ██ ██  ██ ███████ █████   █████       ██   ███ ███████ ██ ████ ██ █████  ");
            mvprintw(startY + 3, startX, "      ██  ██  ██ ██ ██   ██ ██  ██  ██          ██    ██ ██   ██ ██  ██  ██ ██     ");
            mvprintw(startY + 4, startX, " ██████   ██   ████ ██   ██ ██   ██ ███████      ██████  ██   ██ ██      ██ ███████");
            attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);

            // 메뉴 프레임 상자
            const int boxX = startX + 11;
            const int boxY = startY + 7;

            attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);
            mvprintw(boxY, boxX, "╔══════════════════════════════════════════════════════════╗");
            mvprintw(boxY + 1, boxX, "║                     MAIN MENU SELECT                     ║");
            mvprintw(boxY + 2, boxX, "╠══════════════════════════════════════════════════════════╣");
            attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD);

            mvprintw(boxY + 3, boxX, "║  [ A ] 키를 눌러 게임 시작                               ║");
            mvprintw(boxY + 4, boxX, "║  [ B ] 키를 눌러 랭킹 확인                               ║");
            mvprintw(boxY + 5, boxX, "║  [ ? ] 키를 눌러 도움말 및 조작법                        ║");
            mvprintw(boxY + 6, boxX, "║  [ Q ] 키를 눌러 게임 종료                               ║");
            mvprintw(boxY + 7, boxX, "╚══════════════════════════════════════════════════════════╝");

            attron(A_DIM);
            mvprintw(boxY + 9, boxX + 6, "2026 C++ Snake Game Project - 1조");
            attroff(A_DIM);

            refresh();
            needRedraw = false;
        }

        const int key = getch();
        if (key == ERR)
        {
            sleep_usec(10000); // 10ms 대기
            continue;
        }
        if (key == 'a' || key == 'A')
        {
            nodelay(stdscr, TRUE); // 다시 비동기 게임 루프용 폴링으로 원복
            return true;
        }
        else if (key == 'q' || key == 'Q')
        {
            return false;
        }
        else if (key == 'b' || key == 'B')
        {
            showRankingBoardScreen(rankingManager, 0, "[← / →] 방향키를 입력해 다른 스테이지의 랭킹을 확인할 수 있습니다. (돌아가기: [ A ])", true);
            needRedraw = true;
        }
        else if (key == '?' || key == 'h' || key == 'H')
        {
            showHelpScreen();
            needRedraw = true;
        }
        else if (key == KEY_RESIZE)
        {
            needRedraw = true;
        }
    }
}

void GameController::showHelpScreen()
{
    nodelay(stdscr, FALSE);
    clear();
    int H, W;
    getmaxyx(stdscr, H, W);
    (void)H;

    const int startX = (W > 70) ? (W - 70) / 2 : 0;
    const int startY = 1;

    attron(COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE) | A_BOLD);
    mvprintw(startY, startX, "╔════════════════════════════════════════════════════════════════════╗");
    mvprintw(startY + 1, startX, "║                        HELP & GAME CONTROLS                        ║");
    mvprintw(startY + 2, startX, "╠════════════════════════════════════════════════════════════════════╣");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE) | A_BOLD);

    mvprintw(startY + 3, startX, "║  - 조작법                                                          ║");
    mvprintw(startY + 4, startX, "║    - 방향키 [ ↑ | ↓ | ← | → ] : 뱀 이동 방향 전환                  ║");
    mvprintw(startY + 5, startX, "║    - [ Q ] 키 : 게임 중 강제 종료 및 이전 화면 이동                ║");
    mvprintw(startY + 6, startX, "║                                                                    ║");

    mvprintw(startY + 7, startX, "║  - 게임 아이템 종류                                                ║");

    // Growth Item
    mvprintw(startY + 8, startX, "║    - ");
    attron(COLOR_PAIR(GROWTH_ITEM));
    printw("  ");
    attroff(COLOR_PAIR(GROWTH_ITEM));
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH));
    printw(" Growth Item (초록색) : 뱀의 몸통 길이 1 증가             ");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH));
    mvprintw(startY + 8, startX + 69, "║");

    // Poison Item
    mvprintw(startY + 9, startX, "║    - ");
    attron(COLOR_PAIR(POISON_ITEM));
    printw("  ");
    attroff(COLOR_PAIR(POISON_ITEM));
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_POISON));
    printw(" Poison Item (빨간색) : 뱀의 몸통 길이 1 감소             ");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_POISON));
    mvprintw(startY + 9, startX + 69, "║");

    // Speed Item
    mvprintw(startY + 10, startX, "║    - ");
    attron(COLOR_PAIR(SPEED_ITEM));
    printw("  ");
    attroff(COLOR_PAIR(SPEED_ITEM));
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));
    printw(" Speed Item  (시안색) : 뱀의 이동속도 2배 (30틱간 지속)   ");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));
    mvprintw(startY + 10, startX + 69, "║");

    // Warp Gate
    mvprintw(startY + 11, startX, "║    - ");
    attron(COLOR_PAIR(GATE));
    printw("  ");
    attroff(COLOR_PAIR(GATE));
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_GATE));
    printw(" Warp Gate   (자홍색) : 머리 진입 시 반대편 게이트 워프   ");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GATE));
    mvprintw(startY + 11, startX + 69, "║");

    // Used Wall
    mvprintw(startY + 12, startX, "║    - ");
    attron(COLOR_PAIR(USED_GATE_WALL));
    printw("  ");
    attroff(COLOR_PAIR(USED_GATE_WALL));
    attron(COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE));
    printw(" Used Wall   (노란색) : 한 번 사용된 워프 흔적 (재이용 불가)");
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE));
    mvprintw(startY + 12, startX + 69, "║");

    mvprintw(startY + 13, startX, "║                                                                    ║");
    mvprintw(startY + 14, startX, "║  - 게임 규칙 및 실패 조건                                          ║");
    mvprintw(startY + 15, startX, "║    - 각 스테이지별 우측 미션 목표를 전부 완수 시 클리어            ║");
    mvprintw(startY + 16, startX, "║    - 뱀의 머리가 벽에 충돌 시 실패                                 ║");
    mvprintw(startY + 17, startX, "║    - 독약을 먹고 뱀의 몸통 길이가 3 미만이 될 시 실패 (게임 오버)  ║");
    mvprintw(startY + 18, startX, "║    - 맵 내 남은 벽이 부족해져 게이트 미션 달성이 불가할 시 실패    ║");
    mvprintw(startY + 19, startX, "╚════════════════════════════════════════════════════════════════════╝");

    attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));
    mvprintw(startY + 21, startX + 14, "[ A ] 키를 누르면 이전 시작화면으로 이동합니다.");
    attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));

    refresh();

    while (true)
    {
        const int key = getch();
        if (key == ERR)
        {
            sleep_usec(10000); // 10ms 대기
            continue;
        }
        if (key == 'a' || key == 'A')
        {
            break;
        }
    }
}

bool GameController::showRankingBoardScreen(const RankingManager &rankingManager, const int initialStage, const std::string &bottomMessage, const bool allowSwitch)
{
    nodelay(stdscr, FALSE);
    int stageFilter = initialStage; // 0 = 통합 랭킹, 1~5 = 개별 스테이지
    bool needRedraw = true;

    while (true)
    {
        if (needRedraw)
        {
            const auto ranks = rankingManager.getRankings(stageFilter);
            drawRankingTable(ranks, stageFilter, bottomMessage);
            needRedraw = false;
        }

        const int key = getch();
        if (key == ERR)
        {
            sleep_usec(10000); // 10ms 대기
            continue;
        }
        if (key == 'a' || key == 'A')
        {
            return true;
        }
        else if (key == 'q' || key == 'Q')
        {
            return false;
        }
        else if (allowSwitch)
        {
            if (key == KEY_LEFT)
            {
                stageFilter--;
                if (stageFilter < 0)
                    stageFilter = 5;
                needRedraw = true;
            }
            else if (key == KEY_RIGHT)
            {
                stageFilter++;
                if (stageFilter > 5)
                    stageFilter = 0;
                needRedraw = true;
            }
        }
        else if (key == KEY_RESIZE)
        {
            needRedraw = true;
        }
    }
}

void GameController::drawRankingTable(const std::vector<RankingRecord> &ranks, const int stageFilter, const std::string &bottomMessage)
{
    clear();
    int H, W;
    getmaxyx(stdscr, H, W);
    (void)H;

    // 표 전체 너비 88칸
    const int boxWidth = 88;
    const int startX = (W > boxWidth) ? (W - boxWidth) / 2 : 0;
    const int startY = 1;

    attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));
    mvprintw(startY, startX, "╔═══════════════════════════════════════════════════════════════════════════════════════╗");
    if (stageFilter == 0)
    {
        mvprintw(startY + 1, startX, "║                               ALL STAGES RANKING BOARD                               ║");
    }
    else
    {
        mvprintw(startY + 1, startX, "║                                STAGE %d RANKING BOARD                                 ║", stageFilter);
    }
    mvprintw(startY + 2, startX, "╠═══════════════════════════════════════════════════════════════════════════════════════╣");
    attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED));

    // 테이블 헤더
    attron(A_BOLD);
    mvprintw(startY + 3, startX, "║ 순위 │  단계  │ 최대길이 │ Growth │ Poison │ Speed │ Gate통과 │      플레이 일시      ║");
    mvprintw(startY + 4, startX, "╠══════╪════════╪══════════╪════════╪════════╪═══════╪══════════╪═══════════════════════╣");
    attroff(A_BOLD);

    // 최대 11개 행 표출 
    // 10등 내 순위 + 방금 플레이한 외각 순위 포함
    for (int i = 0; i < 11; i++)
    {
        const int rowY = startY + 5 + i;
        if (i < (int)ranks.size())
        {
            const auto &r = ranks[i];

            if (r.isCurrentPlay)
            {
                attron(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
                if (r.rank <= 10)
                {
                    mvprintw(rowY, startX, "║->%2d위│  %d단계 │    %2d    │   %2d   │   %2d   │  %2d   │    %2d    │  %s  ║",
                             r.rank, r.stage, r.maxLength, r.growthCount, r.poisonCount, r.speedCount, r.gateCount, r.timestamp.c_str());
                }
                else
                {
                    mvprintw(rowY, startX, "║내기록│  %d단계 │    %2d    │   %2d   │   %2d   │  %2d   │    %2d    │  %s  ║",
                             r.stage, r.maxLength, r.growthCount, r.poisonCount, r.speedCount, r.gateCount, r.timestamp.c_str());
                }
                attroff(COLOR_PAIR(COLOR_PAIR_TEXT_GROWTH) | A_BOLD);
            }
            else
            {
                // 상위 3등 특별 색상 강조
                if (r.rank == 1)
                {
                    attron(COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE) | A_BOLD); // 금빛
                }
                else if (r.rank == 2)
                {
                    attron(COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | A_BOLD); // 은빛
                }
                else if (r.rank == 3)
                {
                    attron(COLOR_PAIR(COLOR_PAIR_TEXT_GATE) | A_BOLD); // 동빛
                }

                mvprintw(rowY, startX, "║  %2d위│  %d단계 │    %2d    │   %2d   │   %2d   │  %2d   │    %2d    │  %s  ║",
                         r.rank, r.stage, r.maxLength, r.growthCount, r.poisonCount, r.speedCount, r.gateCount, r.timestamp.c_str());

                if (r.rank <= 3)
                {
                    attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_TEXT_USED_GATE) | COLOR_PAIR(COLOR_PAIR_TEXT_SPEED) | COLOR_PAIR(COLOR_PAIR_TEXT_GATE));
                }
            }
        }
        else
        {
            // 빈 공간 채워넣기
            mvprintw(rowY, startX, "║      │        │          │        │        │       │          │                       ║");
        }
    }

    mvprintw(startY + 16, startX, "╚═══════════════════════════════════════════════════════════════════════════════════════╝");

    // 하단 정보 메시지
    attron(A_BOLD);
    const int visualWidth = getUtf8VisualWidth(bottomMessage);
    int textX = startX + (boxWidth - visualWidth) / 2;
    if (textX < startX)
        textX = startX;
    mvprintw(startY + 18, textX, "%s", bottomMessage.c_str());
    attroff(A_BOLD);

    refresh();
}

bool GameController::showGameOverScreen(RankingManager &rankingManager) const
{
    rankingManager.addRecord(stageNum, scoreBoard.getMaxLength(), scoreBoard.getGrowthCount(), scoreBoard.getPoisonCount(), scoreBoard.getSpeedCount(), scoreBoard.getGateCount());
    rankingManager.markLatestAsCurrent();
    rankingManager.saveToFile();

    const bool playAgain = showRankingBoardScreen(rankingManager, stageNum, "GAME OVER! [ A ] 처음부터 재시작 | [ Q ] 종료 | [← / →] 다른 랭킹", true);
    rankingManager.clearCurrentPlayFlag();
    return playAgain;
}

bool GameController::showStageClearScreen(RankingManager &rankingManager) const
{
    rankingManager.addRecord(stageNum, scoreBoard.getMaxLength(), scoreBoard.getGrowthCount(), scoreBoard.getPoisonCount(), scoreBoard.getSpeedCount(), scoreBoard.getGateCount());
    rankingManager.markLatestAsCurrent();
    rankingManager.saveToFile();

    const bool keepGoing = showRankingBoardScreen(rankingManager, stageNum, "STAGE CLEAR! [ A ] 다음 단계 계속 | [ Q ] 종료 | [← / →] 다른 랭킹", true);
    rankingManager.clearCurrentPlayFlag();
    return keepGoing;
}

bool GameController::showTotalClearScreen(RankingManager &rankingManager, const int maxLength, const int growth, const int poison, const int speed, const int gate)
{
    rankingManager.addRecord(5, maxLength, growth, poison, speed, gate);
    rankingManager.markLatestAsCurrent();
    rankingManager.saveToFile();

    const bool playAgain = showRankingBoardScreen(rankingManager, 0, "ALL STAGES CLEAR! [ A ] 처음부터 재시작 | [ Q ] 종료 | [← / →] 다른 랭킹", true);
    rankingManager.clearCurrentPlayFlag();
    return playAgain;
}
