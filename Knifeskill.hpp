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
  // knives: số dao, cooldown, pierce: số lần xuyên thấu, damage: hệ số nhân sát
  // thương, speed: tốc độ bay của dao
  inline static const LevelStats LEVEL_TABLE[MAX_LEVEL] = {
      {1, 0.8f, 0, 1.0f, 500.f},   // Lv1
      {2, 0.8f, 0, 1.0f, 540.f},   // Lv2
      {2, 0.7f, 0, 1.0f, 540.f},   // Lv3
      {3, 0.7f, 1, 1.2f, 560.f},   // Lv4
      {3, 0.6f, 1, 1.5f, 580.f},   // Lv5 (Giảm từ 2 xuống 1.5)
      {4, 0.6f, 2, 1.5f, 590.f},   // Lv6 (Giảm từ 2 xuống 1.5)
      {4, 0.6f, 2, 1.8f, 600.f},   // Lv7 (Giảm từ 2 xuống 1.8)
      {6, 0.50f, 2, 2.0f, 610.f},  // Lv8 MAX (Giảm từ 3 xuống 2.0)
  };

  KnifeSkill() : ISkill("knife", "Knife", LEVEL_TABLE[0].cooldown) {
    info_.description = "Dao bay theo huong di chuyen";
    info_.maxLevel = MAX_LEVEL;
    applyLevelStats();
  }

  // ── Game gọi mỗi frame để cập nhật hướng di chuyển ──────
  // movingDir: lấy từ player.getFacingVector() hoặc input vector
  // Nếu player đứng yên (0,0): giữ nguyên hướng cũ
  void setFacingDir(sf::Vector2f movingDir) {
    if (movingDir.x != 0.f || movingDir.y != 0.f)
      lastDir_ = knifeNormalize(movingDir);
    // Nếu đứng yên: lastDir_ giữ nguyên hướng lần cuối di chuyển
  }

  // ── tryFire ──────────────────────────────────────────────
  std::vector<ShotData> tryFire(sf::Vector2f origin, sf::Vector2f facing,
                                float dt) override {
    tickCooldown(dt);
    if (!isReady()) return {};

    // Dùng lastDir_ (hướng di chuyển), fallback về facing nếu chưa có
    sf::Vector2f dir = (lastDir_.x != 0.f || lastDir_.y != 0.f)
                           ? lastDir_
                           : knifeNormalize(facing);

    if (dir.x == 0.f && dir.y == 0.f) return {};

    if (evolved_) {
      ++burstShotsFired_;
      if (burstShotsFired_ >= maxBurstShots_) {
        setCooldown(
            burstRestTime_);  // Nghỉ một khoảng thời gian sau khi hết đợt
        burstShotsFired_ = 0;
      } else {
        setCooldown(burstInterval_);  // Giãn cách giữa các dao trong 1 đợt
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
      // Nhiều dao: xếp song song nhau (offset vuông góc với hướng bắn)
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

  // ── Upgrade ──────────────────────────────────────────────
  bool upgrade() override {
    if (evolved_ || info_.level >= MAX_LEVEL) return false;
    ++info_.level;
    applyLevelStats();
    return true;
  }

  // Evolution — Thousand Edge:
  // Không thay đổi số dao hay hướng bắn
  // Chỉ giảm cooldown xuống cực thấp → bắn liên tục
  bool evolve() {
    if (info_.level < MAX_LEVEL || evolved_) return false;
    evolved_ = true;
    setCooldown(burstInterval_);  // Bắt đầu đợt bắn đầu tiên
    burstShotsFired_ = 0;
    info_.description = "EVOLVED: Thousand Edge (ban theo dot roi nghi)";
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

  // Hướng di chuyển cuối cùng của player (8 hướng WASD)
  sf::Vector2f lastDir_ = {1.f, 0.f};  // mặc định nhìn phải

  int currentKnives_ = 1;
  int currentPierce_ = 0;
  int currentDamage_ = 1;
  float currentSpeed_ = 480.f;
  float spreadOffset_ = 0.15f;  // radian, khoảng cách giữa các dao
  bool evolved_ = false;

  int burstShotsFired_ = 0;
  int maxBurstShots_ = 20;  // Số dao phóng ra trong 1 đợt (có thể chỉnh tuỳ ý)
  float burstRestTime_ = 1.5f;   // Thời gian nghỉ giữa các đợt (giây)
  float burstInterval_ = 0.08f;  // Thời gian giữa các dao trong cùng 1 đợt
};