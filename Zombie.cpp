#include "Zombie.hpp"

#include <cmath>
#include <iostream>

sf::Texture Zombie::texWalk_;
sf::Texture Zombie::texDeath_;
bool Zombie::texturesLoaded_ = false;

// Khởi tạo và nạp hình ảnh vào bộ nhớ cho quái vật Zombie
bool Zombie::loadTextures() {
  if (texturesLoaded_) return true;
  texturesLoaded_ = true;
  bool ok = true;
  if (!texWalk_.loadFromFile("hinh anh\\zombie.png")) {
    std::cerr << "[zombie] Missing: hinh anh\\zombie.png\n";
    ok = false;
  }
  texWalk_.setSmooth(false);

  // Tải hình ảnh riêng dành cho lúc quái bị hạ gục
  if (!texDeath_.loadFromFile("hinh anh\\dead_zombie.png")) {
    texDeath_ = texWalk_;
  }
  texDeath_.setSmooth(false);
  return ok;
}

Zombie::Zombie(sf::Vector2f pos) {
  typeId_ = "zombie";
  pos_ = pos;
  hp_ = MAX_HP;
  maxHp_ = MAX_HP;
  alive_ = true;
  dead_ = false;
  attackRange_ = ATTACK_RANGE_PX;
  attackDamage_ = 5;
  attackCooldown_ = 0.2f;
  expValue_ = 5;
}

// Chuyển đổi trạng thái của quái vật và reset bộ đếm thời gian hiệu ứng
void Zombie::setState(ZombieState s) {
  if (state_ == s) return;
  state_ = s;
  frame_ = 0;
  timer_ = 0.f;
  animDone_ = false;
}

const sf::Texture& Zombie::currentTex() const {
  if (state_ == ZombieState::Death) return texDeath_;
  return texWalk_;
}

int Zombie::currentFrameCount() const {
  switch (state_) {
    case ZombieState::TakeHit:
      return FRAMES_HIT;
    case ZombieState::Death:
      return FRAMES_DEATH;
    default:
      return FRAMES_WALK;
  }
}

float Zombie::currentFrameTime() const {
  switch (state_) {
    case ZombieState::TakeHit:
      return FRAME_TIME_HIT;
    case ZombieState::Death:
      return FRAME_TIME_DEATH;
    default:
      return FRAME_TIME_WALK;
  }
}

void Zombie::advanceAnim(float dt) {
  timer_ += dt;
  if (timer_ >= currentFrameTime()) {
    timer_ -= currentFrameTime();
    hitFlash_ = !hitFlash_;
    if (++frame_ >= currentFrameCount()) {
      if (state_ == ZombieState::Chase)
        frame_ = 0;
      else {
        frame_ = currentFrameCount() - 1;
        animDone_ = true;
      }
    }
  }
}

// Cập nhật lượng HP khi chịu sát thương
void Zombie::takeHit(int damage) {
  if (!alive_ || state_ == ZombieState::Death) return;
  hp_ -= damage;
  if (hp_ <= 0) {
    hp_ = 0;
    alive_ = false;
    setState(ZombieState::Death);
  } else {
    if (state_ != ZombieState::TakeHit) setState(ZombieState::TakeHit);
  }
}

bool Zombie::overlapsPoint(sf::Vector2f pt, float r) const {
  if (dead_) return false;
  sf::Vector2f d = pt - pos_;
  float minD = HIT_RADIUS + r;
  return (d.x * d.x + d.y * d.y) < minD * minD;
}

void Zombie::update(float dt, sf::Vector2f playerPos) {
  if (dead_) return;

  // Quản lý và kết thúc hiệu ứng chuyển tiếp trạng thái
  switch (state_) {
    case ZombieState::Chase:
      break;
    case ZombieState::TakeHit:
      if (animDone_) setState(alive_ ? ZombieState::Chase : ZombieState::Death);
      break;
    case ZombieState::Death:
      if (animDone_) {
        dead_ = true;
        return;
      }
      break;
  }

  // Xử lý di chuyển và hướng tới người chơi
  if (state_ == ZombieState::Chase) {
    sf::Vector2f diff = playerPos - pos_;
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

    // Thực hiện tấn công nếu trong tầm đánh
    if (dist <= attackRange_) {
      tickAttack(dt, playerPos);
    }

    if (dist > 1.f) {
      sf::Vector2f dir = seekMove(playerPos, SPEED, dt, 15.f);
      // Đảo chiều khung hình để tương ứng với hướng di chuyển
      flipX_ = (dir.x > 0.f);
    }
    integrateVelocity(dt, 0.75f);
  }

  advanceAnim(dt);
}

