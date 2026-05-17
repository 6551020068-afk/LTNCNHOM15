#include "FlyEye.hpp"

#include <cmath>
#include <iostream>

sf::Texture FlyEye::texFlight_;
sf::Texture FlyEye::texDeath_;
bool FlyEye::texturesLoaded_ = false;

// ─────────────────────────────────────────────────────────────────────────────
//  loadTextures
// ─────────────────────────────────────────────────────────────────────────────
bool FlyEye::loadTextures() {
  if (texturesLoaded_) return true;
  texturesLoaded_ = true;
  bool ok = true;

  if (!texFlight_.loadFromFile("hinh anh\\bat_sprite.png")) {
    std::cerr << "[FlyEye] Missing: hinh anh\\bat_sprite.png\n";
    ok = false;
  }
  texFlight_.setSmooth(false);

  if (!texDeath_.loadFromFile("hinh anh\\dead_bat.png")) {
    std::cerr << "[FlyEye] Missing: hinh anh\\dead_bat.png\n";
    ok = false;
  }
  texDeath_.setSmooth(false);

  return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
FlyEye::FlyEye(sf::Vector2f pos) {
  typeId_ = "flyeye";
  pos_ = pos;
  hp_ = MAX_HP;
  maxHp_ = MAX_HP;
  alive_ = true;
  dead_ = false;
  attackRange_ = ATTACK_RANGE_PX;
  attackDamage_ = 3;
  attackCooldown_ = 1.f;
  expValue_ = 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  advanceAnim
// ─────────────────────────────────────────────────────────────────────────────
void FlyEye::advanceAnim(float dt) {
  float currentFrameTime =
      (state_ == FlyEyeState::Death) ? FRAME_TIME_DEATH : FRAME_TIME;

  timer_ += dt;
  if (timer_ >= currentFrameTime) {
    timer_ -= currentFrameTime;
    hitFlash_ = !hitFlash_;

    if (state_ == FlyEyeState::Death) {
      if (frame_ < FRAMES_DEATH_BAT - 1) {
        frame_++;
      } else {
        animDone_ = true;
      }
    } else {
      frame_ = (frame_ + 1) % FRAMES_FLIGHT;
      if (frame_ == 0) animDone_ = true;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  takeHit
// ─────────────────────────────────────────────────────────────────────────────
void FlyEye::takeHit(int damage) {
  if (!alive_ || state_ == FlyEyeState::Death) return;
  hp_ -= damage;
  if (hp_ <= 0) {
    hp_ = 0;
    alive_ = false;
    state_ = FlyEyeState::Death;
    animDone_ = false;
    frame_ = 0;
    timer_ = 0.f;
  } else {
    state_ = FlyEyeState::TakeHit;
    animDone_ = false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  overlapsPoint
// ─────────────────────────────────────────────────────────────────────────────
bool FlyEye::overlapsPoint(sf::Vector2f pt, float r) const {
  if (dead_) return false;
  sf::Vector2f d = pt - pos_;
  float minD = HIT_RADIUS + r;
  return (d.x * d.x + d.y * d.y) < minD * minD;
}

// ─────────────────────────────────────────────────────────────────────────────
//  update
// ─────────────────────────────────────────────────────────────────────────────
void FlyEye::update(float dt, sf::Vector2f playerPos) {
  if (dead_) return;

  // ── State transitions ────────────────────────────────────
  switch (state_) {
    case FlyEyeState::Chase:
      break;
    case FlyEyeState::Attack:
      if (animDone_) state_ = FlyEyeState::Chase;
      break;
    case FlyEyeState::TakeHit:
      if (animDone_) state_ = alive_ ? FlyEyeState::Chase : FlyEyeState::Death;
      break;
    case FlyEyeState::Death:
      if (animDone_) {
        dead_ = true;
        return;
      }
      break;
  }

  // ── Movement & combat ────────────────────────────────────
  if (state_ == FlyEyeState::Chase || state_ == FlyEyeState::Attack) {
    sf::Vector2f diff = playerPos - pos_;
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

    if (dist > 1.f) {
      if (dist < ATTACK_RANGE_PX) {
        if (state_ == FlyEyeState::Chase) {
          state_ = FlyEyeState::Attack;
          animDone_ = false;
        }
        tickAttack(dt, playerPos);
      } else {
        sf::Vector2f dir = seekMove(playerPos, SPEED, dt, ATTACK_RANGE_PX);
        flipX_ = (dir.x < 0.f);
      }
    }

    // Hover lên xuống nhẹ
    hoverT_ += dt * 3.f;
    pos_.y += std::sin(hoverT_) * 2.5f * dt;

    integrateVelocity(dt, /*damping=*/0.78f);
  }

  advanceAnim(dt);
}

// ─────────────────────────────────────────────────────────────────────────────
//  draw
// ─────────────────────────────────────────────────────────────────────────────
void FlyEye::draw(sf::RenderTarget& target) const {
  if (dead_) return;

  const bool boss = isBoss();
  const float bossScale = boss ? 2.2f : 1.0f;
  const float finalScale = SCALE * bossScale;

  // ── Boss glow ring ───────────────────────────────────────
  if (boss) {
    float pulse = std::abs(std::sin(hoverT_ * 2.5f));
    uint8_t alpha = static_cast<uint8_t>(160 + 95 * pulse);
    float ringR = HIT_RADIUS * bossScale + 10.f;

    sf::CircleShape ring2(ringR + 6.f);
    ring2.setFillColor(sf::Color::Transparent);
    ring2.setOutlineColor(
        sf::Color(255, 80, 0, static_cast<uint8_t>(alpha * 0.6f)));
    ring2.setOutlineThickness(4.f);
    ring2.setOrigin({ringR + 6.f, ringR + 6.f});
    ring2.setPosition(pos_);
    target.draw(ring2);

    sf::CircleShape ring(ringR);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 220, 0, alpha));
    ring.setOutlineThickness(3.f);
    ring.setOrigin({ringR, ringR});
    ring.setPosition(pos_);
    target.draw(ring);
  }

  // ── Sprite ───────────────────────────────────────────────
  const sf::Texture& tex =
      (state_ == FlyEyeState::Death) ? texDeath_ : texFlight_;
  sf::Sprite sprite(tex);

  // Tự động tính frame width (fw) và frame height (fh) giống cách của
  // Zombie.cpp
  int totalFrames =
      (state_ == FlyEyeState::Death) ? FRAMES_DEATH_BAT : FRAMES_FLIGHT;
  int fw = tex.getSize().x / totalFrames;
  int fh = tex.getSize().y;
  if (fh == 0) fh = 1;  // Tránh lỗi chia cho 0

  int frameIdx =
      (state_ == FlyEyeState::TakeHit) ? (frame_ % FRAMES_FLIGHT) : frame_;

  sprite.setTextureRect(sf::IntRect({frameIdx * fw, 0}, {fw, fh}));
  sprite.setOrigin({fw / 2.f, fh / 2.f});

  // Áp dụng chung 1 mức scale cho cả bay và chết để kích thước bằng nhau
  int flightFh = texFlight_.getSize().y;
  if (flightFh == 0) flightFh = 1;
  float sy = finalScale * (static_cast<float>(FRAME_H) / flightFh);

  // Nhân thêm hệ số phóng to khi Dơi chết nếu thấy xác quá bé
  if (state_ == FlyEyeState::Death) {
    sy *=
        4.5f;  // Bạn có thể chỉnh lại mức 4.5f này cho vừa mắt giống bên Zombie
  }

  float sx = flipX_ ? -sy : sy;
  sprite.setScale({sx, sy});
  sprite.setPosition(pos_);

  // ── Màu sắc theo state ───────────────────────────────────
  if (state_ == FlyEyeState::TakeHit) {
    sprite.setColor(hitFlash_ ? sf::Color(255, 80, 80)
                              : sf::Color(255, 160, 160));
  } else if (state_ == FlyEyeState::Death) {
    // Chỉ fade mờ dần ở 1/4 cuối animation, giữ nguyên màu gốc phần còn lại
    // (Giống Zombie)
    float ratio =
        static_cast<float>(frame_) / static_cast<float>(FRAMES_DEATH_BAT - 1);
    float fadeStart = 0.75f;
    uint8_t alpha =
        (ratio < fadeStart)
            ? 255
            : static_cast<uint8_t>(
                  255 * (1.f - (ratio - fadeStart) / (1.f - fadeStart)));
    sprite.setColor(sf::Color(255, 255, 255, alpha));
  } else if (boss) {
    sprite.setColor(sf::Color(255, 180, 180));
  }
  target.draw(sprite);

  // ── Boss HP bar ──────────────────────────────────────────
  if (boss) {
    float ratio = static_cast<float>(getHp()) / static_cast<float>(getMaxHp());
    const float barW = 80.f, barH = 8.f;
    float barY = pos_.y - HIT_RADIUS * bossScale - 20.f;

    sf::RectangleShape bg({barW, barH});
    bg.setFillColor(sf::Color(60, 0, 0, 200));
    bg.setOrigin({barW / 2.f, barH / 2.f});
    bg.setPosition({pos_.x, barY});
    target.draw(bg);

    sf::RectangleShape bar({barW * ratio, barH});
    bar.setFillColor(sf::Color(255, 60, 60, 220));
    bar.setOrigin({barW / 2.f, barH / 2.f});
    bar.setPosition({pos_.x, barY});
    target.draw(bar);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawDebug
// ─────────────────────────────────────────────────────────────────────────────
void FlyEye::drawDebug(sf::RenderTarget& target) const {
  if (dead_) return;

  sf::CircleShape hit(HIT_RADIUS);
  hit.setFillColor(sf::Color(255, 0, 0, 60));
  hit.setOutlineColor(sf::Color::Red);
  hit.setOutlineThickness(0.5f);
  hit.setOrigin({HIT_RADIUS, HIT_RADIUS});
  hit.setPosition(pos_);
  target.draw(hit);

  sf::CircleShape atk(ATTACK_RANGE_PX);
  atk.setFillColor(sf::Color::Transparent);
  atk.setOutlineColor(sf::Color(255, 80, 0, 150));
  atk.setOutlineThickness(0.5f);
  atk.setOrigin({ATTACK_RANGE_PX, ATTACK_RANGE_PX});
  atk.setPosition(pos_);
  target.draw(atk);
}