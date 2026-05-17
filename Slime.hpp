#pragma once
// ════════════════════════════════════════════════════════════
//  Slime.hpp  —  Ví dụ thêm quái mới chỉ trong 1 file
//
//  Slime: chậm hơn FlyEye, HP cao hơn, damage cao hơn
//  Không cần texture phức tạp — dùng shape để demo
//
//  CÁCH DÙNG: MonsterManager::registerFactory("slime", ...)
// ════════════════════════════════════════════════════════════
#include <cmath>

#include "IMonster.hpp"

class Slime : public IMonster {
 public:
  static constexpr float SPEED = 20.f;
  static constexpr float HIT_RADIUS = 15.f;  // Giảm từ 22.f xuống 15.f
  static constexpr float ATTACK_RANGE_PX = 40.f;
  static constexpr int MAX_HP = 5;

  explicit Slime(sf::Vector2f pos) {
    typeId_ = "slime";
    pos_ = pos;
    hp_ = MAX_HP;
    maxHp_ = MAX_HP;
    alive_ = true;
    dead_ = false;
    attackDamage_ = 2;
    attackRange_ = ATTACK_RANGE_PX;
    attackCooldown_ = 2.0f;
    expValue_ = 2;
  }

  void update(float dt, sf::Vector2f playerPos) override {
    if (dead_) return;

    // Bounce timer cho hiệu ứng nhảy
    bounceT_ += dt * 4.f;

    if (alive_) {
      sf::Vector2f diff = playerPos - pos_;
      float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
      if (dist > 1.f) {
        sf::Vector2f dir = diff / dist;
        if (dist > ATTACK_RANGE_PX) pos_ += dir * SPEED * dt;
        tickAttack(dt, playerPos);
      }
    }

    // Death: biến mất sau 0.5s
    if (!alive_) {
      deathTimer_ += dt;
      if (deathTimer_ >= 0.5f) dead_ = true;
    }
  }

  void draw(sf::RenderTarget& target) const override {
    if (dead_) return;

    float bounce = alive_ ? std::abs(std::sin(bounceT_)) * 6.f : 0.f;
    float alpha = alive_ ? 255.f : 255.f * (1.f - deathTimer_ / 0.5f);

    // Body
    sf::CircleShape body(HIT_RADIUS);
    sf::Color col = hitFlash_ ? sf::Color(255, 150, 150, (uint8_t)alpha)
                              : sf::Color(80, 200, 100, (uint8_t)alpha);
    body.setFillColor(col);
    body.setOutlineColor(sf::Color(40, 120, 60, (uint8_t)alpha));
    body.setOutlineThickness(2.f);
    body.setOrigin({HIT_RADIUS, HIT_RADIUS});
    body.setPosition({pos_.x, pos_.y - bounce});
    target.draw(body);

    // Eyes
    sf::CircleShape eye(4.f);
    eye.setFillColor(sf::Color(20, 20, 20, (uint8_t)alpha));
    eye.setOrigin({4.f, 4.f});
    eye.setPosition({pos_.x - 7.f, pos_.y - 8.f - bounce});
    target.draw(eye);
    eye.setPosition({pos_.x + 7.f, pos_.y - 8.f - bounce});
    target.draw(eye);

    // HP bar
    const float barW = 44.f, barH = 5.f;
    float ratio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
    sf::RectangleShape bg({barW, barH});
    bg.setFillColor(sf::Color(60, 0, 0, (uint8_t)alpha));
    bg.setOrigin({barW / 2.f, barH / 2.f});
    bg.setPosition({pos_.x, pos_.y - HIT_RADIUS - 12.f - bounce});
    target.draw(bg);
    sf::RectangleShape bar({barW * ratio, barH});
    bar.setFillColor(sf::Color(80, 220, 80, (uint8_t)alpha));
    bar.setOrigin({barW / 2.f, barH / 2.f});
    bar.setPosition({pos_.x, pos_.y - HIT_RADIUS - 12.f - bounce});
    target.draw(bar);
  }

  void drawDebug(sf::RenderTarget& target) const override {
    sf::CircleShape hit(HIT_RADIUS);
    hit.setFillColor(sf::Color(0, 255, 0, 50));
    hit.setOutlineColor(sf::Color::Green);
    hit.setOutlineThickness(0.5f);
    hit.setOrigin({HIT_RADIUS, HIT_RADIUS});
    hit.setPosition(pos_);
    target.draw(hit);
  }

  void takeHit(int damage) override {
    if (!alive_) return;
    hitFlash_ = true;
    flashTimer_ = 0.1f;
    hp_ -= damage;
    if (hp_ <= 0) {
      hp_ = 0;
      alive_ = false;
    }
  }

  bool overlapsPoint(sf::Vector2f pt, float r) const override {
    if (dead_) return false;
    sf::Vector2f d = pt - pos_;
    float minD = HIT_RADIUS + r;
    return (d.x * d.x + d.y * d.y) < minD * minD;
  }

 private:
  float bounceT_ = 0.f;
  float deathTimer_ = 0.f;
  mutable bool hitFlash_ = false;
  mutable float flashTimer_ = 0.f;
};