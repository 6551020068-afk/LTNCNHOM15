#pragma once
// ════════════════════════════════════════════════════════════
//  ScoreSystem.hpp  —  Hệ thống tính điểm
//
//  Công thức điểm:
//    - Mỗi quái chết     : +điểm theo loại quái
//    - Mỗi level up      : +500 * level
//    - Sống sót theo thời gian: +1 điểm/giây
//    - Hệ số nhân theo độ khó (Easy x1.0, Hard x1.5)
// ════════════════════════════════════════════════════════════
#include <string>

enum class Difficulty { Easy, Hard };

struct ScoreSystem {
  // ── Điểm theo loại quái ──────────────────────────────────
  static constexpr int SCORE_FLYEYE = 10;
  static constexpr int SCORE_SKELETON = 25;
  static constexpr int SCORE_SLIME = 15;
  static constexpr int SCORE_DEFAULT = 10;

  // ── Bonus ────────────────────────────────────────────────
  static constexpr int SCORE_PER_LEVEL = 500;
  static constexpr float SCORE_PER_SECOND = 1.f;

  Difficulty difficulty = Difficulty::Easy;
  int score = 0;
  float timeAlive = 0.f;  // giây sống sót
  int kills = 0;
  int coins = 0;           // Coin nhặt được trong trận
  float scoreAccum = 0.f;  // tích lũy điểm thời gian (float → int)

  // ── Hệ số nhân theo độ khó ───────────────────────────────
  float multiplier() const {
    return (difficulty == Difficulty::Hard) ? 1.5f : 1.0f;
  }

  // ── Thêm điểm khi giết quái ──────────────────────────────
  void addKill(const std::string& typeId) {
    ++kills;
    int base = SCORE_DEFAULT;
    if (typeId == "flyeye")
      base = SCORE_FLYEYE;
    else if (typeId == "skeleton")
      base = SCORE_SKELETON;
    else if (typeId == "slime")
      base = SCORE_SLIME;
    score += static_cast<int>(base * multiplier());
    coins += 1;  // Cộng 1 coin cho mỗi quái bị diệt
  }

  // ── Bonus khi level up ───────────────────────────────────
  void addLevelUp(int newLevel) {
    score += static_cast<int>(SCORE_PER_LEVEL * newLevel * multiplier());
  }

  // ── Gọi mỗi frame ────────────────────────────────────────
  void update(float dt) {
    timeAlive += dt;
    scoreAccum += SCORE_PER_SECOND * multiplier() * dt;
    if (scoreAccum >= 1.f) {
      score += static_cast<int>(scoreAccum);
      scoreAccum -= static_cast<int>(scoreAccum);
    }
  }

  // ── Reset ────────────────────────────────────────────────
  void reset() {
    score = 0;
    timeAlive = 0.f;
    kills = 0;
    coins = 0;
    scoreAccum = 0.f;
  }

  // ── Format thời gian MM:SS ───────────────────────────────
  std::string formatTime() const {
    int s = static_cast<int>(timeAlive);
    int m = s / 60;
    s = s % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return buf;
  }
};