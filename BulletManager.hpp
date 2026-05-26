#pragma once
// ════════════════════════════════════════════════════════════
//  BulletManager.hpp  —  Quản lý đạn
//
//  Pipeline:
//    Game                SkillManager           BulletManager
//     │                       │                      │
//     ├─ player.getFacing() ──►                      │
//     │                       ├─ tryFire() ──────────►
//     │                       │   (ShotData)         │
//     │                                              ├─ spawn bullets
//     │                                              ├─ update bullets
//     │                                              ├─ check hits vs monsters
//     │◄──────────────────────────────── KillInfo ───┘
//
//  BulletManager KHÔNG biết gì về Skill. Nó chỉ nhận ShotData.
// ════════════════════════════════════════════════════════════
#include <vector>

#include "Bullet.hpp"
#include "IMonster.hpp"
#include "ISkill.hpp"

struct KillInfo;

class BulletManager {
 public:
  BulletManager() { Bullet::loadTextures(); }

  // Nhận ShotData từ SkillManager, spawn đạn tương ứng
  void spawnFromShots(const std::vector<ShotData>& shots);

  // Update đạn + kiểm tra va chạm với quái
  // damagePerBullet: lấy từ PlayerStats::damage
  std::vector<KillInfo> update(float dt, std::vector<IMonster*>& monsters,
                               int damagePerBullet);

  void draw(sf::RenderTarget& target) const;

 private:
  std::vector<Bullet> bullets_;
};