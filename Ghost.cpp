#include "Ghost.hpp"

#include <cmath>
#include <iostream>

// Werewolf-Spritesheet.png      :  140x34,  4 frames (fw=35, fh=34), 1 hàng
// Werewolf-Death-Spritesheet.png:  329x132, 21 frames (fw=47, fh=44), 7 cột x 3
// hàng

sf::Texture Ghost::texWalk_;
sf::Texture Ghost::texDeath_;
bool Ghost::texturesLoaded_ = false;

// ─────────────────────────────────────────────────────────────
bool Ghost::loadTextures() {
  if (texturesLoaded_) return true;
  texturesLoaded_ = true;
  bool ok = true;

  if (!texWalk_.loadFromFile("hinh anh\\Werewolf-Spritesheet.png")) {
    std::cerr << "[Ghost] Missing: hinh anh\\Werewolf-Spritesheet.png\n";
    ok = false;
  }
  texWalk_.setSmooth(false);

  if (!texDeath_.loadFromFile("hinh anh\\Werewolf-Death-Spritesheet.png")) {
    std::cerr << "[Ghost] Missing: hinh anh\\Werewolf-Death-Spritesheet.png\n";
    texDeath_ = texWalk_;  // fallback
  }
  texDeath_.setSmooth(false);
  return ok;
}

// ─────────────────────────────────────────────────────────────
Ghost::Ghost(sf::Vector2f pos) {
  typeId_ = "ghost";
  pos_ = pos;
  hp_ = MAX_HP;
  maxHp_ = MAX_HP;
  alive_ = true;
  dead_ = false;
  attackRange_ = 30.f;
  attackDamage_ = 10;
  attackCooldown_ = 0.2f;
  expValue_ = 8;  // nhiều EXP hơn Zombie vì khó tiêu diệt hơn
}

// ── Helpers ──────────────────────────────────────────────────
void Ghost::setState(GhostState s) {
  if (state_ == s) return;
  state_ = s;
  frame_ = 0;
  timer_ = 0.f;
  animDone_ = false;
}

int Ghost::currentFrameCount() const {
  switch (state_) {
    case GhostState::TakeHit:
      return FRAMES_HIT;
    case GhostState::Death:
      return FRAMES_DEATH;
    default:
      return FRAMES_WALK;
  }
}

float Ghost::currentFrameTime() const {
  switch (state_) {
    case GhostState::TakeHit:
      return FRAME_TIME_HIT;
    case GhostState::Death:
      return FRAME_TIME_DEATH;
    default:
      return FRAME_TIME_WALK;
  }
}

uint8_t Ghost::chaseAlpha() const {
  // Nhấp nháy trong suốt nhẹ nhàng: dao động từ 160 → 255
  float a = 160.f + 95.f * (0.5f + 0.5f * std::sin(pulseTimer_ * 3.f));
  return static_cast<uint8_t>(a);
}

void Ghost::advanceAnim(float dt) {
  timer_ += dt;
  if (timer_ >= currentFrameTime()) {
    timer_ -= currentFrameTime();
    hitFlash_ = !hitFlash_;
    if (++frame_ >= currentFrameCount()) {
      if (state_ == GhostState::Chase)
        frame_ = 0;  // walk loop
      else {
        frame_ = currentFrameCount() - 1;
        animDone_ = true;
      }
    }
  }
}

// ── IMonster interface ────────────────────────────────────────
void Ghost::takeHit(int damage) {
  if (!alive_ || state_ == GhostState::Death) return;
  hp_ -= damage;
  if (hp_ <= 0) {
    hp_ = 0;
    alive_ = false;
    setState(GhostState::Death);
  } else {
    if (state_ != GhostState::TakeHit) setState(GhostState::TakeHit);
  }
}

bool Ghost::overlapsPoint(sf::Vector2f pt, float r) const {
  if (dead_) return false;
  sf::Vector2f d = pt - pos_;
  float minD = HIT_RADIUS + r;
  return (d.x * d.x + d.y * d.y) < minD * minD;
}

void Ghost::update(float dt, sf::Vector2f playerPos) {
  if (dead_) return;

  pulseTimer_ += dt;

  // ── Chuyển trạng thái ────────────────────────────────────
  switch (state_) {
    case GhostState::Chase:
      break;
    case GhostState::TakeHit:
      if (animDone_) setState(alive_ ? GhostState::Chase : GhostState::Death);
      break;
    case GhostState::Death:
      if (animDone_) {
        dead_ = true;
        return;
      }
      break;
  }

  // ── Di chuyển (chỉ khi Chase) ────────────────────────────
  if (state_ == GhostState::Chase) {
    sf::Vector2f diff = playerPos - pos_;
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (dist > 1.f) {
      sf::Vector2f dir = seekMove(playerPos, SPEED, dt, /*stopRange=*/0.f);
      // Ghost bay thẳng, lật chiều theo hướng di chuyển
      flipX_ = (dir.x > 0.f);
    }
    integrateVelocity(
        dt, /*damping=*/0.85f);  // damping cao hơn zombie: Ghost mượt mà hơn
  }

  advanceAnim(dt);
}

