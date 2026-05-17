// ════════════════════════════════════════════════════════════
//  test_runner.cpp  —  Unit test cơ bản (không cần framework)
//
//  Build riêng: g++ -std=c++17 test_runner.cpp -o test_runner
//  Chạy      : ./test_runner  (hoặc test_runner.exe trên Win)
//
//  Mỗi TEST() là 1 hàm độc lập. Dùng ASSERT_EQ / ASSERT_TRUE
//  để kiểm tra. Kết quả in ra console.
// ════════════════════════════════════════════════════════════
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

// ── Stub SFML để test không cần link SFML ────────────────────
namespace sf {
struct Vector2f {
  float x = 0, y = 0;
};
}  // namespace sf

// Include các module cần test (header-only hoặc đã tách .cpp)
#include "SaveSystem.hpp"
#include "ScoreSystem.hpp"
#include "dokho.hpp"

// ════════════════════════════════════════════════════════════
//  Micro test framework
// ════════════════════════════════════════════════════════════
static int g_pass = 0, g_fail = 0;

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      ++g_pass;                                                                \
    } else {                                                                   \
      ++g_fail;                                                                \
      std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ << "  expected " \
                << (b) << "  got " << (a) << "\n";                             \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(expr)                                     \
  do {                                                        \
    if (expr) {                                               \
      ++g_pass;                                               \
    } else {                                                  \
      ++g_fail;                                               \
      std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ \
                << "  \"" #expr "\" is false\n";              \
    }                                                         \
  } while (0)

#define ASSERT_THROWS(expr, ExType)                           \
  do {                                                        \
    bool caught = false;                                      \
    try {                                                     \
      expr;                                                   \
    } catch (const ExType&) {                                 \
      caught = true;                                          \
    } catch (...) {                                           \
    }                                                         \
    if (caught) {                                             \
      ++g_pass;                                               \
    } else {                                                  \
      ++g_fail;                                               \
      std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ \
                << "  expected exception " #ExType "\n";      \
    }                                                         \
  } while (0)

#define TEST(name)                        \
  static void name();                     \
  struct _Reg_##name {                    \
    _Reg_##name() {                       \
      std::cout << "[ RUN ] " #name "\n"; \
      name();                             \
    }                                     \
  } _reg_##name;                          \
  static void name()

// ════════════════════════════════════════════════════════════
//  ScoreSystem tests
// ════════════════════════════════════════════════════════════
TEST(score_initial_zero) {
  ScoreSystem s;
  ASSERT_EQ(s.score, 0);
  ASSERT_EQ(s.kills, 0);
}

TEST(score_add_kill_flyeye_easy) {
  ScoreSystem s;
  s.difficulty = Difficulty::Easy;
  s.addKill("flyeye");
  ASSERT_EQ(s.score, ScoreSystem::SCORE_FLYEYE);  // x1.0
  ASSERT_EQ(s.kills, 1);
}

TEST(score_add_kill_skeleton_hard) {
  ScoreSystem s;
  s.difficulty = Difficulty::Hard;
  s.addKill("skeleton");
  // 25 * 1.5 = 37
  ASSERT_EQ(s.score, static_cast<int>(ScoreSystem::SCORE_SKELETON * 1.5f));
}

TEST(score_level_up_bonus) {
  ScoreSystem s;
  s.difficulty = Difficulty::Easy;
  s.addLevelUp(2);
  ASSERT_EQ(s.score, ScoreSystem::SCORE_PER_LEVEL * 2);
}

TEST(score_level_up_bonus_hard) {
  ScoreSystem s;
  s.difficulty = Difficulty::Hard;
  s.addLevelUp(1);
  // 500 * 1 * 1.5 = 750
  ASSERT_EQ(s.score, static_cast<int>(ScoreSystem::SCORE_PER_LEVEL * 1.5f));
}

TEST(score_time_accumulates) {
  ScoreSystem s;
  s.difficulty = Difficulty::Easy;
  s.update(5.0f);  // 5 giây → +5 điểm
  ASSERT_EQ(s.score, 5);
  ASSERT_TRUE(s.timeAlive >= 4.9f);
}

TEST(score_reset_clears_all) {
  ScoreSystem s;
  s.addKill("flyeye");
  s.addLevelUp(3);
  s.update(10.f);
  s.reset();
  ASSERT_EQ(s.score, 0);
  ASSERT_EQ(s.kills, 0);
  ASSERT_EQ(s.timeAlive, 0.f);
}

TEST(score_format_time) {
  ScoreSystem s;
  s.update(90.f);  // 1 phút 30 giây
  ASSERT_TRUE(s.formatTime() == "01:30");
}

TEST(score_multiplier_easy) {
  ScoreSystem s;
  s.difficulty = Difficulty::Easy;
  ASSERT_TRUE(std::abs(s.multiplier() - 1.0f) < 0.001f);
}

TEST(score_multiplier_hard) {
  ScoreSystem s;
  s.difficulty = Difficulty::Hard;
  ASSERT_TRUE(std::abs(s.multiplier() - 1.5f) < 0.001f);
}

