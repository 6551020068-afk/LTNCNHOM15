#pragma once
#include <cmath>
#include <limits>
#include <vector>

#include "ISkill.hpp"

inline sf::Vector2f knifeNormalize(sf::Vector2f v) {
  float len = std::sqrt(v.x * v.x + v.y * v.y);
  return (len > 0.001f) ? v / len : sf::Vector2f{0.f, 1.f};
}

class KnifeSkill : public ISkill {
 public:
  struct LevelStats {
    int knives;
    float cooldown;
    int pierce;
    float damage;
    float speed;
  };

  static constexpr int MAX_LEVEL = 8;
  // Bảng chỉ số kỹ năng theo từng cấp (Số dao, Cooldown, Xuyên thấu, ST nhân,
  // Tốc độ)
  inline static const LevelStats LEVEL_TABLE[MAX_LEVEL] = {
      {1, 0.8f, 0, 1.0f, 500.f},   // Lv1
      {2, 0.8f, 0, 1.0f, 540.f},   // Lv2
      {2, 0.7f, 0, 1.0f, 540.f},   // Lv3
      {3, 0.7f, 1, 1.2f, 560.f},   // Lv4
      {3, 0.6f, 1, 1.5f, 580.f},   // Lv5
      {4, 0.6f, 2, 1.5f, 590.f},   // Lv6
      {4, 0.6f, 2, 1.8f, 600.f},   // Lv7
      {6, 0.50f, 2, 2.0f, 610.f},  // Lv8 (MAX)
  };

  KnifeSkill() : ISkill("knife", "Knife", LEVEL_TABLE[0].cooldown) {
    info_.description = "Dao bay theo huong di chuyen";
    info_.maxLevel = MAX_LEVEL;
    applyLevelStats();
  }

  void setFacingDir(sf::Vector2f movingDir) {
    if (movingDir.x != 0.f || movingDir.y != 0.f)
      lastDir_ = knifeNormalize(movingDir);
  }

  std::vector<ShotData> tryFire(sf::Vector2f origin, sf::Vector2f facing,
                                float dt) override {
    tickCooldown(dt);
    if (!isReady()) return {};

    // Xác định hướng ném: Ưu tiên hướng di chuyển, nếu đứng im dùng hướng đối
    // mặt
    sf::Vector2f dir = (lastDir_.x != 0.f || lastDir_.y != 0.f)
                           ? lastDir_
                           : knifeNormalize(facing);

    if (dir.x == 0.f && dir.y == 0.f) return {};

    if (evolved_) {
      ++burstShotsFired_;
      if (burstShotsFired_ >= maxBurstShots_) {
        setCooldown(burstRestTime_);  // Nghỉ ngơi giữa các đợt ném
        burstShotsFired_ = 0;
      } else {
        setCooldown(
            burstInterval_);  // Thời gian chờ giữa các dao trong cùng 1 đợt
      }
    }
    resetCooldown();

    ShotData s;
    s.origin = origin;
    s.damageMultiplier = currentDamage_;
    s.speed = currentSpeed_;
    s.pierce = currentPierce_;

    float baseAngle = std::atan2(dir.y, dir.x);
    int n = currentKnives_;
    for (int i = 0; i < n; ++i) {
      // Tính toán góc nghiêng để các phi tiêu bay tỏa ra song song
      float offset = 0.f;
      if (n > 1) {
        float t = (float)i / (float)(n - 1) - 0.5f;
        offset = t * spreadOffset_;
      }
      float a = baseAngle + offset;
      s.directions.push_back({std::cos(a), std::sin(a)});
    }

    return {s};
  }

  // Cập nhật chỉ số kỹ năng khi lên cấp
  bool upgrade() override {
    if (evolved_ || info_.level >= MAX_LEVEL) return false;
    ++info_.level;
    applyLevelStats();
    return true;
  }

  // Kích hoạt dạng tiến hóa (Thousand Edge: ném dao liên tục thành từng đợt)
  bool evolve() {
    if (info_.level < MAX_LEVEL || evolved_) return false;
    evolved_ = true;
    setCooldown(burstInterval_);
    burstShotsFired_ = 0;
    info_.description = "EVOLVED: Thousand Edge (Ban lien tuc)";
    return true;
  }

  bool isEvolved() const { return evolved_; }
  bool isMaxLevel() const { return info_.level >= MAX_LEVEL; }
  int getPierce() const { return currentPierce_; }
  float getBulletSpeed() const { return currentSpeed_; }

 private:
  void applyLevelStats() {
    int idx = std::min(info_.level, MAX_LEVEL - 1);
    const auto& s = LEVEL_TABLE[idx];
    setCooldown(s.cooldown);
    currentKnives_ = s.knives;
    currentPierce_ = s.pierce;
    currentDamage_ = s.damage;
    currentSpeed_ = s.speed;
  }

  // Hướng di chuyển gần nhất của nhân vật
  sf::Vector2f lastDir_ = {1.f, 0.f};

  int currentKnives_ = 1;
  int currentPierce_ = 0;
  int currentDamage_ = 1;
  float currentSpeed_ = 480.f;
  float spreadOffset_ = 0.15f;
  bool evolved_ = false;

  int burstShotsFired_ = 0;
  int maxBurstShots_ = 20;       // Số lượng dao tối đa tung ra trong một đợt
  float burstRestTime_ = 1.5f;   // Thời gian chờ phục hồi giữa các đợt (giây)
  float burstInterval_ = 0.08f;  // Thời gian cách nhau giữa 2 lưỡi dao (giây)
};