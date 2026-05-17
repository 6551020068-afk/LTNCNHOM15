#pragma once
#include <SFML/Graphics.hpp>

class Bullet {
 public:
  // Bullet sprite
  static constexpr int TEX_W = 29;
  static constexpr int TEX_H = 7;
  static constexpr float SCALE = 3.f;

  // Hit effect (procedural — không cần texture)
  static constexpr int HIT_FRAMES = 6;         // số bước mở rộng
  static constexpr float HIT_FRAME_T = 0.04f;  // giây/bước
  static constexpr float HIT_RADIUS_START = 8.f;
  static constexpr float HIT_RADIUS_END = 28.f;

  static bool loadTextures();

  // Constructor nhận thêm các chỉ số linh hoạt từ ShotData
  Bullet(sf::Vector2f pos, sf::Vector2f dir, float speed = 599.f,
         float radius = 6.f, float lifetime = 3.f, int damage = 1);

  void update(float dt);
  void draw(sf::RenderTarget& target) const;

  sf::Vector2f getPosition() const { return pos_; }
  bool isDead() const { return dead_; }
  int getDamage() const { return damage_; }

  bool hits(sf::Vector2f center, float r) const;
  void kill();

 private:
  sf::Vector2f pos_;
  sf::Vector2f vel_;
  float angle_ = 0.f;
  float age_ = 0.f;
  bool dead_ = false;

  // Cấu hình linh hoạt của viên đạn
  float speed_;
  float radius_;
  float lifetime_;
  int damage_;

  bool hitting_ = false;
  int hitFrame_ = 0;
  float hitTimer_ = 0.f;

  static sf::Texture texBullet_;
  static bool texLoaded_;
};