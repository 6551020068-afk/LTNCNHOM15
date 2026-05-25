#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "ISkill.hpp"

struct ZapInfo {
  sf::Vector2f pos;
  void* monster;
  int damage;
};

struct LightningEffect {
  sf::Vector2f pos;
  float timer = 0.f;
  float duration = 0.35f;
  int rings = 3;
  bool isDone() const { return timer >= duration; }
};

class LightningRingSkill : public ISkill {
 public:
  struct LevelStats {
    int zapCount;
    float range;
    float cooldown;
    int damage;
  };

  static constexpr int MAX_LEVEL = 8;

  inline static const LevelStats LEVEL_TABLE[MAX_LEVEL] = {
      {1, 300.f, 1.4f, 1}, {1, 320.f, 1.3f, 1}, {2, 340.f, 1.2f, 1},
      {2, 360.f, 1.1f, 2}, {3, 380.f, 1.0f, 2}, {3, 400.f, 0.9f, 2},
      {4, 420.f, 0.8f, 3}, {4, 440.f, 0.7f, 3},
  };

  LightningRingSkill()
      : ISkill("lightning", "Lightning Ring", LEVEL_TABLE[0].cooldown) {
    info_.description = "Set danh quai gan nhat";
    info_.maxLevel = MAX_LEVEL;
    applyLevelStats();
  }

  std::vector<ShotData> tryFire(sf::Vector2f, sf::Vector2f, float dt) override {
    tickCooldown(dt);
    for (auto& fx : effects_) fx.timer += dt;
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
                       [](const LightningEffect& e) { return e.isDone(); }),
        effects_.end());
    return {};
  }

  std::vector<ZapInfo> zapMonsters(
      sf::Vector2f playerPos,
      const std::vector<std::pair<sf::Vector2f, void*>>& monsters,
      int baseDamage) {
    if (!isReady() || monsters.empty()) return {};
    resetCooldown();

    std::vector<std::pair<float, int>> inRange;
    for (int i = 0; i < (int)monsters.size(); ++i) {
      sf::Vector2f d = monsters[i].first - playerPos;
      float distSq = d.x * d.x + d.y * d.y;
      if (distSq <= currentRange_ * currentRange_)
        inRange.push_back({distSq, i});
    }
    if (inRange.empty()) return {};

    std::sort(inRange.begin(), inRange.end());

    std::vector<ZapInfo> result;
    int toZap = std::min(currentZapCount_, (int)inRange.size());
    for (int i = 0; i < toZap; ++i) {
      int idx = inRange[i].second;
      ZapInfo z;
      z.pos = monsters[idx].first;
      z.monster = monsters[idx].second;
      z.damage = baseDamage * currentDamage_;
      result.push_back(z);
      LightningEffect fx;
      fx.pos = monsters[idx].first;
      fx.rings = evolved_ ? 5 : 3;
      effects_.push_back(fx);
    }

    if (evolved_ && toZap < (int)inRange.size()) {
      int chainIdx = inRange[toZap].second;
      ZapInfo z;
      z.pos = monsters[chainIdx].first;
      z.monster = monsters[chainIdx].second;
      z.damage = baseDamage * currentDamage_ / 2;
      result.push_back(z);
      LightningEffect fx;
      fx.pos = monsters[chainIdx].first;
      effects_.push_back(fx);
    }
    return result;
  }

  void drawEffects(sf::RenderTarget& target) const {
    for (const auto& fx : effects_) {
      float t = fx.timer / fx.duration;
      for (int r = 0; r < fx.rings; ++r) {
        float progress = std::min(1.f, t * 1.5f - r * 0.15f);
        if (progress <= 0.f) continue;

        float radius = 12.f + progress * (40.f + r * 18.f);
        uint8_t alpha = static_cast<uint8_t>(220 * (1.f - progress) *
                                             (1.f - (float)r / fx.rings));

        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.f - r * 0.5f);
        ring.setOutlineColor(sf::Color(180, 230, 255, alpha));
        ring.setOrigin({radius, radius});
        ring.setPosition(fx.pos);
        sf::RenderStates st;
        st.blendMode = sf::BlendAdd;
        target.draw(ring, st);

        if (r == 0) {
          float coreR = radius * 0.3f;
          sf::CircleShape core(coreR);
          core.setFillColor(sf::Color(
              220, 240, 255, static_cast<uint8_t>(180 * (1.f - progress))));
          core.setOrigin({coreR, coreR});
          core.setPosition(fx.pos);
          target.draw(core, st);
        }
      }
      drawBolt(target, fx.pos, t);
    }
  }

  void drawRange(sf::RenderTarget& target, sf::Vector2f pos) const {
    if (evolved_) return;  // range vo han thi khong ve
    sf::CircleShape range(currentRange_);
    range.setFillColor(sf::Color(100, 180, 255, 20));
    range.setOutlineColor(sf::Color(100, 180, 255, 60));
    range.setOutlineThickness(1.f);
    range.setOrigin({currentRange_, currentRange_});
    range.setPosition(pos);
    target.draw(range);
  }

  bool upgrade() override {
    if (evolved_ || info_.level >= MAX_LEVEL) return false;
    ++info_.level;
    applyLevelStats();
    return true;
  }

  bool evolve() {
    if (info_.level < MAX_LEVEL || evolved_) return false;
    evolved_ = true;
    currentZapCount_ = 6;
    currentRange_ = 99999.f;
    currentDamage_ = 4;
    setCooldown(0.5f);
    info_.description = "EVOLVED: Thunder Loop";
    return true;
  }

  bool isEvolved() const { return evolved_; }
  bool isMaxLevel() const { return info_.level >= MAX_LEVEL; }
  float getRange() const { return currentRange_; }

 private:
  void applyLevelStats() {
    int idx = std::min(info_.level, MAX_LEVEL - 1);
    const auto& s = LEVEL_TABLE[idx];
    setCooldown(s.cooldown);
    currentZapCount_ = s.zapCount;
    currentRange_ = s.range;
    currentDamage_ = s.damage;
  }

  // SFML 3: sf::Vertex khong co constructor (pos, color)
  // Phai dung aggregate initialization
  void drawBolt(sf::RenderTarget& target, sf::Vector2f center, float t) const {
    if (t > 0.5f) return;
    uint8_t alpha = static_cast<uint8_t>(200 * (1.f - t * 2.f));
    constexpr int numBolts = 6;
    constexpr float boltLen = 30.f;
    constexpr float PI2 = 2.f * 3.14159265f;

    for (int i = 0; i < numBolts; ++i) {
      float angle = (PI2 / numBolts) * i;
      float jitter = (float)(std::rand() % 10 - 5) * 0.1f;
      float endX = center.x + std::cos(angle + jitter) * boltLen;
      float endY = center.y + std::sin(angle + jitter) * boltLen;

      // SFML 3 aggregate init
      sf::Vertex line[2];
      line[0].position = center;
      line[0].color = sf::Color(200, 230, 255, alpha);
      line[1].position = sf::Vector2f(endX, endY);
      line[1].color = sf::Color(100, 180, 255, static_cast<uint8_t>(alpha / 2));
      target.draw(line, 2, sf::PrimitiveType::Lines);
    }
  }

  int currentZapCount_ = 1;
  float currentRange_ = 300.f;
  int currentDamage_ = 1;
  bool evolved_ = false;
  std::vector<LightningEffect> effects_;
};