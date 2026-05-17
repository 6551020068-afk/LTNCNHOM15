#pragma once
// ════════════════════════════════════════════════════════════
//  ExpManager.hpp
//
//  Công thức EXP:  expRequired(lv) = lv + 5
//  Ví dụ:
//    Lv1→2 :  6 EXP    Lv5→6 : 10 EXP
//    Lv10→11: 15 EXP   Lv20→21: 25 EXP
//  → Player cần giết ~8-12 quái mỗi level (phù hợp nhịp 10 phút)
//
//  EXP per kill (theo thiết kế cân bằng):
//    FlyEye  = 1     Skeleton = 3     Slime = 2
//    Boss mini KHÔNG drop EXP — reward HP restore (xử lý ở Game.cpp)
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>

#include "ExpOrb.hpp"

class ExpManager {
 public:
  static int expRequired(int level) {
    static const std::array<int, 20> fixedExp = {
        7,    // Lv 1 -> 2
        10,   // Lv 2 -> 3
        18,   // Lv 3 -> 4
        24,   // Lv 4 -> 5
        33,   // Lv 5 -> 6
        40,   // Lv 6 -> 7
        50,   // Lv 7 -> 8
        62,   // Lv 8 -> 9
        70,   // Lv 9 -> 10
        78,   // Lv 10 -> 11
        85,   // Lv 11 -> 12
        96,   // Lv 12 -> 13
        105,  // Lv 13 -> 14
        115,  // Lv 14 -> 15
        125,  // Lv 15 -> 16
        135,  // Lv 16 -> 17
        150,  // Lv 17 -> 18
        160,  // Lv 18 -> 19
        170,  // Lv 19 -> 20
        180   // Lv 20 -> 21
    };

    if (level >= 1 && level <= 20) {
      return fixedExp[level - 1];
    }

    // Đối với level > 20, bạn có thể áp dụng công thức tăng mạnh như đã thảo
    // luận Ví dụ: Mỗi cấp sau 20 tăng thêm 500 EXP so với cấp trước
    return 180 + (level - 20) * 50;
  }

  ExpManager() { ExpOrb::loadTexture(); }

  void spawnOrb(sf::Vector2f pos, int value = 1);
  int update(float dt, sf::Vector2f playerPos);  // trả về EXP nhặt được
  void draw(sf::RenderTarget& target) const;

  // Thêm EXP, trả về true nếu level up
  bool addExp(int amount);

  int getLevel() const { return level_; }
  int getExp() const { return currentExp_; }
  int getExpReq() const { return expRequired(level_); }
  int getTotalExp() const { return totalExp_; }

 private:
  std::vector<ExpOrb> orbs_;
  int level_ = 1;
  int currentExp_ = 0;
  int totalExp_ = 0;
};