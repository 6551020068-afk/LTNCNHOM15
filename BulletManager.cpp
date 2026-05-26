#include "BulletManager.hpp"

#include <algorithm>

#include "MonsterManager.hpp"  // Để sử dụng struct KillInfo
#include "SoundManager.hpp"

// Tạo đạn từ dữ liệu các lần bắn (ShotData) được cung cấp
void BulletManager::spawnFromShots(const std::vector<ShotData>& shots) {
  for (const auto& shot : shots) {
    for (const auto& dir : shot.directions) {
      // Khởi tạo và thêm đạn mới vào danh sách
      bullets_.emplace_back(shot.origin, dir, shot.speed, shot.radius,
                            shot.lifetime, shot.damageMultiplier);
    }
  }
}

// Cập nhật trạng thái đạn, xử lý va chạm với quái vật và trả về danh sách quái bị hạ gục
std::vector<KillInfo> BulletManager::update(float dt,
                                            std::vector<IMonster*>& monsters,
                                            int damagePerBullet) {
  // 1. Cập nhật vị trí của tất cả đạn theo thời gian dt
  for (auto& b : bullets_) {
    b.update(dt);
  }

  std::vector<KillInfo> recentlyKilled;

  // 2. Kiểm tra va chạm giữa đạn và quái vật
  for (auto& b : bullets_) {
    if (b.isDead()) continue; // Bỏ qua đạn đã hết hiệu lực

    for (auto* m : monsters) {
      if (!m->isAlive()) continue; // Bỏ qua quái vật đã chết

      // Kiểm tra va chạm: Giả sử bán kính xét va chạm của quái vật là 18.f
      if (b.hits(m->getPosition(), 18.f)) {
        // Gây sát thương lên quái vật
        m->takeHit(damagePerBullet * b.getDamage());
        
        // Phát âm thanh khi trúng quái
        SoundManager::get().playVaried(SoundManager::SFX::HIT_MONSTER);
        
        // Vô hiệu hóa viên đạn sau khi trúng mục tiêu
        b.kill();

        // Nếu quái vật chết sau đòn đánh, lưu lại thông tin để rớt kinh nghiệm
        if (!m->isAlive()) {
          recentlyKilled.push_back(
              {m->getPosition(), m->getExpValue(), m->getTypeId()});
        }
        break; // Một viên đạn chỉ trúng một quái vật, nên thoát vòng lặp quái
      }
    }
  }

  // 3. Dọn dẹp các viên đạn đã hết hiệu lực (hết thời gian sống hoặc đã trúng mục tiêu)
  bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                [](const Bullet& b) { return b.isDead(); }),
                 bullets_.end());

  return recentlyKilled;
}

// Vẽ tất cả đạn đang có lên màn hình
void BulletManager::draw(sf::RenderTarget& target) const {
  for (const auto& b : bullets_) {
    b.draw(target);
  }
}
