#pragma once
// ════════════════════════════════════════════════════════════
//  ISkill.hpp  —  Abstract base cho mọi kỹ năng của player
//
//  CÁCH THÊM SKILL MỚI:
//    1. Tạo class kế thừa ISkill
//    2. Override tryFire() — trả về ShotData khi sẵn sàng bắn
//    3. Đăng ký vào SkillManager::addSkill()
//
//  ISkill KHÔNG biết gì về Bullet hay BulletManager.
//  Nó chỉ trả về ShotData (origin + list hướng normalized).
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Một lần bắn = vị trí xuất phát + danh sách hướng (đã normalize)
struct ShotData {
  sf::Vector2f origin;
  std::vector<sf::Vector2f> directions;
  int damageMultiplier = 1;  // nhân với PlayerStats::damage

  // Thêm cấu hình vật lý để BulletManager biết cách tạo ra đạn phù hợp
  float speed = 599.f;
  float radius = 6.f;
  float lifetime = 3.f;
  int pierce = 0;  // (Tuỳ chọn) Số lần xuyên thấu
};
// ── Meta info để HUD hiển thị ───────────────────────────────
struct SkillInfo {
  std::string id;
  std::string displayName;
  std::string description;
  int level = 0;
  int maxLevel = 10;
};

// ════════════════════════════════════════════════════════════
class ISkill {
 public:
  explicit ISkill(std::string id, std::string name, float cooldown)
      : info_{std::move(id), std::move(name), "", 0, 10},
        cooldownMax_(cooldown),
        timer_(cooldown) {}  // sẵn sàng ngay từ đầu

  virtual ~ISkill() = default;

  // Gọi mỗi frame. Trả về ShotData nếu đến lượt bắn, rỗng nếu chưa.
  virtual std::vector<ShotData> tryFire(sf::Vector2f origin,
                                        sf::Vector2f facing, float dt) = 0;

  // Nâng cấp skill — trả về false nếu đã đạt max level
  virtual bool upgrade() { return false; }

  // ── Cooldown helpers (dùng trong subclass) ───────────────
  bool isReady() const { return timer_ >= cooldownMax_; }
  float getCooldown() const { return cooldownMax_; }
  void setCooldown(float cd) {
    cooldownMax_ = cd;
    if (timer_ > cd) timer_ = cd;
  }

  // ── Info ─────────────────────────────────────────────────
  const SkillInfo& getInfo() const { return info_; }
  const std::string& getId() const { return info_.id; }

 protected:
  void tickCooldown(float dt) {
    if (timer_ < cooldownMax_) timer_ += dt;
  }
  void resetCooldown() { timer_ = 0.f; }

  SkillInfo info_;
  float cooldownMax_;
  float timer_;
};