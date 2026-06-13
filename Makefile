# Snake Game Makefile
#
# 디렉토리 구조
#   src/     : .cpp 파일들
#   include/ : .h 파일들
#   stages/  : 게임 맵 데이터 (빌드와 무관)
#
# 크로스 플랫폼 빌드 지원
#   - Linux  : ncursesw (UTF-8 지원)
#   - macOS  : ncurses (시스템 라이브러리)
#   - Windows: PDCurses (MSYS2/MinGW 기준 -lpdcurses)

CXX      = g++
CXXFLAGS = -std=c++14 -Wall -O2 -Iinclude

# OS 감지해서 링크 옵션 결정
ifeq ($(OS),Windows_NT)
    LDFLAGS = -L. -lpdcurses
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        LDFLAGS = -lncurses
    else
        LDFLAGS = -lncursesw
    endif
endif

TARGET   = snake
SRCS     = src/main.cpp src/board.cpp src/snake.cpp src/food.cpp src/poison.cpp src/speeditem.cpp src/gate.cpp src/ScoreBoard.cpp src/GameController.cpp src/RankingManager.cpp src/BlockWall.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

# 실행파일 만들기 (.o 들을 묶어서 링크)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# .cpp 마다 .o 만드는 일반 규칙
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 빌드 산출물 청소
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
