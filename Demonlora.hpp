#pragma once
// ════════════════════════════════════════════════════════════
//  DemonLord.hpp  —  Final Boss, kế thừa IMonster
//
//  3 GIAI ĐOẠN (theo HP):
//    Giai đoạn 1 (100%→60%): Chase đơn, speed 70px/s
//                              Triệu hồi 5 FlyEye mỗi 15s
//    Giai đoạn 2  (60%→30%): Bắn 4 đạn chữ thập, speed 90px/s
//                              Triệu hồi Skeleton mỗi 20s
//    Giai đoạn 3  (30%→0% ): Enrage — speed 120px/s
//                              AoE pulse mỗi 3s (damage 3, knockback)
//                              Triệu hồi Slime mỗi 10s
//
//  HP tuỳ độ khó:
//  Easy: 5000 HP (đủ để thử hết 3 giai đoạn)
//  Hard: 8000 HP (đủ để thử hết 3 giai đoạn
//    Truyền vào constructor: DemonLord(pos, cfg.finalBossHp)
//
//  Poll projectile mỗi frame (Game::update):
//    for (auto* m : monsters_.getLiveMonsters()) {
//        if (auto* d = dynamic_cast<DemonLord*>(m)) {
//            for (auto& p : d->getPendingProjectiles())
//                bullets_.spawnEnemy(p.origin, p.direction, p.speed, p.damage);
//            d->clearProjectiles();
//            // Summon requests
//            for (auto& s : d->getPendingSummons())
//                monsters_.spawnDirect(s.typeId, s.pos, false);
//            d->clearSummons();
//            // AoE pulse
//            if (d->hasPendingAoe()) { /* apply to nearby monsters */ }
//            d->clearAoe();
//        }
//    }
// ════════════════════════════════════════════════════════════

#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>
#include <vector>

#include "IMonster.hpp"

// ── Đạn boss bắn ra ──────────────────────────────────────────
struct BossProjectile {
  sf::Vector2f origin;
  sf::Vector2f direction;
  float speed = 240.f;
  float damage = 2.f;
};

// ── Yêu cầu triệu hồi ────────────────────────────────────────
struct SummonRequest {
  std::string typeId;
  sf::Vector2f pos;
};

// ── AoE pulse ────────────────────────────────────────────────
struct AoePulse {
  sf::Vector2f origin;
  float radius = 180.f;
  int damage = 3;
  float kbForce = 280.f;
};

enum class DemonPhase { Phase1, Phase2, Phase3 };
enum class DemonState { Chase, Attack };

// ════════════════════════════════════════════════════════════
class DemonLord : public IMonster {
 public:
  // ── Sprite ───────────────────────────────────────────────
  // demonlord.png: 1344x176px — 6 frames đều nhau, mỗi frame 224px wide
  // Sprite thực nằm khoảng x=21..207 (local), y=9..175 trong mỗi slot 224x176
  static constexpr int FRAME_W = 224;
  static constexpr int FRAME_H = 176;
  static constexpr int FRAMES_RUN = 6;
  static constexpr int FRAMES_ATTACK = 6;
  static constexpr float SCALE = 1.f;

  static constexpr float FRAME_TIME_RUN = 0.09f;
  static constexpr float FRAME_TIME_ATTACK = 0.07f;

  // ── Base stats (Phase 1) ─────────────────────────────────
  static constexpr float HIT_RADIUS = 60.f;  // px
  static constexpr float ATTACK_RANGE = 54.f;
  static constexpr int MELEE_DAMAGE = 18;  // HP mỗi đòn
  static constexpr float ATTACK_COOLDOWN = 0.3f;

  // ── Phase speeds ─────────────────────────────────────────
  static constexpr float SPEED_P1 = 70.f;
  static constexpr float SPEED_P2 = 90.f;
  static constexpr float SPEED_P3 = 120.f;

