#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  skillSachthanh.hpp  —  Sách Thánh (Holy Bible / King Bible)
//
//  Cơ chế:
//    • N quyển sách bay orbit quanh nhân vật, liên tục xoay
//    • Có COOLDOWN thực sự: mỗi "vòng quét" tính là 1 lần dùng skill,
//      sau đó chờ cooldown trước khi vòng tiếp theo gây damage
//    • Mỗi quái có per-monster hit-cooldown → không bị spam
//    • Level 1-5: normal → gọi upgrade()
//    • Level 6 (isMaxLevel): gọi evolve() → "Thanh Kinh Quy" (Unholy Vespers)
//      * Thêm sách, damage ×2, tốc độ tăng, hitbox lớn hơn
//
//  Trả về std::vector<BibleHit> — Game xử lý giống garlic
// ════════════════════════════════════════════════════════════════════════════

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

static constexpr float BIBLE_PI = 3.14159265f;

// ── Kết quả 1 lần hit ────────────────────────────────────────────────────────
struct BibleHit {
  void* monster = nullptr;  // IMonster* — cast bên ngoài
  sf::Vector2f monsterPos;
  sf::Vector2f knockbackDir;
  float damage = 0.f;
  float knockback = 90.f;
};

// ════════════════════════════════════════════════════════════════════════════
//  HolyBibleSkill
// ════════════════════════════════════════════════════════════════════════════
class HolyBibleSkill {
 public:
  HolyBibleSkill() = default;

  // ── Texture ──────────────────────────────────────────────────────────────
  static void loadTexture(
      const std::string& path = "hinh anh\\Sprite-King_Bible.png") {
    texLoaded_ = tex_.loadFromFile(path);
    if (texLoaded_) tex_.setSmooth(false);
  }
  // Texture riêng cho dạng evolved (Thanh Kinh Quy) — tuỳ chọn
  static void loadEvolvedTexture(
      const std::string& path = "hinh anh\\Sprite-Unholy_Vespers.png") {
    texEvoLoaded_ = texEvo_.loadFromFile(path);
    if (texEvoLoaded_) texEvo_.setSmooth(false);
  }

  // ── isUnlocked / level helpers ───────────────────────────────────────────
  bool isUnlocked() const { return unlocked_; }
  int getLevel() const { return level_; }  // 0 = base khi mới unlock
  int getMaxLevel() const { return maxLevel_; }
  bool isMaxLevel() const { return level_ >= maxLevel_; }
  bool isEvolved() const { return evolved_; }
  int getBooks() const { return books_; }

  // ── Cooldown helpers (dùng cho SkillManager::applyUpgrade AttackSpeed) ──
  float getCooldown() const { return cooldownMax_; }
  void setCooldown(float v) {
    cooldownMax_ = std::max(v, 0.3f);
    cooldownTimer_ = std::min(cooldownTimer_, cooldownMax_);
  }

  // ── upgrade: lần đầu unlock; sau đó level up ─────────────────────────────
  bool upgrade() {
    if (!unlocked_) {
      unlocked_ = true;
      applyLevelStats();
      return true;
    }
    if (level_ >= maxLevel_) return false;  // dùng evolve()
    ++level_;
    applyLevelStats();
    return true;
  }

  // ── evolve: gọi khi level đã MAX → biến thành Thanh Kinh Quy ────────────
  bool evolve() {
    if (!unlocked_ || !isMaxLevel() || evolved_) return false;
    evolved_ = true;
    evoFlashTimer_ = 1.2f;  // flash 1.2 giây sau khi evolve
    books_ = std::min(books_ + 2, 7);
    damageMult_ *= 2.0f;
    rotSpeed_ +=
        3.5f;  // Tăng mạnh tốc độ xoay khi Evolve (có thể thay đổi số này)
    hitboxMonster_ = 26.f;
    orbitRadius_ = std::max(orbitRadius_ + 60.f,
                            160.f);  // Tăng mạnh phạm vi (range) khi Evolve
    cooldownMax_ = std::max(cooldownMax_ - 0.4f, 0.25f);
    return true;
  }

