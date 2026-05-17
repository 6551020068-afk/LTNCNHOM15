#include "BulletManager.hpp"

#include <algorithm>

#include "MonsterManager.hpp"  // KillInfo

void BulletManager::spawnFromShots(const std::vector<ShotData>& shots) {
  for (const auto& shot : shots)
    for (const auto& dir : shot.directions) {
      bullets_.emplace_back(shot.origin, dir, shot.speed, shot.radius,
                            shot.lifetime, shot.damageMultiplier);
    }
}

std::vector<KillInfo> BulletManager::update(float dt,
                                            std::vector<IMonster*>& monsters,
                                            int damagePerBullet) {
  // Cập nhật vị trí đạn
  for (auto& b : bullets_) b.update(dt);

  // Kiểm tra va chạm đạn ↔ quái
  std::vector<KillInfo> recentlyKilled;
  for (auto& b : bullets_) {
    if (b.isDead()) continue;
    for (auto* m : monsters) {
      if (!m->isAlive()) continue;
      if (b.hits(m->getPosition(), 18.f)) {
        m->takeHit(damagePerBullet * b.getDamage());
        b.kill();
        if (!m->isAlive()) {
          ++killCount_;
          recentlyKilled.push_back(
              {m->getPosition(), m->getExpValue(), m->getTypeId()});
        }
        break;
      }
    }
  }

  // Dọn đạn dead
  bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                [](const Bullet& b) { return b.isDead(); }),
                 bullets_.end());

  return recentlyKilled;
}

void BulletManager::draw(sf::RenderTarget& target) const {
  for (auto& b : bullets_) b.draw(target);
}