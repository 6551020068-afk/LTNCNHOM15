#pragma once
// ════════════════════════════════════════════════════════════
// Quản lý Lưu và Tải dữ liệu trò chơi (Save/Load System)
//
// Dữ liệu được lưu dưới dạng văn bản (key=value) để dễ đọc và kiểm tra.
// Tích hợp xử lý ngoại lệ khi file không tồn tại hoặc dữ liệu bị hỏng.
//
// Các dữ liệu được lưu bao gồm:
// - Điểm cao nhất (High Score) phân theo độ khó.
// - Thông tin lần chơi cuối: nhân vật, điểm số, thời gian sinh tồn, số quái
// diệt.
// - Tiến trình nâng cấp vĩnh viễn hệ thống (Meta Progression).
// ════════════════════════════════════════════════════════════
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "ScoreSystem.hpp"

// Cấu trúc lưu trữ toàn bộ dữ liệu của người chơi
struct SaveData {
  // Điểm cao nhất
  int highScoreEasy = 0;
  int highScoreHard = 0;

  // Dữ liệu của ván chơi gần nhất
  int lastScore = 0;
  int lastCharIndex = 0;
  std::string lastDifficulty = "Easy";
  int lastKills = 0;
  float lastTimeAlive = 0.f;

  // Tiến trình nâng cấp chỉ số vĩnh viễn (Meta Progression)
  int totalCoins = 0;
  int metaHpLevel = 0;
  int metaDamageLevel = 0;
  int metaSpeedLevel = 0;
};

// ════════════════════════════════════════════════════════════
class SaveSystem {
 public:
  static constexpr const char* SAVE_FILE = "savegame.dat";

  // Ghi dữ liệu vào file (Ném ngoại lệ std::runtime_error nếu thất bại)
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

  // Đọc dữ liệu từ file (Trả về giá trị mặc định nếu file không tồn tại)
  static SaveData load() {
    SaveData data;
    std::ifstream f(SAVE_FILE);
    if (!f.is_open()) {
      std::cout << "[SaveSystem] Chua co file luu. Su dung gia tri mac dinh.\n";
      return data;
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

    // Chuyển đổi và gán dữ liệu vào cấu trúc SaveData
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

      // Đọc tiến trình nâng cấp vĩnh viễn (nếu có)
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

    // Kiểm tra tính hợp lệ của dữ liệu
    if (data.highScoreEasy < 0 || data.highScoreHard < 0)
      throw std::runtime_error("[SaveSystem] High score am - du lieu bi loi.");
    if (data.lastCharIndex < 0 || data.lastCharIndex > 2)
      data.lastCharIndex = 0;

    std::cout << "[SaveSystem] Da tai: highEasy=" << data.highScoreEasy
              << " highHard=" << data.highScoreHard << "\n";
    return data;
  }

  // Cập nhật điểm kỷ lục sau mỗi ván đấu
  static void updateHighScore(SaveData& data, int score, Difficulty diff) {
    if (diff == Difficulty::Hard) {
      if (score > data.highScoreHard) data.highScoreHard = score;
    } else {
      if (score > data.highScoreEasy) data.highScoreEasy = score;
    }
  }

  // Xóa file lưu trữ game hiện tại
  static void deleteSave() {
    if (std::remove(SAVE_FILE) == 0)
      std::cout << "[SaveSystem] Da xoa file luu.\n";
  }
};