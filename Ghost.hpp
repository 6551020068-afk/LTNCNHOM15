#pragma once
// ════════════════════════════════════════════════════════════
//  Ghost.hpp  —  Quái Ghost, kế thừa IMonster
//
//  Trạng thái: Chase → TakeHit → Death  (KHÔNG có Attack)
//  Texture: sprite_Ghost.png (4 frames, 29x32px)
//           sprite_Ghost_Death.png (12 frames, 30x55px)
//
//  Đặc điểm Ghost:
//    - Di chuyển nhanh hơn Zombie
//    - Có hiệu ứng trong suốt (alpha pulsing) khi Chase
//    - Kích thước nhỏ hơn, khó hit hơn
//    - HP thấp hơn nhưng tốc độ bù lại
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>

#include "IMonster.hpp"

enum class GhostState { Chase, TakeHit, Death };

class Ghost : public IMonster {
 public:
  // ── Hằng số ─────────────────────────────────────────────
  static constexpr int FRAME_W =
      35;  // frame width  (walk sprite)  — Werewolf-Spritesheet 140/4
  static constexpr int FRAME_H =
      34;  // frame height (walk sprite)  — Werewolf-Spritesheet 34px
  static constexpr int FRAME_W_DEATH =
      47;  // frame width  (death sprite) — Werewolf-Death-Spritesheet 329/7
  static constexpr int FRAME_H_DEATH =
      44;  // frame height (death sprite) — Werewolf-Death-Spritesheet 132/3
  static constexpr int FRAMES_WALK = 4;  // số frame animation đi / bay
  static constexpr int FRAMES_HIT = 4;   // dùng lại walk frames khi bị đánh
  static constexpr int FRAMES_DEATH =
      21;  // số frame animation chết (7 cột × 3 hàng)
  static constexpr int DEATH_COLS = 7;  // số cột trong death spritesheet
  static constexpr float SCALE =
      2.0f;  // scale hiển thị (ghost to hơn 1 chút so với sprite nhỏ)
  static constexpr float SPEED = 55.f;       // nhanh hơn Zombie (30)
  static constexpr float HIT_RADIUS = 12.f;  // nhỏ hơn Zombie (14), khó hit hơn
  static constexpr float FRAME_TIME_WALK = 0.14f;
  static constexpr float FRAME_TIME_HIT = 0.09f;
  static constexpr float FRAME_TIME_DEATH =
      0.04f;  // animation chết nhanh hơn vì ít frame hơn và tránh kéo dài khi
              // đã chết
  static constexpr int MAX_HP = 80;

  explicit Ghost(sf::Vector2f pos);
  static bool loadTextures();

  // ── IMonster interface ───────────────────────────────────
  void update(float dt, sf::Vector2f playerPos) override;
  void draw(sf::RenderTarget& target) const override;
  void drawDebug(sf::RenderTarget& target) const override;
  void takeHit(int damage) override;
  bool overlapsPoint(sf::Vector2f pt, float radius) const override;

 private:
  GhostState state_ = GhostState::Chase;
  int frame_ = 0;
  float timer_ = 0.f;
  float pulseTimer_ = 0.f;  // timer riêng cho hiệu ứng alpha pulsing
  bool animDone_ = false;
  bool flipX_ = false;
  bool hitFlash_ = true;

  void setState(GhostState s);
  void advanceAnim(float dt);
  int currentFrameCount() const;
  float currentFrameTime() const;

  // Trả về alpha 0-255 cho hiệu ứng nhấp nháy trong suốt của ghost
  uint8_t chaseAlpha() const;

  static sf::Texture texWalk_;
  static sf::Texture texDeath_;
  static bool texturesLoaded_;
};