void Zombie::draw(sf::RenderTarget& target) const {
  if (dead_) return;

  const bool boss = isBoss();
  const float bossScale = boss ? 2.2f : 1.0f;
  const float finalScale = SCALE * bossScale;

  // Hiển thị vòng sáng nhấp nháy dưới chân nếu là Boss
  if (boss) {
    float pulse = std::abs(std::sin(timer_ * 4.f));
    uint8_t alpha = static_cast<uint8_t>(160 + 95 * pulse);
    float ringR = HIT_RADIUS * bossScale + 10.f;

    sf::CircleShape ring2(ringR + 6.f);
    ring2.setFillColor(sf::Color::Transparent);
    ring2.setOutlineColor(
        sf::Color(180, 0, 255, static_cast<uint8_t>(alpha * 0.6f)));
    ring2.setOutlineThickness(4.f);
    ring2.setOrigin({ringR + 6.f, ringR + 6.f});
    ring2.setPosition(pos_);
    target.draw(ring2);

    sf::CircleShape ring(ringR);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(220, 100, 255, alpha));
    ring.setOutlineThickness(3.f);
    ring.setOrigin({ringR, ringR});
    ring.setPosition(pos_);
    target.draw(ring);
  }

  // Vẽ hình ảnh Zombie (Sprite) dựa trên số khung hình
  const sf::Texture& tex = currentTex();

  int totalFrames = (state_ == ZombieState::Death) ? FRAMES_DEATH : FRAMES_WALK;
  int fw = tex.getSize().x / totalFrames;
  int fh = tex.getSize().y;
  if (fh == 0) fh = 1;

  int frameIdx =
      (state_ == ZombieState::TakeHit) ? (frame_ % FRAMES_WALK) : frame_;

  sf::Sprite sprite(tex);
  sprite.setTextureRect(sf::IntRect({frameIdx * fw, 0}, {fw, fh}));
  sprite.setOrigin({fw / 2.f, fh / 2.f});

  int walkFh = texWalk_.getSize().y;
  if (walkFh == 0) walkFh = 1;
  float sy = finalScale * (static_cast<float>(FRAME_H) / walkFh);

  if (state_ == ZombieState::Death) {
    sy *= 4.0f;
  }

  float sx = flipX_ ? -sy : sy;

  sprite.setScale({sx, sy});
  sprite.setPosition(pos_);

  if (state_ == ZombieState::TakeHit) {
    sprite.setColor(hitFlash_ ? sf::Color(255, 80, 80)
                              : sf::Color(255, 160, 160));
  } else if (state_ == ZombieState::Death) {
    float ratio =
        static_cast<float>(frame_) / static_cast<float>(FRAMES_DEATH - 1);
    float fadeStart = 0.75f;
    uint8_t alpha =
        (ratio < fadeStart)
            ? 255
            : static_cast<uint8_t>(
                  255 * (1.f - (ratio - fadeStart) / (1.f - fadeStart)));
    sprite.setColor(sf::Color(255, 255, 255, alpha));
  } else if (boss) {
    sprite.setColor(sf::Color(220, 180, 255));
  }
  target.draw(sprite);

  // Hiển thị thanh máu (HP bar) cho Mini Boss
  if (boss) {
    float ratio = static_cast<float>(getHp()) / static_cast<float>(getMaxHp());
    const float barW = 80.f, barH = 8.f;
    sf::RectangleShape bg({barW, barH});
    bg.setFillColor(sf::Color(60, 0, 0, 200));
    bg.setOrigin({barW / 2.f, barH / 2.f});
    float barY = pos_.y - HIT_RADIUS * bossScale - 20.f;
    bg.setPosition({pos_.x, barY});
    target.draw(bg);

    sf::RectangleShape bar({barW * ratio, barH});
    bar.setFillColor(sf::Color(200, 60, 255, 220));
    bar.setOrigin({barW / 2.f, barH / 2.f});
    bar.setPosition({pos_.x, barY});
    target.draw(bar);
  }
}

void Zombie::drawDebug(sf::RenderTarget& target) const {
  if (dead_) return;

  sf::CircleShape hit(HIT_RADIUS);
  hit.setFillColor(sf::Color(255, 0, 0, 60));
  hit.setOutlineColor(sf::Color::Red);
  hit.setOutlineThickness(0.5f);
  hit.setOrigin({HIT_RADIUS, HIT_RADIUS});
  hit.setPosition(pos_);
  target.draw(hit);
}