  // ── Projectile ───────────────────────────────────────────
  // Phase 1: vòng 6 viên mỗi 3s
  // Phase 2: xoắn ốc 2 nhánh mỗi 0.4s
  // Phase 3: xoắn ốc 3 nhánh mỗi 0.25s + vòng 8 viên mỗi 5s
  static constexpr float PROJ_INTERVAL_P1 = 3.0f;
  static constexpr float PROJ_INTERVAL_P2 = 0.4f;
  static constexpr float PROJ_INTERVAL_P3 = 0.25f;
  static constexpr float BURST_INTERVAL_P3 = 5.0f;
  static constexpr float PROJ_SPEED = 180.f;
  static constexpr float PROJ_DAMAGE = 2.f;

  // ── Summon intervals ─────────────────────────────────────
  static constexpr float SUMMON_INTERVAL_P1 = 15.f;  // 5 FlyEye
  static constexpr float SUMMON_INTERVAL_P2 = 20.f;  // Skeleton
  static constexpr float SUMMON_INTERVAL_P3 = 10.f;  // Slime

  // ── AoE pulse (Phase 3) ──────────────────────────────────
  static constexpr float AOE_INTERVAL = 3.f;
  static constexpr float AOE_RADIUS = 180.f;
  static constexpr int AOE_DAMAGE = 3;
  static constexpr float AOE_KB_FORCE = 280.f;

  // ────────────────────────────────────────────────────────
  explicit DemonLord(sf::Vector2f pos, int maxHp = 1000) {
    typeId_ = "final_boss";
    pos_ = pos;
    hp_ = maxHp;
    maxHp_ = maxHp;
    alive_ = true;
    dead_ = false;
    attackDamage_ = MELEE_DAMAGE;
    attackRange_ = ATTACK_RANGE;
    attackCooldown_ = ATTACK_COOLDOWN;
    expValue_ = 0;  // WIN condition, không cộng EXP
    setIsBoss(true);

    speed_ = SPEED_P1;
    phase_ = DemonPhase::Phase1;
  }

  static bool loadTextures() {
    if (texturesLoaded_) return true;
    bool ok = texRun_.loadFromFile("hinh anh\\demonlord.png");
    ok &= texAttack_.loadFromFile("hinh anh\\demonlord.png");
    if (ok) texturesLoaded_ = true;
    return ok;
  }

  // ════════════════════════════════════════════════════════
  //  update
  // ════════════════════════════════════════════════════════
  void update(float dt, sf::Vector2f playerPos) override {
    if (dead_) return;

    flipX_ = (playerPos.x > pos_.x);
    updatePhase();

    // ── Chase / Attack ───────────────────────────────────
    if (alive_) {
      sf::Vector2f diff = playerPos - pos_;
      float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

      if (dist > ATTACK_RANGE) {
        pos_ += (diff / dist) * speed_ * dt;
        if (state_ != DemonState::Chase) setState(DemonState::Chase);
      } else {
        if (state_ != DemonState::Attack) setState(DemonState::Attack);
        tickAttack(dt, playerPos);
      }

      // ── Bắn đạn theo phase ──────────────────────────
      projTimer_ += dt;
      float projInterval = (phase_ == DemonPhase::Phase1)   ? PROJ_INTERVAL_P1
                           : (phase_ == DemonPhase::Phase2) ? PROJ_INTERVAL_P2
                                                            : PROJ_INTERVAL_P3;
      if (projTimer_ >= projInterval) {
        projTimer_ = 0.f;
        fireProjectiles(playerPos);
      }
      if (phase_ == DemonPhase::Phase3) {
        burstTimer_ += dt;
        if (burstTimer_ >= BURST_INTERVAL_P3) {
          burstTimer_ = 0.f;
          fireBurst(8);
        }
      }

      // ── Summon ──────────────────────────────────────
      summonTimer_ += dt;
      float summonInterval = summonIntervalForPhase();
      if (summonTimer_ >= summonInterval) {
        summonTimer_ = 0.f;
        fireSummons(playerPos);
      }

      // ── AoE pulse (Phase 3 only) ─────────────────────
      if (phase_ == DemonPhase::Phase3) {
        aoeTimer_ += dt;
        if (aoeTimer_ >= AOE_INTERVAL) {
          aoeTimer_ = 0.f;
          pendingAoe_.push_back({pos_, AOE_RADIUS, AOE_DAMAGE, AOE_KB_FORCE});
        }
      }
    }

    // ── Hit flash ────────────────────────────────────────
    if (hitFlash_) {
      flashTimer_ -= dt;
      if (flashTimer_ <= 0.f) hitFlash_ = false;
    }

    // ── Knockback (boss ít bị đẩy) ───────────────────────
    pos_ += kbVel_ * dt;
    kbVel_ *= 0.78f;

    // Cập nhật Aura nhấp nháy dựa trên Delta Time để không bị phụ thuộc FPS
    glowT_ += dt * 3.0f;

    advanceAnim(dt);
  }

