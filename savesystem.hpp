#pragma once
// ════════════════════════════════════════════════════════════
//  SaveSystem.hpp  —  Lưu và tải dữ liệu game
//
//  Lưu vào file text đơn giản (key=value), dễ đọc / debug.
//  Xử lý ngoại lệ đầy đủ: file không tồn tại, dữ liệu hỏng.
//
//  Dữ liệu lưu:
//    - High score (theo từng độ khó)
//    - Lần chơi cuối: class, score, thời gian sống, kills
// ════════════════════════════════════════════════════════════
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "ScoreSystem.hpp"

// ── Dữ liệu 1 lần chơi ───────────────────────────────────────
struct SaveData {
  // High scores
  int highScoreEasy = 0;
  int highScoreHard = 0;

  // Lần chơi gần nhất
  int lastScore = 0;
  int lastCharIndex = 0;
  std::string lastDifficulty = "Easy";
  int lastKills = 0;
  float lastTimeAlive = 0.f;

  // ── Thêm các biến Hệ thống Nâng cấp (Meta Progression) ──
  int totalCoins = 0;
  int metaHpLevel = 0;
  int metaDamageLevel = 0;
  int metaSpeedLevel = 0;
};

// ════════════════════════════════════════════════════════════
class SaveSystem {
 public:
  static constexpr const char* SAVE_FILE = "savegame.dat";

  // ── Lưu file ─────────────────────────────────────────────
  // Ném std::runtime_error nếu không thể ghi file
  static void save(const SaveData& data) {
    std::ofstream f(SAVE_FILE);
    if (!f.is_open())
      throw std::runtime_error(
          std::string("[SaveSystem] Khong the ghi file: ") + SAVE_FILE);

    f << "highScoreEasy=" << data.highScoreEasy << "\n"
      << "highScoreHard=" << data.highScoreHard << "\n"
      << "lastScore=" << data.lastScore << "\n"
      << "lastCharIndex=" << data.lastCharIndex << "\n"
      << "lastDifficulty=" << data.lastDifficulty << "\n"
      << "lastKills=" << data.lastKills << "\n"
      << "lastTimeAlive=" << data.lastTimeAlive << "\n"
      << "totalCoins=" << data.totalCoins << "\n"
      << "metaHpLevel=" << data.metaHpLevel << "\n"
      << "metaDamageLevel=" << data.metaDamageLevel << "\n"
      << "metaSpeedLevel=" << data.metaSpeedLevel << "\n";

    if (!f.good())
      throw std::runtime_error("[SaveSystem] Loi khi ghi du lieu.");

    std::cout << "[SaveSystem] Da luu: " << SAVE_FILE << "\n";
  }

  // ── Tải file ─────────────────────────────────────────────
  // Trả về SaveData mặc định nếu file không tồn tại.
  // Ném std::runtime_error nếu file tồn tại nhưng dữ liệu hỏng.
  static SaveData load() {
    SaveData data;
    std::ifstream f(SAVE_FILE);
    if (!f.is_open()) {
      std::cout << "[SaveSystem] Chua co file luu — dung gia tri mac dinh.\n";
      return data;  // lần đầu chơi
    }
    std::unordered_map<std::string, std::string> kv;
    std::string line;
    int lineNum = 0;
    while (std::getline(f, line)) {
      ++lineNum;
      if (line.empty() || line[0] == '#') continue;
      auto pos = line.find('=');
      if (pos == std::string::npos)
        throw std::runtime_error("[SaveSystem] Dinh dang loi dong " +
                                 std::to_string(lineNum) + ": " + line);
      kv[line.substr(0, pos)] = line.substr(pos + 1);
    }

    // Parse từng key — bọc try/catch cho stoi/stof
    try {
      if (kv.count("highScoreEasy"))
        data.highScoreEasy = std::stoi(kv["highScoreEasy"]);
      if (kv.count("highScoreHard"))
        data.highScoreHard = std::stoi(kv["highScoreHard"]);
      if (kv.count("lastScore")) data.lastScore = std::stoi(kv["lastScore"]);
      if (kv.count("lastCharIndex"))
        data.lastCharIndex = std::stoi(kv["lastCharIndex"]);
      if (kv.count("lastDifficulty"))
        data.lastDifficulty = kv["lastDifficulty"];
      if (kv.count("lastKills")) data.lastKills = std::stoi(kv["lastKills"]);
      if (kv.count("lastTimeAlive"))
        data.lastTimeAlive = std::stof(kv["lastTimeAlive"]);

      // Đọc dữ liệu Upgrade
      if (kv.count("totalCoins")) data.totalCoins = std::stoi(kv["totalCoins"]);
      if (kv.count("metaHpLevel"))
        data.metaHpLevel = std::stoi(kv["metaHpLevel"]);
      if (kv.count("metaDamageLevel"))
        data.metaDamageLevel = std::stoi(kv["metaDamageLevel"]);
      if (kv.count("metaSpeedLevel"))
        data.metaSpeedLevel = std::stoi(kv["metaSpeedLevel"]);
    } catch (const std::exception& e) {
      throw std::runtime_error(std::string("[SaveSystem] Du lieu bi hong: ") +
                               e.what());
    }

    // Validate giá trị hợp lệ
    if (data.highScoreEasy < 0 || data.highScoreHard < 0)
      throw std::runtime_error("[SaveSystem] High score am - du lieu bi loi.");
    if (data.lastCharIndex < 0 || data.lastCharIndex > 2)
      data.lastCharIndex = 0;  // reset về mặc định

    std::cout << "[SaveSystem] Da tai: highEasy=" << data.highScoreEasy
              << " highHard=" << data.highScoreHard << "\n";
    return data;
  }

  // ── Cập nhật high score sau mỗi lần chơi ─────────────────
  static void updateHighScore(SaveData& data, int score, Difficulty diff) {
    if (diff == Difficulty::Hard) {
      if (score > data.highScoreHard) data.highScoreHard = score;
    } else {
      if (score > data.highScoreEasy) data.highScoreEasy = score;
    }
  }

  // ── Xóa file save ─────────────────────────────────────────
  static void deleteSave() {
    if (std::remove(SAVE_FILE) == 0)
      std::cout << "[SaveSystem] Da xoa file luu.\n";
  }
};