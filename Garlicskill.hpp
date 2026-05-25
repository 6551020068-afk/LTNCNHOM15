#pragma once
// ════════════════════════════════════════════════════════════
//  GarlicSkill.hpp  —  Garlic kiểu Vampire Survivors
//
//  Cơ chế:
//    - Vùng AoE LIÊN TỤC xung quanh player (không bắn đạn)
//    - Gây damage theo tick (mỗi X giây)
//    - Đẩy lùi (knockback) quái ra xa player
//    - Hiệu ứng: vòng tròn nhấp nháy quanh player
//
//
//  Evolution (Soul Eater):
//    - Range x1.5, damage x4
//    - Mỗi quái chết trong range: heal player 1 HP
//    - Knockback mạnh hơn nhiều
//
//  GarlicSkill trả về GarlicHitInfo thay vì ShotData/ZapInfo
//  Game xử lý: takeHit + knockback + heal
// ════════════════════════════════════════════════════════════
#include <cmath>
#include <vector>

#include "ISkill.hpp"

// Thông tin một lần tick garlic
struct GarlicHitInfo {
  void* monster;
  sf::Vector2f monsterPos;
  sf::Vector2f knockbackDir;  // hướng đẩy (đã normalize, từ player ra ngoài)
  float knockbackForce;
  int damage;
  bool killedByHit;  // Game set sau takeHit — dùng để heal
};

// Hiệu ứng vòng tròn garlic quanh player
struct GarlicEffect {
  float pulseTimer = 0.f;  // 0 → pulsePeriod, loop
  float pulsePeriod = 0.6f;
  float alpha = 180.f;
};

class GarlicSkill : public ISkill {
 public:
  struct LevelStats {
    float range;
    float damage;
    float knockback;
    float tickInterval;  // giây giữa các lần damage
  };

  static constexpr int MAX_LEVEL = 8;

  inline static const LevelStats LEVEL_TABLE[MAX_LEVEL] = {
      //  range   dmg  knockback  tick
      {80.f, 1.f, 60.f, 0.6f},    // Lv1
      {90.f, 1.2f, 70.f, 0.6f},   // Lv2
      {100.f, 1.2f, 80.f, 0.4f},  // Lv3
      {110.f, 2.f, 90.f, 0.4f},   // Lv4
      {120.f, 2.f, 100.f, 0.4f},  // Lv5
      {125.f, 2.f, 110.f, 0.4f},  // Lv6
      {130.f, 3.f, 120.f, 0.2f},  // Lv7
      {140.f, 3.f, 130.f, 0.2f},  // Lv8 MAX
  };

  GarlicSkill() : ISkill("garlic", "Garlic", LEVEL_TABLE[0].tickInterval) {
    info_.description = "Vung AoE xung quanh";
    info_.maxLevel = MAX_LEVEL;
    applyLevelStats();
  }

  std::vector<ShotData> tryFire(sf::Vector2f origin, sf::Vector2f /*facing*/,
                                float dt) override {
    tickCooldown(dt);
    playerPos_ = origin;

    // Update hieu ung nhap nhay
    fx_.pulseTimer += dt;
    if (fx_.pulseTimer >= fx_.pulsePeriod) fx_.pulseTimer -= fx_.pulsePeriod;

    return {};  // Khong spawn dan
  }

  std::vector<GarlicHitInfo> tickDamage(
      sf::Vector2f playerPos,
      const std::vector<std::pair<sf::Vector2f, void*>>& monsters,
      int baseDamage) {
    if (!isReady() || monsters.empty()) return {};
    resetCooldown();

    std::vector<GarlicHitInfo> result;
    for (auto& [mpos, mptr] : monsters) {
      sf::Vector2f diff = mpos - playerPos;
      float distSq = diff.x * diff.x + diff.y * diff.y;

      if (distSq <= currentRange_ * currentRange_) {
        GarlicHitInfo hit;
        hit.monster = mptr;
        hit.monsterPos = mpos;
        hit.damage = baseDamage * currentDamage_;
        hit.killedByHit = false;

        float dist = std::sqrt(distSq);
        if (dist > 0.1f)
          hit.knockbackDir = diff / dist;
        else
          hit.knockbackDir = {1.f, 0.f};

        hit.knockbackForce = currentKnockback_;
        result.push_back(hit);
      }
    }
    return result;
  }