  // ── update (gọi mỗi frame) ────────────────────────────────────────────────
  //   Trả về danh sách BibleHit trong frame này.
  //   Cơ chế cooldown:
  //     cooldownTimer_ đếm ngược.
  //     Khi = 0 → mở "cửa sổ tấn công" (attackWindow_) trong ATTACK_WINDOW giây
  //     Trong cửa sổ đó sách va chạm quái → gây damage.
  //     Sau khi hết cửa sổ → reset cooldown.
  std::vector<BibleHit> update(
      sf::Vector2f playerPos, float dt,
      const std::vector<std::pair<sf::Vector2f, void*>>& monsters,
      float baseDamage) {
    if (!unlocked_) return {};
    // Xoay liên tục (kể cả khi ẩn để không bị giật khi xuất hiện lại)
    angle_ += rotSpeed_ * dt;
    if (evoFlashTimer_ > 0.f) evoFlashTimer_ -= dt;
    // ── Scale animation ──────────────────────────────────────────────────
    if (scaleDir_ != 0) {
      bookScale_ += scaleDir_ * SCALE_SPEED * dt;
      if (scaleDir_ == -1 && bookScale_ <= 0.f) {
        // Thu nhỏ xong → ẩn hẳn, bắt đầu đếm cooldown
        bookScale_ = 0.f;
        scaleDir_ = 0;
        booksHidden_ = true;
        cooldownTimer_ = cooldownMax_;
        hitTimers_.clear();
      } else if (scaleDir_ == 1 && bookScale_ >= 1.f) {
        // Phóng to xong → bắt đầu tấn công
        bookScale_ = 1.f;
        scaleDir_ = 0;
        attacking_ = true;
        attackWindow_ = ATTACK_WINDOW;
      }
    }

    // ── Cooldown & attack window ─────────────────────────────────────────
    if (attacking_) {
      attackWindow_ -= dt;
      if (attackWindow_ <= 0.f) {
        // Hết attack window → bắt đầu thu nhỏ
        attacking_ = false;
        scaleDir_ = -1;  // kích hoạt thu nhỏ
      }
    } else if (booksHidden_) {
      // Đang ẩn → đếm cooldown
      cooldownTimer_ -= dt;
      if (cooldownTimer_ <= 0.f) {
        // Hết cooldown → bắt đầu phóng to
        booksHidden_ = false;
        scaleDir_ = 1;
      }
    }

    // Tính hit chỉ khi đang trong attack window
    if (!attacking_) return {};

    std::vector<BibleHit> hits;
    const float totalDmg = baseDamage * damageMult_;
    const float bookR = evolved_ ? 10.f : 8.f;

    for (int i = 0; i < books_; ++i) {
      float a = angle_ + (2.f * BIBLE_PI / books_) * i;
      sf::Vector2f bPos = playerPos + sf::Vector2f(orbitRadius_ * std::cos(a),
                                                   orbitRadius_ * std::sin(a));

      for (auto& [mPos, mPtr] : monsters) {
        if (!mPtr) continue;

        // Per-monster hit cooldown trong 1 attack window
        auto it = hitTimers_.find(mPtr);
        if (it != hitTimers_.end() && it->second > 0.f) {
          it->second -= dt;
          continue;
        }

        sf::Vector2f d = bPos - mPos;
        float thresh = bookR + hitboxMonster_;
        if (d.x * d.x + d.y * d.y < thresh * thresh) {
          hitTimers_[mPtr] = PER_MONSTER_COOLDOWN;

          BibleHit h;
          h.monster = mPtr;
          h.monsterPos = mPos;
          h.damage = totalDmg;
          h.knockback = knockbackForce_;

          sf::Vector2f kb = mPos - playerPos;
          float kl = std::sqrt(kb.x * kb.x + kb.y * kb.y);
          h.knockbackDir = (kl > 0.001f) ? kb / kl : sf::Vector2f{1.f, 0.f};
          hits.push_back(h);
        }
      }
    }
    return hits;
  }

