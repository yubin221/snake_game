// RankingManager.h
// 랭킹(최고 점수) 기록을 관리하는 클래스.
// 플레이 결과를 RankingRecord 로 저장하고 rankings.txt 에 영속화하며,
// 스테이지별 상위 랭킹 조회와 방금 플레이한 기록 하이라이트를 제공한다.

#ifndef RANKINGMANAGER_H
#define RANKINGMANAGER_H

#include <string>
#include <vector>

struct RankingRecord {
    std::string timestamp;
    int stage;
    int maxLength;
    int growthCount;
    int poisonCount;
    int speedCount;
    int gateCount;
    bool isCurrentPlay = false;
    int rank = 0; // 랭킹 정렬 후 계산된 순위
};

class RankingManager {
public:
    RankingManager();
    ~RankingManager() = default;

    // 파일에서 랭킹 로드
    void loadFromFile(const std::string& filepath = "scoreboard/rankings.txt");

    // 파일에 랭킹 저장 (const 멤버 함수로 수정)
    void saveToFile(const std::string& filepath = "scoreboard/rankings.txt") const;

    // 새로운 랭킹 레코드 추가 
    // 추가 시 현재 시간 구해서 timestamp 자동 생성
    void addRecord(const int stage, const int maxLength, const int growth, const int poison, const int speed, const int gate);

    // 특정 스테이지의 랭킹 리스트 반환
    // 최대 길이 내림차순 정렬 후 상위 10등 + 방금 플레이한 기록이 10등 밖일 시 추가 반환
    std::vector<RankingRecord> getRankings(const int stageFilter) const;

    // 방금 플레이한 기록 표시 설정
    void markLatestAsCurrent();

    // 방금 플레이한 기록 표시 해제
    void clearCurrentPlayFlag();

private:
    std::vector<RankingRecord> records;
};

#endif
