#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>

class IMonster {
 public:
  virtual ~IMonster() = default;

  virtual void update(float dt, sf::Vector2f playerPos) = 0;  // gọi mỗi frame
  virtual void draw(sf::RenderTarget& target) const = 0;      // world-space
  virtual void drawDebug(sf::RenderTarget& target) const {}   // tuỳ chọn
  virtual void takeHit(int damage) = 0;  // gọi khi bị tấn công, cộng dồn damage
                                         // vào pendingDamage
  virtual bool overlapsPoint(sf::Vector2f pt, float radius)
      const = 0;  // check va chạm với point (dùng cho hit detection)

  void applyKnockback(sf::Vector2f dir, float force) {
    knockbackVel_ += dir * force;
  }

  void setIsBoss(bool b) { isBoss_ = b; }
  bool isBoss() const { return isBoss_; }

  // ── Despawn (đi xa player) — KHÔNG drop exp ─────────────
  void despawn() {
    alive_ = false;
    dead_ = true;
    killedByPlayer_ = false;  // ← không drop exp
  }

  // ── Bị giết thật (HP = 0) — DROP exp ───────────────────
  // Subclass gọi hàm này khi HP về 0 (thay vì set alive_ trực tiếp)
  void markKilled() {
    alive_ = false;
    dead_ = true;
    killedByPlayer_ = true;  // ← drop exp
  }

  // Giữ lại kill() cũ để không break code cũ,
  // nhưng nội bộ nó = despawn (không drop exp)
  void kill() { despawn(); }

  void setPosition(sf::Vector2f p) {
    pos_ = p;
    velocity_ = {};
  }

  sf::Vector2f getPosition() const { return pos_; }
  bool isAlive() const { return alive_; }
  bool isDead() const { return dead_; }
  bool isKilledByPlayer() const { return killedByPlayer_; }  // ← mới
  int getHp() const { return hp_; }
  int getMaxHp() const { return maxHp_; }
  int getExpValue() const { return expValue_; }
  const std::string& getTypeId() const { return typeId_; }

  int getAndResetPendingDamage() {
    int dmg = pendingDamage_;
    pendingDamage_ = 0;
    return dmg;
  }

  void addSeparationForce(sf::Vector2f f) { separationForce_ += f; }

  void flushSeparation(float dt, float maxSpeed) {
    velocity_ += separationForce_;
    separationForce_ = {};
    float spd2 = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
    float limit = maxSpeed * 0.55f;
    if (spd2 > limit * limit) {
      float spd = std::sqrt(spd2);
      velocity_ = velocity_ / spd * limit;
    }
    pos_ += velocity_ * dt;
    velocity_ *= 0.12f;
  }

  sf::Vector2f seekMove(sf::Vector2f playerPos, float speed, float dt,
                        float stopRange = 0.f) {
    sf::Vector2f diff = playerPos - pos_;
    float dist2 = diff.x * diff.x + diff.y * diff.y;
    if (dist2 < 1.f) return {};
    float dist = std::sqrt(dist2);
    sf::Vector2f dir = diff / dist;
    if (dist > stopRange) {
      velocity_ += dir * speed;
      float spd2 = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
      if (spd2 > speed * speed) {
        float spd = std::sqrt(spd2);
        velocity_ = velocity_ / spd * speed;
      }
    }
    return dir;
  }

  void integrateVelocity(float dt, float damping = 0.80f) {
    pos_ += velocity_ * dt;
    velocity_ *= damping;
  }

  void applyOffset(sf::Vector2f offset) { pos_ += offset; }
  void scaleHp(float mult) {
    hp_ = static_cast<int>(hp_ * mult + 0.5f);
    maxHp_ = static_cast<int>(maxHp_ * mult + 0.5f);
    if (hp_ < 1) hp_ = 1;
  }

 protected:
  bool tickAttack(float dt, sf::Vector2f playerPos) {
    if (attackTimer_ < attackCooldown_) attackTimer_ += dt;
    if (attackTimer_ < attackCooldown_) return false;
    sf::Vector2f diff = playerPos - pos_;
    float distSq = diff.x * diff.x + diff.y * diff.y;
    if (distSq > attackRange_ * attackRange_) return false;
    attackTimer_ = 0.f;
    pendingDamage_ += attackDamage_;
    return true;
  }

  std::string typeId_ = "unknown";
  sf::Vector2f pos_ = {};
  bool alive_ = true;
  bool dead_ = false;
  int hp_ = 1;
  int maxHp_ = 1;
  int expValue_ = 1;
  int attackDamage_ = 1;
  float attackRange_ = 45.f;
  float attackCooldown_ = 1.5f;

 private:
  float attackTimer_ = 999.f;
  int pendingDamage_ = 0;
  sf::Vector2f knockbackVel_ = {};
  bool isBoss_ = false;
  bool killedByPlayer_ = false;  // ← flag mới

 protected:
  sf::Vector2f velocity_ = {};
  sf::Vector2f separationForce_ = {};
};