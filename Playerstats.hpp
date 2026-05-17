#pragma once
// ════════════════════════════════════════════════════════════
//  PlayerStats.hpp  —  Số liệu thuần của player
//
//  Chỉ lưu số liệu. Logic skill nằm ở SkillManager.
//  Logic quái nằm ở MonsterManager / IMonster.
// ════════════════════════════════════════════════════════════
struct PlayerStats {
  // ── Combat ──────────────────────────────────────────────
  float attackSpeed = 2.f;  // shots/giây — SkillManager dùng
  int damage = 1;           // BulletManager dùng khi check hit

  // ── HP ──────────────────────────────────────────────────
  int maxHp = 100;
  int hp = 100;
  float fractionalHp = 0.f;  // Lưu lượng máu lẻ tích lũy

  // ── Upgrade tracking ────────────────────────────────────
  int attackSpeedLevel = 1;
  int damageLevel = 1;

  bool coneShot = false;  // Biến kiểm tra trạng thái kỹ năng
  int coneShotLevel = 0;

  int regenLevel = 0;      // Cấp độ hồi máu
  float regenTimer = 0.f;  // bộ đếm thời gian hồi máu, reset sau mỗi lần hồi
  static constexpr float ATTACK_SPEED_MAX = 20.f;
  static constexpr int LEVEL_MAX = 10;

  // Cooldown tương ứng với attackSpeed
  float fireInterval() const { return 1.f / attackSpeed; }

  bool upgradeAttackSpeed() {
    if (attackSpeedLevel >= LEVEL_MAX) return false;
    ++attackSpeedLevel;
    attackSpeed = 1.f + (attackSpeedLevel - 1) * 0.3f;
    if (attackSpeed > ATTACK_SPEED_MAX) attackSpeed = ATTACK_SPEED_MAX;
    return true;
  }

  bool upgradeDamage() {
    if (damageLevel >= LEVEL_MAX) return false;
    ++damageLevel;
    damage = damageLevel;
    return true;
  }

  // Hàm hồi máu chuẩn xác, hỗ trợ số thập phân
  void heal(float amount) {
    fractionalHp += amount;
    if (fractionalHp >= 1.f) {
      int healInt = static_cast<int>(fractionalHp);
      hp = std::min(hp + healInt, maxHp);
      fractionalHp -= healInt;
    }
  }

  void takeDamage(int d) {
    hp -= d;
    if (hp < 0) hp = 0;
  }

  bool isDead() const { return hp <= 0; }
};