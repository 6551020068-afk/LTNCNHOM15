#pragma once
// ════════════════════════════════════════════════════════════
//  FlyEye.hpp  —  Quái dơi, chỉ dùng 1 animation bay (4 frame)
//  Texture: bat_sprite.png (560x125, 4 frame ngang)
//           dead_bat.png   (624x31,  8 frame ngang)
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>

#include "IMonster.hpp"

enum class FlyEyeState { Chase, Attack, TakeHit, Death };

class FlyEye : public IMonster {
 public:
  // ── Flight sprite: bat_sprite.png ────────────────────────
  static constexpr int FRAME_W = 140;
  static constexpr int FRAME_H = 125;
  static constexpr int FRAMES_FLIGHT = 4;
  static constexpr float FRAME_TIME = 0.12f;

  // ── Death sprite: dead_bat.png 624x31, 8 frames ──────────
  static constexpr int FRAMES_DEATH_BAT = 13;  // ← từ 18 → 13
  static constexpr int FRAME_W_DEATH = 48;     // ← từ 78 → 48  (624/13 ≈ 48)
  static constexpr int FRAME_H_DEATH = 31;
  static constexpr float FRAME_TIME_DEATH =
      0.05f;  // ← có thể tăng lên cho dễ thấy

  // ── Gameplay ─────────────────────────────────────────────
  static constexpr float SCALE = 0.4f;
  static constexpr float SPEED = 50.f;
  static constexpr float ATTACK_RANGE_PX = 45.f;
  static constexpr float HIT_RADIUS = 18.f;
  static constexpr int MAX_HP = 1;

  explicit FlyEye(sf::Vector2f pos);
  static bool loadTextures();

  // ── IMonster interface ───────────────────────────────────
  void update(float dt, sf::Vector2f playerPos) override;
  void draw(sf::RenderTarget& target) const override;
  void drawDebug(sf::RenderTarget& target) const override;
  void takeHit(int damage) override;
  bool overlapsPoint(sf::Vector2f pt, float radius) const override;

 private:
  FlyEyeState state_ = FlyEyeState::Chase;
  int frame_ = 0;
  float timer_ = 0.f;
  bool animDone_ = false;
  bool flipX_ = false;
  float hoverT_ = 0.f;
  bool hitFlash_ = true;

  void advanceAnim(float dt);

  static sf::Texture texFlight_;
  static sf::Texture texDeath_;
  static bool texturesLoaded_;
};