  // ── Draw: vong tron nhap nhay quanh player ───────────────
  void drawEffect(sf::RenderTarget& target, sf::Vector2f playerPos) const {
    float t = fx_.pulseTimer / fx_.pulsePeriod;  // 0 → 1

    // Vong chinh: nhap nhay scale nhe
    float scale = 1.f + std::sin(t * 3.14159265f * 2.f) * 0.06f;
    float r = currentRange_ * scale;
    uint8_t a =
        static_cast<uint8_t>(60 + std::abs(std::sin(t * 3.14159265f)) * 60.f);

    sf::RenderStates st;
    st.blendMode = sf::BlendAdd;

    // Vong ngoai (xanh la)
    sf::CircleShape outer(r);
    outer.setFillColor(sf::Color::Transparent);
    outer.setOutlineThickness(2.5f);
    outer.setOutlineColor(sf::Color(120, 255, 80, a));
    outer.setOrigin({r, r});
    outer.setPosition(playerPos);
    target.draw(outer, st);

    // Vong trong (vang nhat)
    float rInner = r * 0.7f;
    sf::CircleShape inner(rInner);
    inner.setFillColor(
        sf::Color(180, 255, 120, static_cast<uint8_t>(a * 0.3f)));
    inner.setOutlineThickness(1.f);
    inner.setOutlineColor(
        sf::Color(200, 255, 100, static_cast<uint8_t>(a * 0.6f)));
    inner.setOrigin({rInner, rInner});
    inner.setPosition(playerPos);
    target.draw(inner, st);

    // Evolution: them vong do
    if (evolved_) {
      sf::CircleShape evo(r * 1.08f);
      evo.setFillColor(sf::Color::Transparent);
      evo.setOutlineThickness(1.5f);
      evo.setOutlineColor(
          sf::Color(255, 80, 80, static_cast<uint8_t>(a * 0.8f)));
      evo.setOrigin({r * 1.08f, r * 1.08f});
      evo.setPosition(playerPos);
      target.draw(evo, st);
    }
  }

  void drawDebugRange(sf::RenderTarget& target, sf::Vector2f pos) const {
    sf::CircleShape dbg(currentRange_);
    dbg.setFillColor(sf::Color(80, 255, 50, 15));
    dbg.setOutlineColor(sf::Color(80, 255, 50, 80));
    dbg.setOutlineThickness(1.f);
    dbg.setOrigin({currentRange_, currentRange_});
    dbg.setPosition(pos);
    target.draw(dbg);
  }

  // ── Upgrade ──────────────────────────────────────────────
  bool upgrade() override {
    if (evolved_ || info_.level >= MAX_LEVEL) return false;
    ++info_.level;
    applyLevelStats();
    return true;
  }

  // Evolution — Soul Eater
  bool evolve() {
    if (info_.level < MAX_LEVEL || evolved_) return false;
    evolved_ = true;
    currentRange_ *= 1.5f;
    currentDamage_ = 4;
    currentKnockback_ = 200.f;
    setCooldown(0.6f);
    info_.description = "EVOLVED: Soul Eater (hut mau)";
    return true;
  }

  bool isEvolved() const { return evolved_; }
  bool isMaxLevel() const { return info_.level >= MAX_LEVEL; }
  float getRange() const { return currentRange_; }
  bool canHeal() const { return evolved_; }  // chi heal khi EVO

 private:
  void applyLevelStats() {
    int idx = std::min(info_.level, MAX_LEVEL - 1);
    const auto& s = LEVEL_TABLE[idx];
    setCooldown(s.tickInterval);
    currentRange_ = s.range;
    currentDamage_ = s.damage;
    currentKnockback_ = s.knockback;
  }

  float currentRange_ = 80.f;
  int currentDamage_ = 1;
  float currentKnockback_ = 60.f;
  bool evolved_ = false;
  sf::Vector2f playerPos_ = {};
  GarlicEffect fx_;
};