  // ════════════════════════════════════════════════════════
  //  draw
  // ════════════════════════════════════════════════════════
  void draw(sf::RenderTarget& target) const override {
    if (dead_) return;

    // Aura màu theo phase
    float pulse = 0.5f + 0.5f * std::sin(glowT_);
    sf::Color auraCol = auraColorForPhase();
    sf::CircleShape aura(HIT_RADIUS * 1.5f);
    aura.setFillColor(sf::Color(auraCol.r, auraCol.g, auraCol.b,
                                static_cast<uint8_t>(20 + pulse * 30)));
    aura.setOrigin({HIT_RADIUS * 1.5f, HIT_RADIUS * 1.5f});
    aura.setPosition(pos_);
    sf::RenderStates rs;
    rs.blendMode = sf::BlendAdd;
    target.draw(aura, rs);

    // Sprite
    const sf::Texture& tex =
        (state_ == DemonState::Attack) ? texAttack_ : texRun_;
    sf::Sprite spr(tex);
    spr.setTextureRect(sf::IntRect(
        {(frame_ % currentFrameCount()) * FRAME_W, 0}, {FRAME_W, FRAME_H}));
    spr.setScale({flipX_ ? -SCALE : SCALE, SCALE});
    spr.setOrigin({FRAME_W / 2.f, FRAME_H / 2.f});
    spr.setPosition(pos_);
    if (hitFlash_) spr.setColor(sf::Color(255, 100, 100, 255));
    target.draw(spr);

    drawHpBar(target);
  }

  void drawDebug(sf::RenderTarget& target) const override {
    sf::CircleShape c(HIT_RADIUS);
    c.setFillColor(sf::Color(255, 0, 0, 35));
    c.setOutlineColor(sf::Color::Red);
    c.setOutlineThickness(1.f);
    c.setOrigin({HIT_RADIUS, HIT_RADIUS});
    c.setPosition(pos_);
    target.draw(c);

    // AoE range (phase 3)
    if (phase_ == DemonPhase::Phase3) {
      sf::CircleShape ar(AOE_RADIUS);
      ar.setFillColor(sf::Color(255, 80, 0, 18));
      ar.setOutlineColor(sf::Color(255, 120, 0, 80));
      ar.setOutlineThickness(1.f);
      ar.setOrigin({AOE_RADIUS, AOE_RADIUS});
      ar.setPosition(pos_);
      target.draw(ar);
    }
  }

  // ── IMonster ─────────────────────────────────────────────
  void takeHit(int damage) override {
    if (!alive_) return;
    hitFlash_ = true;
    flashTimer_ = 0.12f;
    hp_ -= damage;
    if (hp_ <= 0) {
      hp_ = 0;
      alive_ = false;
      dead_ = true;
      killedFlag_ = true;  // ← Game.cpp đọc qua isKilledFlag()
      markKilled();        // → WaveManager nhận isFinalBossDefeated() = true
    }
  }

