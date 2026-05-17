#pragma once
// ════════════════════════════════════════════════════════════
//  2 cấp độ khó: Easy / Hard
//
//  Easy : quái×1.0, tối đa 80 con, Final Boss 1000 HP
//  Hard : quái×1.8 HP / ×1.5 DMG / ×1.3 Speed,
//         tối đa 150 con, boss mini ×1.5 HP, Final Boss 2500 HP
// ════════════════════════════════════════════════════════════
#include <string>

#include "ScoreSystem.hpp"  // Difficulty enum

struct DifficultyConfig {
  std::string name;
  std::string description;

  // Monster spawner
  int maxMonsters;
  float spawnIntervalBase;
  float spawnIntervalMin;
  int maxMonstersCap = 300;
  // Monster stat multiplier (áp dụng trong từng Monster::loadStats)
  float monsterHpMult;
  float monsterDamageMult;
  float monsterSpeedMult;

  // Boss
  float bossMiniHpMult;  // nhân HP boss mini  (Easy=1.0, Hard=1.5)
  int finalBossHp;       // HP tuyệt đối Final Boss (Easy=1000, Hard=2500)

  // Player
  int playerStartHp;

  // ── 2 preset cố định ─────────────────────────────────────
  static const DifficultyConfig& get(Difficulty d) {
    static const DifficultyConfig EASY{
        "Easy",
        "Danh cho nguoi moi choi. Quai it, chay cham.",
        /*maxMonsters*/ 300,
        /*spawnIntervalBase*/ 3.5f,
        /*spawnIntervalMin*/ 0.8f,
        /*maxMonstersCap*/ 250,
        /*monsterHpMult*/ 1.0f,
        /*monsterDamageMult*/ 1.0f,
        /*monsterSpeedMult*/ 1.0f,
        /*bossMiniHpMult*/ 1.5f,
        /*finalBossHp*/ 5000,
        /*playerStartHp*/ 100,
    };
    static const DifficultyConfig HARD{
        "Hard",
        "Thu thach that su. Quai nhieu, manh va nhanh hon.",
        /*maxMonsters*/ 400,
        /*spawnIntervalBase*/ 2.0f,
        /*spawnIntervalMin*/ 0.4f,
        /*maxMonstersCap*/ 300,
        /*monsterHpMult*/ 1.8f,
        /*monsterDamageMult*/ 1.5f,
        /*monsterSpeedMult*/ 1.3f,
        /*bossMiniHpMult*/ 2.5f,
        /*finalBossHp*/ 8000,
        /*playerStartHp*/ 100,
    };
    return (d == Difficulty::Hard) ? HARD : EASY;
  }
};