  // ── draw (world-space) ────────────────────────────────────────────────────
  void draw(sf::RenderTarget& target, sf::Vector2f playerPos) const {
    if (!unlocked_) return;

    // Vòng orbit mờ — ẩn dần theo scale
    if (bookScale_ > 0.f) {
      sf::CircleShape orbit(orbitRadius_);
      orbit.setOrigin({orbitRadius_, orbitRadius_});
      orbit.setFillColor(sf::Color::Transparent);
      orbit.setOutlineColor(
          sf::Color(140, 180, 255, static_cast<uint8_t>(20 * bookScale_)));
      orbit.setOutlineThickness(1.f);
      orbit.setPosition(playerPos);
      target.draw(orbit);
    }

    for (int i = 0; i < books_; ++i) {
      float a = angle_ + (2.f * BIBLE_PI / books_) * i;
      sf::Vector2f pos = playerPos + sf::Vector2f(orbitRadius_ * std::cos(a),
                                                  orbitRadius_ * std::sin(a));

      // Không vẽ nếu hoàn toàn ẩn
      if (bookScale_ <= 0.f) continue;

      // Hào quang (scale theo bookScale_)
      float glowR = (evolved_ ? 24.f : 18.f) * bookScale_;
      sf::Color glowCol =
          evolved_ ? sf::Color(200, 80, 255, 50) : sf::Color(120, 170, 255, 40);

      // Flash khi trong attack window
      if (attacking_) {
        glowCol.a = static_cast<uint8_t>(glowCol.a +
                                         40 * std::abs(std::sin(angle_ * 3.f)));
      }

      sf::CircleShape glow(glowR);
      glow.setOrigin({glowR, glowR});
      glow.setFillColor(glowCol);
      glow.setPosition(pos);
      target.draw(glow);

      float bookRot = a * 180.f / BIBLE_PI + 90.f;
      const float s = bookScale_;  // shorthand
      // Dùng texture evolved nếu có, fallback về tex_ thường
      const bool useEvoTex = evolved_ && texEvoLoaded_;

      sf::Sprite spr(useEvoTex ? texEvo_ : tex_);
      spr.setScale({2.f * s, 2.f * s});
      spr.setOrigin({8.f, 8.f});
      spr.setPosition(pos);
      spr.setRotation(sf::degrees(bookRot));
      // Nếu evolved nhưng không có texture riêng → tô màu tím
      if (evolved_ && !texEvoLoaded_) spr.setColor(sf::Color(220, 160, 255));
      target.draw(spr);
    }

    // Cooldown arc — hiển thị trên sách đầu tiên
    drawCooldownArc(target, playerPos);

    // ── Flash burst khi vừa evolve ────────────────────────────────────────
    if (evoFlashTimer_ > 0.f) {
      const float t = evoFlashTimer_ / 1.2f;  // 1.0 → 0.0
      const float flashR = orbitRadius_ * 1.4f * (1.f - t) + 10.f;
      uint8_t alpha = static_cast<uint8_t>(200 * t);
      sf::CircleShape flash(flashR);
      flash.setOrigin({flashR, flashR});
      flash.setFillColor(
          sf::Color(220, 160, 255, static_cast<uint8_t>(alpha / 3)));
      flash.setOutlineColor(sf::Color(255, 200, 255, alpha));
      flash.setOutlineThickness(3.f);
      flash.setPosition(playerPos);
      target.draw(flash);
    }
  }

  void drawDebug(sf::RenderTarget& target, sf::Vector2f playerPos) const {
    if (!unlocked_) return;
    sf::CircleShape orb(orbitRadius_);
    orb.setOrigin({orbitRadius_, orbitRadius_});
    orb.setFillColor(sf::Color::Transparent);
    orb.setOutlineColor(attacking_ ? sf::Color(255, 200, 0, 160)
                                   : sf::Color(100, 255, 100, 80));
    orb.setOutlineThickness(1.5f);
    orb.setPosition(playerPos);
    target.draw(orb);
  }

  // ── Strings cho HUD / buildUpgradeOptions ────────────────────────────────
  std::string getUpgradeTitle() const {
    if (!unlocked_) return "SACH THANH";
    if (evolved_) return "THANH KINH QUY [EVO]";
    if (isMaxLevel()) return "SACH THANH [MAX - EVO?]";
    return "SACH THANH Lv" + std::to_string(level_ + 1);
  }

  std::string getUpgradeDesc() const {
    if (!unlocked_) return "Mo khoa: sach bay orbit, gay damage khi cham quai";
    if (evolved_)
      return "Da tien hoa: Thanh Kinh Quy. Damage x2, nhieu sach hon!";
    switch (level_) {
      case 0:
        return "+1 quyen sach (2 sach), vong orbit hinh thanh";
      case 1:
        return "+1 sach (3 sach), damage x1.2";
      case 2:
        return "Ban kinh tang +16, xoay nhanh hon";
      case 3:
        return "Cooldown giam 0.3s, hit thuong xuyen hon";
      case 4:
        return "+1 sach (5 sach), damage x1.3 — San sang EVO!";
      default:
        return "Da dat cap toi da — co the tien hoa!";
    }
  }

  // ── Getter cho cooldown display ──────────────────────────────────────────
  //   cooldownRatio = 0.0 (vừa hết CD) → 1.0 (vừa bắt đầu CD)
  float getCooldownRatio() const {
    if (!unlocked_) return 1.f;
    if (attacking_) return 0.f;
    if (!booksHidden_) return 0.f;  // đang thu/phóng → không hiện arc
    return cooldownMax_ > 0.f ? cooldownTimer_ / cooldownMax_ : 0.f;
  }
  bool isAttacking() const { return attacking_; }