  bool overlapsPoint(sf::Vector2f pt, float radius) const override {
    if (dead_) return false;
    sf::Vector2f d = pt - pos_;
    float minD = HIT_RADIUS + radius;
    return (d.x * d.x + d.y * d.y) < minD * minD;
  }

  void applyKnockback(sf::Vector2f dir, float force) {
    kbVel_ += dir * (force * 0.12f);  // boss ít bị đẩy
  }

  // ── Pending outputs cho Game.cpp poll ────────────────────
  const std::vector<BossProjectile>& getPendingProjectiles() const {
    return pendingProj_;
  }
  void clearProjectiles() { pendingProj_.clear(); }

  const std::vector<SummonRequest>& getPendingSummons() const {
    return pendingSummons_;
  }
  void clearSummons() { pendingSummons_.clear(); }

  const std::vector<AoePulse>& getPendingAoe() const { return pendingAoe_; }
  bool hasPendingAoe() const { return !pendingAoe_.empty(); }
  void clearAoe() { pendingAoe_.clear(); }

  DemonPhase getPhase() const { return phase_; }

  // ── Methods được Game.cpp poll ────────────────────────────
  int getAndResetPendingMelee() {
    int dmg = pendingMeleeDamage_;
    pendingMeleeDamage_ = 0;
    return dmg;
  }

  // Trả về true đúng 1 frame ngay khi boss vừa chết
  bool isKilledFlag() {
    if (killedFlag_) {
      killedFlag_ = false;
      return true;
    }
    return false;
  }

  bool isFinalBoss() const { return true; }

  // ── Debug / Demo: Ép Phase ──────────────────────────────
  void debugForcePhase(int phaseNum) {
    if (phaseNum == 1) {
      hp_ = maxHp_;  // Đầy máu
    } else if (phaseNum == 2) {
      hp_ = static_cast<int>(maxHp_ * 0.59f);  // < 60% máu
    } else if (phaseNum == 3) {
      hp_ = static_cast<int>(maxHp_ * 0.29f);  // < 30% máu
    }
    if (hp_ <= 0) hp_ = 1;  // Safeguard để boss không chết ngay
  }

 private:
  // ── tickAttack: override để tích vào pendingMeleeDamage_ ─
  // IMonster::tickAttack gọi addPendingDamage() → player nhận 2 lần.
  // DemonLord dùng kênh riêng để Game.cpp poll qua getAndResetPendingMelee().
  void tickAttack(float dt, sf::Vector2f /*playerPos*/) {
    attackTimer_ += dt;
    if (attackTimer_ >= ATTACK_COOLDOWN) {
      attackTimer_ = 0.f;
      pendingMeleeDamage_ += MELEE_DAMAGE;
    }
  }

  // ── Phase transition ─────────────────────────────────────
  void updatePhase() {
    float ratio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
    DemonPhase newPhase;
    if (ratio > 0.60f)
      newPhase = DemonPhase::Phase1;
    else if (ratio > 0.30f)
      newPhase = DemonPhase::Phase2;
    else
      newPhase = DemonPhase::Phase3;

    if (newPhase != phase_) {
      phase_ = newPhase;
      speed_ = (phase_ == DemonPhase::Phase1)   ? SPEED_P1
               : (phase_ == DemonPhase::Phase2) ? SPEED_P2
                                                : SPEED_P3;
      summonTimer_ = 0.f;
      projTimer_ = 0.f;
      burstTimer_ = 0.f;
      spiralAngle_ = 0.f;
      aoeTimer_ = 0.f;
    }
  }

