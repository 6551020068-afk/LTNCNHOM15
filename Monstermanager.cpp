#include "MonsterManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Camera.hpp"
#include "Constants.hpp"

using namespace Constants;

sf::Vector2f MonsterManager::randomSpawnPos(sf::Vector2f playerPos,
                                            const Camera& cam) const {
  const sf::View& view = cam.getView();
  sf::Vector2f half = view.getSize() / 2.f;
  constexpr float MARGIN = 80.f;

  float left = playerPos.x - half.x - MARGIN;
  float right = playerPos.x + half.x + MARGIN;
  float top = playerPos.y - half.y - MARGIN;
  float bottom = playerPos.y + half.y + MARGIN;

  float mapW = MAP_WIDTH * TILE_RENDER_W;
  float mapH = MAP_HEIGHT * TILE_RENDER_H;
  left = std::max(10.f, left);
  right = std::min(mapW - 10.f, right);
  top = std::max(10.f, top);
  bottom = std::min(mapH - 10.f, bottom);

  int side = std::rand() % 4;
  float x, y;
  switch (side) {
    case 0:
      x = left + (float)(std::rand() % (int)(right - left + 1));
      y = top;
      break;
    case 1:
      x = left + (float)(std::rand() % (int)(right - left + 1));
      y = bottom;
      break;
    case 2:
      x = left;
      y = top + (float)(std::rand() % (int)(bottom - top + 1));
      break;
    default:
      x = right;
      y = top + (float)(std::rand() % (int)(bottom - top + 1));
      break;
  }
  return {x, y};
}

void MonsterManager::spawnInitial(const Camera& cam, int count) {
  sf::Vector2f center = cam.getView().getCenter();
  for (int i = 0; i < count; ++i) {
    if (waveEntries_.empty()) break;
    spawnOne(waveEntries_[0].id, center, cam, false);
  }
}

void MonsterManager::spawnOne(const std::string& id, sf::Vector2f playerPos,
                              const Camera& cam, bool isBoss) {
  auto it = factories_.find(id);
  if (it == factories_.end()) return;
  sf::Vector2f pos = randomSpawnPos(playerPos, cam);
  auto m = it->second(pos);
  if (isBoss) m->setIsBoss(true);
  if (currentHpScale_ > 1.f) m->scaleHp(currentHpScale_);
  monsters_.push_back(std::move(m));
}

std::vector<KillInfo> MonsterManager::update(float dt, sf::Vector2f playerPos,
                                             const Camera& cam) {
  gameTime_ += dt;

  // ── 1. Quota-based spawning ───────────────────────────────
  // Chỉ bọc phần SPAWN trong if, các bước sau luôn chạy
  if (aliveCount() < maxMonstersCap_) {
    for (auto& entry : waveEntries_) {
      spawnTimers_[entry.id] += dt;
      // spawnInterval = 0.0f → spawn mỗi frame (từ phút 8 trở đi)
      if (spawnTimers_[entry.id] < entry.spawnInterval) continue;
      spawnTimers_[entry.id] = 0.f;

      int alive = countAliveOfType(entry.id);
      if (alive < entry.minCount) {
        int toSpawn = std::min(entry.minCount - alive, 3);
        for (int i = 0; i < toSpawn && aliveCount() < maxMonstersCap_; ++i)
          spawnOne(entry.id, playerPos, cam, entry.isBoss);
      } else {
        if (aliveCount() < maxMonstersCap_)
          spawnOne(entry.id, playerPos, cam, entry.isBoss);
      }
    }
  }  // ← đóng if TẠI ĐÂY, chỉ bọc phần spawn

  // ── 2. Despawn (thường) + Boss teleport ── luôn chạy ─────
  for (auto& m : monsters_) {
    if (m->isDead()) continue;
    sf::Vector2f d = m->getPosition() - playerPos;
    float dist = std::sqrt(d.x * d.x + d.y * d.y);

    if (m->isBoss()) {
      if (dist > BOSS_TELEPORT_DISTANCE)
        m->setPosition(randomSpawnPos(playerPos, cam));
    } else {
      if (dist > DESPAWN_DISTANCE) m->despawn();
    }
  }

  // ── 3. AI update ── luôn chạy ────────────────────────────
  for (auto& m : monsters_) {
    if (m->isDead()) continue;
    m->update(dt, playerPos);
  }

  // ── 4. Separation ── luôn chạy ───────────────────────────
  applySeparation();

  // ── 5. Thu thập kills — CHỈ lấy quái bị giết thật ───────
  std::vector<KillInfo> kills;
  for (auto& m : monsters_) {
    if (m->isDead() && m->isKilledByPlayer())
      kills.push_back(
          {m->getPosition(), m->getExpValue(), m->getTypeId(), m->isBoss()});
  }

  // ── 6. Dọn quái chết ── luôn chạy ────────────────────────
  monsters_.erase(std::remove_if(monsters_.begin(), monsters_.end(),
                                 [](const std::unique_ptr<IMonster>& m) {
                                   return m->isDead();
                                 }),
                  monsters_.end());
  return kills;
}

void MonsterManager::applySeparation() {
  constexpr float DESIRED_DIST = 80.f;
  constexpr float FORCE_MAX = 140.f;

  for (size_t i = 0; i < monsters_.size(); ++i) {
    if (!monsters_[i]->isAlive()) continue;
    for (size_t j = i + 1; j < monsters_.size(); ++j) {
      if (!monsters_[j]->isAlive()) continue;
      sf::Vector2f d =
          monsters_[i]->getPosition() - monsters_[j]->getPosition();
      float dist2 = d.x * d.x + d.y * d.y;
      if (dist2 >= DESIRED_DIST * DESIRED_DIST || dist2 < 0.0001f) continue;
      float dist = std::sqrt(dist2);
      float t = 1.f - (dist / DESIRED_DIST);
      float forceMag = FORCE_MAX * t * t;
      sf::Vector2f push = (d / dist) * forceMag;
      monsters_[i]->addSeparationForce(push);
      monsters_[j]->addSeparationForce(-push);
    }
  }

  constexpr float DEFAULT_MAX_SPEED = 90.f;
  constexpr float DT_FLUSH = 1.f / 60.f;
  for (auto& m : monsters_) {
    if (!m->isAlive()) continue;
    m->flushSeparation(DT_FLUSH, DEFAULT_MAX_SPEED);
  }
}

void MonsterManager::draw(sf::RenderTarget& target) const {
  for (auto& m : monsters_)
    if (!m->isDead()) m->draw(target);
}

void MonsterManager::drawDebug(sf::RenderTarget& target) const {
  for (auto& m : monsters_)
    if (!m->isDead()) m->drawDebug(target);
}

std::vector<IMonster*> MonsterManager::getLiveMonsters() {
  std::vector<IMonster*> result;
  result.reserve(monsters_.size());
  for (auto& m : monsters_)
    if (!m->isDead()) result.push_back(m.get());
  return result;
}