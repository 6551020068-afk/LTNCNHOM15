#include "Player.hpp"

#include <cmath>
#include <iostream>

#include "CharacterClass.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
Player::Player() {}

// ─────────────────────────────────────────────────────────────────────────────
//  loadForClass() — load spritesheet + frame size theo class đã chọn
// ─────────────────────────────────────────────────────────────────────────────

bool Player::loadForClass(int classIdx) {
  if (classIdx < 0 || classIdx >= CharacterClass::CLASS_COUNT) classIdx = 0;

  const auto& def = CharacterClass::DEFS[classIdx];

  // Normalize backslash -> forward slash (tuong thich da nen tang)
  std::string path = def.spriteSheetPath;
  for (char& c : path)
    if (c == '\\') c = '/';

  if (!texture_.loadFromFile(path)) {
    std::cerr << "[Player] KHONG THE LOAD: \"" << path << "\" cho class ["
              << classIdx << "] " << def.name
              << "\n[Player] Kiem tra thu muc 'hinh anh' ben canh file exe.\n";
    return false;
  }

  // Tinh kich thuoc 1 frame (spritesheet 1 hang, NUM_FRAMES cot)
  frameW_ = static_cast<int>(texture_.getSize().x) / NUM_FRAMES;
  frameH_ = static_cast<int>(texture_.getSize().y);

  if (frameW_ <= 0 || frameH_ <= 0) {
    std::cerr << "[Player] Spritesheet kich thuoc khong hop le: "
              << texture_.getSize().x << "x" << texture_.getSize().y << "\n";
    return false;
  }

  sprite_ = std::make_unique<sf::Sprite>(texture_);
  sprite_->setScale({SCALE, SCALE});

  // Reset animation khi doi class
  frame_ = 0;
  timer_ = 0.f;

  // ← FIX: sync ngay sau khi tao sprite de frame dau hien dung
  // Neu khong goi day, sprite co IntRect mac dinh {0,0,0,0} -> vo hinh
  syncSprite();

  std::cerr << "[Player] Load OK: " << path << " frameW=" << frameW_
            << " frameH=" << frameH_ << "\n";
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────
sf::Vector2f Player::readInput() {
  sf::Vector2f vel(0.f, 0.f);
  moving_ = false;

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
    vel.x -= 1.f;
    moving_ = true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
    vel.x += 1.f;
    moving_ = true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
    vel.y -= 1.f;
    moving_ = true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
    vel.y += 1.f;
    moving_ = true;
  }

  // Lưu vector di chuyển trước khi normalize (cho KnifeSkill dùng)
  if (vel.x != 0.f || vel.y != 0.f) moveVec_ = vel;

  // Cập nhật hướng – ưu tiên trục X khi đi chéo
  if (vel.x < 0.f)
    dir_ = Direction::Left;
  else if (vel.x > 0.f)
    dir_ = Direction::Right;
  else if (vel.y < 0.f)
    dir_ = Direction::Up;
  else if (vel.y > 0.f)
    dir_ = Direction::Down;

  return vel;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Animation
// ─────────────────────────────────────────────────────────────────────────────
void Player::advanceAnimation(float dt, bool isMoving) {
  if (!isMoving) {
    return;
  }
  timer_ += dt;
  if (timer_ >= FRAME_TIME) {
    timer_ -= FRAME_TIME;
    frame_ = (frame_ + 1) % NUM_FRAMES;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sync sprite: chọn frame + flip ngang nếu đi trái
// ─────────────────────────────────────────────────────────────────────────────
void Player::syncSprite() {
  if (!sprite_) return;

  // ── Texture rect (chọn cột frame) ─────────────────────────────────────────
  constexpr int BLEED_OFFSET = 1;
  sprite_->setTextureRect(
      sf::IntRect(sf::Vector2i(frame_ * frameW_ + BLEED_OFFSET, 0),
                  sf::Vector2i(frameW_ - BLEED_OFFSET * 2, frameH_)));

  // ── Flip: hướng Left → scale X âm ─────────────────────────────────────────
  bool flipX = (dir_ == Direction::Left);
  sprite_->setScale({flipX ? -SCALE : SCALE, SCALE});

  // Origin tâm frame
  // FIX: origin luôn dùng giá trị dương (tính theo texture space),
  // SFML tự xử lý đúng khi scale âm — KHÔNG cần đảo origin khi flip.
  float originX = (frameW_ - BLEED_OFFSET * 2) / 2.f;
  float originY = frameH_ / 2.f;
  sprite_->setOrigin({originX, originY});
  sprite_->setPosition(pos_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  update
// ─────────────────────────────────────────────────────────────────────────────
void Player::update(float dt) {
  if (!sprite_) return;

  sf::Vector2f vel = readInput();

  // Normalize khi đi chéo
  if (vel.x != 0.f && vel.y != 0.f) vel /= std::sqrt(2.f);

  pos_.x += vel.x * speed_ * dt;
  pos_.y += vel.y * speed_ * dt;

  const float maxX =
      static_cast<float>(Constants::MAP_WIDTH * Constants::TILE_RENDER_W) -
      HIT_W;
  const float maxY =
      static_cast<float>(Constants::MAP_HEIGHT * Constants::TILE_RENDER_H) -
      HIT_H;
  pos_.x = std::max(HIT_W, std::min(pos_.x, maxX));
  pos_.y = std::max(HIT_H, std::min(pos_.y, maxY));

  advanceAnimation(dt, moving_);
  syncSprite();
}

// ─────────────────────────────────────────────────────────────────────────────
//  draw
// ─────────────────────────────────────────────────────────────────────────────
static void drawShadow(sf::RenderTarget& target, sf::Vector2f pos, float rx,
                       float ry, sf::Color color) {
  // Vẽ 1 hình dẹt duy nhất để tạo thành đường thẳng
  sf::CircleShape c(rx);
  c.setScale({1.f, ry / rx});
  c.setFillColor(color);
  c.setOrigin({rx, rx});
  c.setPosition(pos);
  target.draw(c);
}

void Player::draw(sf::RenderTarget& target) const {
  if (!sprite_) return;
  drawShadow(target, {pos_.x, pos_.y + frameH_ * SCALE * 0.20f}, 15.f, 1.5f,
             sf::Color(0, 0, 0, 255));
  target.draw(*sprite_);
}

void Player::drawDebug(sf::RenderTarget& target) const {
  sf::RectangleShape hb({HIT_W * 2.f, HIT_H * 2.f});
  hb.setFillColor(sf::Color(255, 0, 0, 80));
  hb.setOutlineColor(sf::Color::Red);
  hb.setOutlineThickness(0.5f);
  hb.setOrigin({HIT_W, HIT_H});
  hb.setPosition(pos_);
  target.draw(hb);

  sf::CircleShape dot(1.5f);
  dot.setFillColor(sf::Color::Yellow);
  dot.setOrigin({1.5f, 1.5f});
  dot.setPosition(pos_);
  target.draw(dot);
}