  float summonIntervalForPhase() const {
    switch (phase_) {
      case DemonPhase::Phase1:
        return SUMMON_INTERVAL_P1;
      case DemonPhase::Phase2:
        return SUMMON_INTERVAL_P2;
      case DemonPhase::Phase3:
        return SUMMON_INTERVAL_P3;
    }
    return SUMMON_INTERVAL_P1;
  }

  sf::Color auraColorForPhase() const {
    switch (phase_) {
      case DemonPhase::Phase1:
        return {55, 138, 221};  // xanh dương
      case DemonPhase::Phase2:
        return {239, 159, 39};  // vàng cam
      case DemonPhase::Phase3:
        return {226, 75, 74};  // đỏ
    }
    return {200, 40, 40};
  }

  // ── Pattern bắn đạn theo phase ───────────────────────────
  void fireProjectiles(sf::Vector2f /*playerPos*/) {
    const float PI = 3.14159265f;

    if (phase_ == DemonPhase::Phase1) {
      // Vòng 6 viên, xoay nhẹ mỗi lần
      for (int i = 0; i < 6; ++i) {
        float angle = spiralAngle_ + (float(i) / 6) * 2.f * PI;
        sf::Vector2f dir = {std::cos(angle), std::sin(angle)};
        pendingProj_.push_back({pos_, dir, PROJ_SPEED, PROJ_DAMAGE});
      }
      spiralAngle_ += 0.5f;

    } else if (phase_ == DemonPhase::Phase2) {
      // Xoắn ốc 4 nhánh chữ thập (Đã fix từ 2 -> 4 để đúng với thiết kế)
      for (int i = 0; i < 4; ++i) {
        float angle = spiralAngle_ + float(i) * (PI / 2.f);
        sf::Vector2f dir = {std::cos(angle), std::sin(angle)};
        pendingProj_.push_back({pos_, dir, PROJ_SPEED, PROJ_DAMAGE});
      }
      spiralAngle_ += 0.3f;

    } else {
      // Phase 3: xoắn ốc 3 nhánh
      for (int i = 0; i < 3; ++i) {
        float angle = spiralAngle_ + float(i) * (2.f * PI / 3.f);
        sf::Vector2f dir = {std::cos(angle), std::sin(angle)};
        pendingProj_.push_back({pos_, dir, PROJ_SPEED + 30.f, PROJ_DAMAGE});
      }
      spiralAngle_ += 0.2f;
    }
  }

  // ── Vòng N viên toả đều (Phase 3 burst) ─────────────────
  void fireBurst(int count) {
    const float PI = 3.14159265f;
    for (int i = 0; i < count; ++i) {
      float angle = float(i) / float(count) * 2.f * PI;
      sf::Vector2f dir = {std::cos(angle), std::sin(angle)};
      pendingProj_.push_back({pos_, dir, PROJ_SPEED * 0.85f, PROJ_DAMAGE});
    }
  }

  // ── Triệu hồi theo phase ─────────────────────────────────
  void fireSummons(sf::Vector2f playerPos) {
    const float PI = 3.14159265f;
    switch (phase_) {
      case DemonPhase::Phase1:
        // 5 FlyEye quanh boss
        for (int i = 0; i < 5; ++i) {
          float angle = (float(i) / 5) * 2.f * PI;
          sf::Vector2f offset = {std::cos(angle) * 200.f,
                                 std::sin(angle) * 200.f};
          pendingSummons_.push_back({"flyeye", pos_ + offset});
        }
        break;
      case DemonPhase::Phase2:
        // 3 Zombie ngẫu nhiên (Đã đổi do trong Game.cpp không có register
        // skeleton)
        for (int i = 0; i < 3; ++i) {
          float angle = (float(i) / 3) * 2.f * PI;
          sf::Vector2f offset = {std::cos(angle) * 250.f,
                                 std::sin(angle) * 250.f};
          pendingSummons_.push_back({"zombie", pos_ + offset});
        }
        break;
      case DemonPhase::Phase3:
        // 4 Slime xung quanh
        for (int i = 0; i < 4; ++i) {
          float angle = (float(i) / 4) * 2.f * PI;
          sf::Vector2f offset = {std::cos(angle) * 180.f,
                                 std::sin(angle) * 180.f};
          pendingSummons_.push_back({"slime", pos_ + offset});
        }
        break;
    }
  }

