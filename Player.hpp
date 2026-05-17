#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

#include "CollisionMap.hpp"
#include "Constants.hpp"

enum class Direction { Down = 0, Left, Right, Up, COUNT };

class Player {
 public:
  // ── Sprite sheet info ──────────────────────────────────────────────────────
  // Frame dimensions thay đổi theo class nhân vật được chọn:
  //   Rogue  : 197×186  (spritesheet gốc)
  //   Mage   : 156×198  (spritesheet_Porta_Ladonna.png)
  //   Druid  : 162×186  (spritesheet_Poe_Ratcho.png)
  //   Cleric : 192×192  (spritesheet_Dommario.png)
  static constexpr int NUM_FRAMES = 4;
  static constexpr float FRAME_TIME = 0.18f;

  // ── Gameplay ───────────────────────────────────────────────────────────────
  static constexpr float SPEED = 200.f;
  static constexpr float SCALE =
      0.38f;  // 240*0.38 ≈ 91 px hiển thị – chỉnh tuỳ ý
  static constexpr float HIT_W = 18.f;
  static constexpr float HIT_H = 18.f;

  explicit Player(const CollisionMap& colMap);

  // Gọi sau khi window sẵn sàng, truyền index class
  // (0=Rogue,1=Mage,2=Druid,3=Cleric)
  bool load();                      // tương thích cũ – load Rogue mặc định
  bool loadForClass(int classIdx);  // load spritesheet theo class đã chọn
  void update(float dt);
  void draw(sf::RenderTarget& target) const;
  void drawDebug(sf::RenderTarget& target) const;

  sf::Vector2f getPosition() const { return pos_; }
  void setPosition(sf::Vector2f p) { pos_ = p; }
  Direction getDirection() const { return dir_; }

  sf::Vector2f getFacingVector() const {
    switch (dir_) {
      case Direction::Up:
        return {0.f, -1.f};
      case Direction::Down:
        return {0.f, 1.f};
      case Direction::Left:
        return {-1.f, 0.f};
      case Direction::Right:
        return {1.f, 0.f};
      default:
        return {0.f, 1.f};
    }
  }
  sf::Vector2f getMoveVector() const { return moveVec_; }

  // Cho phép Game truyền tốc độ theo class nhân vật
  void setSpeed(float s) { speed_ = s; }
  float getSpeed() const { return speed_; }

 private:
  bool collidesAt(float cx, float cy) const;
  sf::Vector2f readInput();
  void advanceAnimation(float dt, bool isMoving);
  void syncSprite();

  const CollisionMap& colMap_;

  // Frame size hiện tại – được set trong loadForClass()
  int frameW_ = 197;
  int frameH_ = 186;

  sf::Vector2f pos_{Constants::MAP_WIDTH * Constants::TILE_RENDER_W / 2.f,
                    Constants::MAP_HEIGHT* Constants::TILE_RENDER_H / 2.f};

  Direction dir_ = Direction::Down;
  int frame_ = 0;
  float timer_ = 0.f;
  bool moving_ = false;

  sf::Texture texture_;  // single spritesheet
  std::unique_ptr<sf::Sprite> sprite_;

  float speed_ = SPEED;  // tốc độ hiện tại, có thể thay đổi qua setSpeed()
  sf::Vector2f moveVec_{};
};