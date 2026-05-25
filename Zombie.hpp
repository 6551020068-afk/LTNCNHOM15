#pragma once
// ════════════════════════════════════════════════════════════
//  Zombie.hpp  —  Quái zombie, kế thừa IMonster
//
//  Trạng thái: Chase → TakeHit → Death  (KHÔNG có Attack)
//  Texture: Walkzb.png, Deathzb.png
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>

#include "IMonster.hpp"

enum class ZombieState { Chase, TakeHit, Death };

class Zombie : public IMonster {
 public:
  // ── Hằng số ─────────────────────────────────────────────
  static constexpr int FRAME_W = 34;     // ← chỉnh theo sprite sheet thực tế
  static constexpr int FRAME_H = 35;     // ← chỉnh theo sprite sheet thực tế
  static constexpr int FRAMES_WALK = 4;  // ← số frame animation đi bộ
  static constexpr int FRAMES_HIT =
      4;  // ← số frame animation bị đánh (dùng lại walk)
  static constexpr float SCALE = 1.8f;  // giảm từ 2.5 → kích thước hợp lý
  static constexpr float SPEED = 30.f;
  static constexpr float HIT_RADIUS = 14.f;
  static constexpr float ATTACK_RANGE_PX = 30.f;
  static constexpr float FRAME_TIME_WALK = 0.12f;
  static constexpr float FRAME_TIME_HIT = 0.10f;
  static constexpr float FRAME_TIME_DEATH =
      0.04f;  // Giảm xuống để hoạt ảnh chết chạy nhanh hơn
  static constexpr int FRAMES_DEATH = 18;  // dùng lại 4 frame walk làm death
  static constexpr int MAX_HP = 7;

  explicit Zombie(sf::Vector2f pos);
  static bool loadTextures();

  // ── IMonster interface ───────────────────────────────────
  void update(float dt, sf::Vector2f playerPos) override;
  void draw(sf::RenderTarget& target) const override;
  void drawDebug(sf::RenderTarget& target) const override;
  void takeHit(int damage) override;
  bool overlapsPoint(sf::Vector2f pt, float radius) const override;

 private:
  ZombieState state_ = ZombieState::Chase;
  int frame_ = 0;
  float timer_ = 0.f;
  bool animDone_ = false;
  bool flipX_ = false;
  mutable bool hitFlash_ = true;

  void setState(ZombieState s);
  void advanceAnim(float dt);
  int currentFrameCount() const;
  float currentFrameTime() const;
  const sf::Texture& currentTex() const;

  static sf::Texture texWalk_;
  static sf::Texture texDeath_;
  static bool texturesLoaded_;
};