  // ── Animation ────────────────────────────────────────────
  void setState(DemonState s) {
    if (state_ == s) return;
    state_ = s;
    frame_ = 0;
    timer_ = 0.f;
  }

  void advanceAnim(float dt) {
    timer_ += dt;
    float ft =
        (state_ == DemonState::Attack) ? FRAME_TIME_ATTACK : FRAME_TIME_RUN;
    if (timer_ >= ft) {
      timer_ -= ft;
      frame_ = (frame_ + 1) % currentFrameCount();
    }
  }

  int currentFrameCount() const {
    return (state_ == DemonState::Attack) ? FRAMES_ATTACK : FRAMES_RUN;
  }

  // ── HP bar 3 màu theo phase ──────────────────────────────
  void drawHpBar(sf::RenderTarget& target) const {
    const float W = 140.f, H = 8.f;
    float ratio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
    float yb = pos_.y - FRAME_H * SCALE * 0.5f - 16.f;

    sf::RectangleShape bg({W, H});
    bg.setFillColor(sf::Color(30, 0, 0, 210));
    bg.setOrigin({W / 2.f, H / 2.f});
    bg.setPosition({pos_.x, yb});
    target.draw(bg);

    // Màu bar theo phase
    sf::Color barCol = (ratio > 0.60f)   ? sf::Color(55, 138, 221)
                       : (ratio > 0.30f) ? sf::Color(239, 159, 39)
                                         : sf::Color(226, 75, 74);

    sf::RectangleShape bar({W * ratio, H});
    bar.setFillColor(barCol);
    bar.setOrigin({W / 2.f, H / 2.f});
    bar.setPosition({pos_.x, yb});
    target.draw(bar);

    // Phase marker lines (tại 60% và 30%)
    auto drawMarker = [&](float pct) {
      sf::RectangleShape mk({1.5f, H + 4.f});
      mk.setFillColor(sf::Color(255, 255, 255, 120));
      mk.setOrigin({0.75f, (H + 4.f) / 2.f});
      mk.setPosition({pos_.x - W / 2.f + W * pct, yb});
      target.draw(mk);
    };
    drawMarker(0.60f);
    drawMarker(0.30f);

    sf::RectangleShape border({W + 2.f, H + 2.f});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(255, 255, 255, 100));
    border.setOutlineThickness(0.5f);
    border.setOrigin({(W + 2.f) / 2.f, (H + 2.f) / 2.f});
    border.setPosition({pos_.x, yb});
    target.draw(border);
  }

  // ── State ────────────────────────────────────────────────
  DemonPhase phase_ = DemonPhase::Phase1;
  DemonState state_ = DemonState::Chase;
  float speed_ = SPEED_P1;

  int frame_ = 0;
  float timer_ = 0.f;
  bool flipX_ = false;
  bool hitFlash_ = false;
  float flashTimer_ = 0.f;

  float projTimer_ = 0.f;
  float burstTimer_ = 0.f;
  float spiralAngle_ = 0.f;
  float summonTimer_ = 0.f;
  float aoeTimer_ = 0.f;

  sf::Vector2f kbVel_ = {};
  float glowT_ = 0.f;
  std::vector<BossProjectile> pendingProj_;
  std::vector<SummonRequest> pendingSummons_;
  std::vector<AoePulse> pendingAoe_;
  int pendingMeleeDamage_ = 0;
  bool killedFlag_ = false;
  float attackTimer_ = 0.f;
  inline static sf::Texture texRun_;
  inline static sf::Texture texAttack_;
  inline static bool texturesLoaded_ = false;
};