 private:
  // ── Hằng số ──────────────────────────────────────────────────────────────
  static constexpr float ATTACK_WINDOW = 8.f;  // giây cửa sổ tấn công
  static constexpr float PER_MONSTER_COOLDOWN =
      0.25f;  // tránh hit spam cùng quái

  // ── Thống số runtime ─────────────────────────────────────────────────────
  int books_ = 1;
  float orbitRadius_ = 80.f;
  float rotSpeed_ = 2.2f;
  float cooldownMax_ = 5.0f;  // giây giữa các vòng tấn công
  float damageMult_ = 1.0f;
  float knockbackForce_ = 90.f;
  float hitboxMonster_ = 18.f;

  float angle_ = 0.f;
  float cooldownTimer_ = 0.f;  // đếm ngược trước khi tấn công
  float attackWindow_ = 0.f;   // đếm ngược cửa sổ tấn công
  bool attacking_ = true;      // bắt đầu ở trạng thái tấn công ngay lần đầu

  // ── Scale animation (thu nhỏ / phóng to khi cooldown) ───────────────────
  float bookScale_ = 1.f;     // scale hiện tại: 0.0 = ẩn, 1.0 = full
  int scaleDir_ = 0;          // -1 = đang thu nhỏ, 0 = dừng, +1 = đang phóng to
  bool booksHidden_ = false;  // đang ẩn (chờ cooldown hết mới phóng to)
  static constexpr float SCALE_SPEED = 4.f;  // tốc độ thay đổi scale (per giây)

  int level_ = 1;
  int maxLevel_ = 5;  // level 1-5= 5 lần upgrade; level 5 → evolve
  bool unlocked_ = false;
  bool evolved_ = false;
  float evoFlashTimer_ = 0.f;  // flash sáng khi vừa evolve (giây)

  std::unordered_map<void*, float> hitTimers_;

  // ── Texture dùng chung ───────────────────────────────────────────────────
  inline static sf::Texture tex_;
  inline static bool texLoaded_ = false;
  inline static sf::Texture texEvo_;  // texture khi evolved
  inline static bool texEvoLoaded_ = false;

  // ── Áp dụng chỉ số theo level ────────────────────────────────────────────
  void applyLevelStats() {
    switch (level_) {
      case 0:  // vừa unlock
        books_ = 1;
        cooldownMax_ = 2.0f;
        damageMult_ = 1.2f;
        orbitRadius_ = 100.f;  // Tầm xa cơ bản ban đầu (mặc định cũ là 75)
        rotSpeed_ = 3.f;
        break;
      case 1:
        books_ = std::min(books_ + 1, 4);
        break;
      case 2:
        books_ = std::min(books_ + 1, 4);
        damageMult_ += 0.4f;
        break;
      case 3:
        orbitRadius_ = std::min(orbitRadius_ + 25.f,
                                150.f);  // Tăng tầm bay xa hơn khi nâng cấp
        rotSpeed_ = std::min(rotSpeed_ + 0.4f, 4.0f);
        break;
      case 4:
        cooldownMax_ = std::max(cooldownMax_ - 0.3f, 2.5f);
        break;
      case 5:
        books_ = std::min(books_ + 1, 5);
        damageMult_ += 0.3f;
        break;
      default:
        break;
    }
  }

  // ── Vẽ arc cooldown nhỏ ở tâm orbit ─────────────────────────────────────
  void drawCooldownArc(sf::RenderTarget& target, sf::Vector2f playerPos) const {
    if (attacking_) return;  // không cần vẽ khi đang tấn công

    const float ratio = getCooldownRatio();  // 1.0 → 0.0
    const int SEGS = 32;
    const float arcR = 10.f;
    const float filled = (1.f - ratio) * 2.f * BIBLE_PI;  // góc đã "nạp"

    // Nền tối
    sf::CircleShape bg(arcR);
    bg.setOrigin({arcR, arcR});
    bg.setFillColor(sf::Color(0, 0, 0, 100));
    bg.setOutlineColor(sf::Color(80, 80, 80, 150));
    bg.setOutlineThickness(1.f);
    bg.setPosition(playerPos);
    target.draw(bg);

    // Phần đã nạp
    sf::VertexArray fan(sf::PrimitiveType::TriangleFan, SEGS + 2);
    fan[0].position = playerPos;
    fan[0].color = sf::Color(180, 230, 255, 200);
    for (int i = 0; i <= SEGS; ++i) {
      float a = -BIBLE_PI / 2.f + filled * i / SEGS;
      fan[i + 1].position =
          playerPos + sf::Vector2f(arcR * std::cos(a), arcR * std::sin(a));
      fan[i + 1].color = sf::Color(100, 190, 255, 180);
    }
    target.draw(fan);
  }
};