void Ghost::draw(sf::RenderTarget& target) const {
  if (dead_) return;

  const bool boss = isBoss();
  const float bossScale = boss ? 2.0f : 1.0f;
  const float finalScale = SCALE * bossScale;

  // ── Boss: viền nhấp nháy màu lạnh ───────────────────────
  if (boss) {
    float pulse = std::abs(std::sin(pulseTimer_ * 3.5f));
    uint8_t alpha = static_cast<uint8_t>(140 + 115 * pulse);
    float ringR = HIT_RADIUS * bossScale + 8.f;

    sf::CircleShape ring2(ringR + 5.f);
    ring2.setFillColor(sf::Color::Transparent);
    ring2.setOutlineColor(
        sf::Color(0, 180, 255, static_cast<uint8_t>(alpha * 0.5f)));
    ring2.setOutlineThickness(4.f);
    ring2.setOrigin({ringR + 5.f, ringR + 5.f});
    ring2.setPosition(pos_);
    target.draw(ring2);

    sf::CircleShape ring(ringR);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(100, 220, 255, alpha));
    ring.setOutlineThickness(2.f);
    ring.setOrigin({ringR, ringR});
    ring.setPosition(pos_);
    target.draw(ring);
  }

  // ── Chọn texture và thông số frame ───────────────────────
  const bool isDeath = (state_ == GhostState::Death);
  const sf::Texture& tex = isDeath ? texDeath_ : texWalk_;

  const int fw = isDeath ? FRAME_W_DEATH : FRAME_W;
  const int fh = isDeath ? FRAME_H_DEATH : FRAME_H;

  // TakeHit: dùng walk frame để tránh index out-of-range
  const int frameIdx =
      (state_ == GhostState::TakeHit) ? (frame_ % FRAMES_WALK) : frame_;

  sf::Sprite sprite(tex);

  if (isDeath) {
    // Death spritesheet dạng lưới: 7 cột x 3 hàng
    int col = frameIdx % DEATH_COLS;
    int row = frameIdx / DEATH_COLS;
    sprite.setTextureRect(sf::IntRect({col * fw, row * fh}, {fw, fh}));
  } else {
    // Walk spritesheet: 1 hàng duy nhất
    sprite.setTextureRect(sf::IntRect({frameIdx * fw, 0}, {fw, fh}));
  }

  sprite.setOrigin({fw / 2.f, fh / 2.f});

  // ── Scale đồng nhất để kích thước walk = death ───────────
  float sy = finalScale;
  if (isDeath) {
    // Điều chỉnh để death sprite có chiều cao tương đương walk
    // FRAME_H=34 (walk) vs FRAME_H_DEATH=44 (death) → tỉ lệ gần nhau, nhân
    // thêm 1.8f
    sy *= static_cast<float>(FRAME_H) / static_cast<float>(FRAME_H_DEATH);
    sy *= 1.3f;
  }
  float sx = flipX_ ? -sy : sy;

  sprite.setScale({sx, sy});
  sprite.setPosition(pos_);

  // ── Màu sắc theo trạng thái ──────────────────────────────
  if (state_ == GhostState::TakeHit) {
    // Flash đỏ khi bị đánh
    sprite.setColor(hitFlash_ ? sf::Color(255, 80, 80)
                              : sf::Color(255, 160, 160));
  } else if (isDeath) {
    // Fade out toàn bộ animation chết
    float ratio =
        static_cast<float>(frame_) / static_cast<float>(FRAMES_DEATH - 1);
    float fadeStart = 0.5f;  //  tan biến sớm hơn zombie
    uint8_t alpha =
        (ratio < fadeStart)
            ? 255
            : static_cast<uint8_t>(
                  255 * (1.f - (ratio - fadeStart) / (1.f - fadeStart)));
  } else if (boss) {
    // Boss ghost: màu xanh lạnh đặc trưng
    sprite.setColor(sf::Color(180, 220, 255));
  }

  target.draw(sprite);

  // ── Boss HP bar ──────────────────────────────────────────
  if (boss) {
    float ratio = static_cast<float>(getHp()) / static_cast<float>(getMaxHp());
    const float barW = 70.f, barH = 7.f;

    sf::RectangleShape bg({barW, barH});
    bg.setFillColor(sf::Color(0, 20, 60, 200));
    bg.setOrigin({barW / 2.f, barH / 2.f});
    float barY = pos_.y - HIT_RADIUS * bossScale - 18.f;
    bg.setPosition({pos_.x, barY});
    target.draw(bg);

    sf::RectangleShape bar({barW * ratio, barH});
    bar.setFillColor(sf::Color(60, 200, 255, 220));
    bar.setOrigin({barW / 2.f, barH / 2.f});
    bar.setPosition({pos_.x, barY});
    target.draw(bar);
  }
}

void Ghost::drawDebug(sf::RenderTarget& target) const {
  if (dead_) return;

  sf::CircleShape hit(HIT_RADIUS);
  hit.setFillColor(sf::Color(0, 150, 255, 60));
  hit.setOutlineColor(sf::Color(100, 200, 255));
  hit.setOutlineThickness(0.5f);
  hit.setOrigin({HIT_RADIUS, HIT_RADIUS});
  hit.setPosition(pos_);
  target.draw(hit);
}