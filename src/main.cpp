// main.cpp
// 5단계 - 스테이지 전개 및 미션 시스템 통합 구현 (랭킹 및 인트로 기능 통합)

#include "curses_compat.h" // OS별 curses + sleep 호환 레이어
#include <locale.h>        // 한글 출력용 locale 설정
#include <ctime>           // srand 사용을 위한 ctime
#include <cstdlib>         // srand/rand
#include <vector>
#include <string>
#include "GameController.h"
#include "RankingManager.h"
#include "board.h"

int main()
{
    // 윈도우 환경에서 콘솔 코드페이지를 UTF-8로 설정하여 한글/이모지 출력 및 프리즈 방지
    #ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    #endif

    // 한글 출력을 위해 시스템 locale 사용
    #ifdef _WIN32
    setlocale(LC_ALL, ".UTF-8");
    #else
    setlocale(LC_ALL, "");
    #endif

    // 무작위 난수 생성을 위한 시드값 초기화
    srand(time(NULL));

    // ncurses 초기화 
    initscr();             // ncurses 모드 시작
    start_color();         // 색상 사용 선언
    
    // 인트로 화면이 처음 켜질 때도 색상이 예쁘게 출력되도록 색상 페어 사전 초기화
    Board().initColors();

    keypad(stdscr, TRUE);  // 화살표 키 입력 받기
    noecho();              // 입력한 키를 화면에 보여주지 않음
    curs_set(0);           // 커서 안 보이게
    nodelay(stdscr, TRUE); // getch()가 키 없으면 기다리지 않게

    // 스테이지 리스트 준비
    const std::vector<std::string> stageFiles = {
        "stages/stage1.txt",
        "stages/stage2.txt",
        "stages/stage3.txt",
        "stages/stage4.txt",
        "stages/stage5.txt"};

    // 랭킹 매니저 생성 및 파일 로드
    RankingManager rankingManager;
    rankingManager.loadFromFile();

    // 게임 전체 메인 루프
    while (true)
    {
        // (a) 인트로 화면 출력 (시작 키 'A'가 눌리면 true, 'Q'가 눌리면 false)
        if (GameController::showIntroScreen(rankingManager) == false)
        {
            break; // 프로그램 즉시 종료
        }

        int currentStageIdx = 0;
        bool totalSuccess = false;
        
        // 최종 클리어 시 상위 랭킹에 기록할 5단계 최종 데이터 저장용 변수
        int finalMax = 0, finalGrowth = 0, finalPoison = 0, finalSpeed = 0, finalGate = 0;

        // (b) 스테이지 루프 
        // 순차적 스테이지 진행
        while (currentStageIdx < (int)stageFiles.size())
        {
            GameController game(currentStageIdx + 1, stageFiles[currentStageIdx]);

            if (game.initialize() == false)
            {
                endwin();
                printf("%s 를 읽을 수 없거나 초기화에 실패했습니다.\n", stageFiles[currentStageIdx].c_str());
                return 1;
            }

            // 게임 루프 실행
            const GameResult result = game.run();

            if (result == GameResult::QUIT)
            {
                // 게임 중 'Q' 키를 누른 경우 -> 인트로 화면으로 복구
                break;
            }
            else if (result == GameResult::GAME_OVER)
            {
                // 게임 오버: 랭킹 등록 및 스크린 출력
                // A 누르면 true(1단계 재시작), Q 누르면 false(인트로 화면 이동)
                const bool playAgain = game.showGameOverScreen(rankingManager);
                if (playAgain)
                {
                    currentStageIdx = 0; // 처음(1단계)부터 다시 시작
                    continue;
                }
                else
                {
                    break; // 인트로 화면으로 돌아감
                }
            }
            else if (result == GameResult::STAGE_CLEAR)
            {
                currentStageIdx++;
                if (currentStageIdx < (int)stageFiles.size())
                {
                    // 스테이지 클리어: 랭킹 등록 및 스크린 출력
                    // A 누르면 true(다음 단계 계속), Q 누르면 false(인트로 이동)
                    const bool keepGoing = game.showStageClearScreen(rankingManager);
                    if (keepGoing == false)
                    {
                        break; // 인트로 화면으로 돌아감
                    }
                }
                else
                {
                    // 모든 스테이지 통과 판정
                    totalSuccess = true;
                    finalMax = game.getMaxLength();
                    finalGrowth = game.getGrowthCount();
                    finalPoison = game.getPoisonCount();
                    finalSpeed = game.getSpeedCount();
                    finalGate = game.getGateCount();
                }
            }
        }

        // (c) 최종 전체 성공 화면 (모든 스테이지 클리어 성공)
        if (totalSuccess)
        {
            // 랭킹 등록 및 게임 클리어 보드 출력
            // A 누르면 다시 시작(currentStageIdx = 0 이 되어 루프 재진입 가능)
            const bool restart = GameController::showTotalClearScreen(rankingManager, finalMax, finalGrowth, finalPoison, finalSpeed, finalGate);
            if (restart)
            {
                // 1단계부터 다시 게임 시작
                // 이 블록 밖으로 나가면 다시 인트로 화면으로 돌아가므로,
                // 인트로를 건너뛰고 바로 1단계부터 재시작하고 싶다면 루프 형태를 수정할 수 있으나,
                // 기본 게임 라이프사이클에 맞게 자연스럽게 메인 Intro로 복귀하도록 구현하는 것이 사용자 조작상 훨씬 명확합니다.
            }
        }
    }

    // ncurses 종료 및 터미널 모드 복원
    endwin();
    return 0;
}