TEST(score_unknown_monster_uses_default) {
  ScoreSystem s;
  s.addKill("dragon_boss_xyz");
  ASSERT_EQ(s.score, ScoreSystem::SCORE_DEFAULT);
}

// ════════════════════════════════════════════════════════════
//  SaveSystem tests
// ════════════════════════════════════════════════════════════
TEST(save_load_roundtrip) {
  SaveData d;
  d.highScoreEasy = 1234;
  d.highScoreHard = 5678;
  d.lastScore = 999;
  d.lastCharIndex = 2;
  d.lastDifficulty = "Hard";
  d.lastKills = 42;
  d.lastTimeAlive = 123.5f;

  SaveSystem::save(d);
  SaveData loaded = SaveSystem::load();

  ASSERT_EQ(loaded.highScoreEasy, 1234);
  ASSERT_EQ(loaded.highScoreHard, 5678);
  ASSERT_EQ(loaded.lastScore, 999);
  ASSERT_EQ(loaded.lastCharIndex, 2);
  ASSERT_TRUE(loaded.lastDifficulty == "Hard");
  ASSERT_EQ(loaded.lastKills, 42);
  ASSERT_TRUE(std::abs(loaded.lastTimeAlive - 123.5f) < 0.01f);

  SaveSystem::deleteSave();
}

TEST(save_load_missing_file_returns_default) {
  SaveSystem::deleteSave();  // đảm bảo không có file
  SaveData d = SaveSystem::load();
  ASSERT_EQ(d.highScoreEasy, 0);
  ASSERT_EQ(d.highScoreHard, 0);
}

TEST(save_corrupted_file_throws) {
  // Ghi nội dung hỏng vào file
  {
    std::ofstream f(SaveSystem::SAVE_FILE);
    f << "highScoreEasy=NOT_A_NUMBER\n";
  }
  ASSERT_THROWS(SaveSystem::load(), std::runtime_error);
  SaveSystem::deleteSave();
}

TEST(save_bad_format_throws) {
  {
    std::ofstream f(SaveSystem::SAVE_FILE);
    f << "no_equals_sign_here\n";
  }
  ASSERT_THROWS(SaveSystem::load(), std::runtime_error);
  SaveSystem::deleteSave();
}

TEST(save_update_high_score_easy) {
  SaveData d;
  d.highScoreEasy = 100;
  SaveSystem::updateHighScore(d, 200, Difficulty::Easy);
  ASSERT_EQ(d.highScoreEasy, 200);
}

TEST(save_update_high_score_not_lower) {
  SaveData d;
  d.highScoreEasy = 500;
  SaveSystem::updateHighScore(d, 100, Difficulty::Easy);
  ASSERT_EQ(d.highScoreEasy, 500);  // không ghi đè nếu thấp hơn
}

TEST(save_update_high_score_hard) {
  SaveData d;
  d.highScoreHard = 0;
  SaveSystem::updateHighScore(d, 750, Difficulty::Hard);
  ASSERT_EQ(d.highScoreHard, 750);
}

// ════════════════════════════════════════════════════════════
//  DifficultyConfig tests
// ════════════════════════════════════════════════════════════
TEST(difficulty_easy_config) {
  const auto& cfg = DifficultyConfig::get(Difficulty::Easy);
  ASSERT_TRUE(cfg.name == "Easy");
  ASSERT_TRUE(cfg.maxMonsters > 0);
  ASSERT_TRUE(std::abs(cfg.monsterHpMult - 1.0f) < 0.001f);
  ASSERT_TRUE(cfg.spawnIntervalMin > 0.f);
}

TEST(difficulty_hard_config) {
  const auto& cfg = DifficultyConfig::get(Difficulty::Hard);
  ASSERT_TRUE(cfg.name == "Hard");
  ASSERT_TRUE(cfg.maxMonsters >
              DifficultyConfig::get(Difficulty::Easy).maxMonsters);
  ASSERT_TRUE(cfg.monsterHpMult > 1.0f);
  ASSERT_TRUE(cfg.monsterSpeedMult > 1.0f);
  ASSERT_TRUE(cfg.monsterDamageMult > 1.0f);
}

TEST(difficulty_hard_spawn_faster_than_easy) {
  const auto& easy = DifficultyConfig::get(Difficulty::Easy);
  const auto& hard = DifficultyConfig::get(Difficulty::Hard);
  ASSERT_TRUE(hard.spawnIntervalBase < easy.spawnIntervalBase);
  ASSERT_TRUE(hard.spawnIntervalMin < easy.spawnIntervalMin);
}

// ════════════════════════════════════════════════════════════
//  main
// ════════════════════════════════════════════════════════════
int main() {
  std::cout << "\n========================================\n"
            << "  Unit Tests\n"
            << "========================================\n\n";

  // Tests tự chạy qua _Reg_ constructors ở trên (static init)

  std::cout << "\n========================================\n";
  std::cout << "  PASS: " << g_pass << "   FAIL: " << g_fail << "\n";
  std::cout << "========================================\n\n";

  return (g_fail > 0) ? 1 : 0;
}