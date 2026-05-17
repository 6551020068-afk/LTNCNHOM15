#pragma once
// ════════════════════════════════════════════════════════════
//  MonsterManager.hpp  —  Quản lý toàn bộ quái vật
//
//  Cơ chế spawn (giống Vampire Survivors):
//    - Mỗi frame spawn theo quota của wave hiện tại
//    - Nếu alive < minCount của wave → spawn đến khi đủ quota
//    - Nếu alive >= minCount → spawn 1 con mỗi loại trong wave
//    - Nếu alive >= MAX_MONSTERS (300) → chỉ boss/map event mới spawn được
//    - Boss KHÔNG despawn khi player chạy xa, chỉ bị teleport lại màn hình
//
//  CÁCH THÊM QUÁI MỚI (chỉ 2 bước):
//    1. #include "MyNewMonster.hpp"
//    2. monsters_.registerFactory("mynew",
//           [](sf::Vector2f p){ return std::make_unique<MyNewMonster>(p); });
// ════════════════════════════════════════════════════════════

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "IMonster.hpp"

class Camera;

struct KillInfo {
  sf::Vector2f pos;
  int expValue;
  std::string typeId;
  bool isBoss = false;
};

using MonsterFactory = std::function<std::unique_ptr<IMonster>(sf::Vector2f)>;

struct WaveEntry {
  std::string id;
  int minCount;         // số tối thiểu phải có trong màn
  float spawnInterval;  // giây giữa mỗi lần manager check spawn loại này
  bool isBoss = false;
  float hpScale =
      1.f;  // nhân HP theo thời gian để quái không quá yếu cuối game
};

class MonsterManager {
 public:
  static constexpr float DESPAWN_DISTANCE = 1200.f;
  static constexpr float BOSS_TELEPORT_DISTANCE = 900.f;

  // ── Factory ─────────────────────────────────────────────
  void registerFactory(const std::string& id, MonsterFactory fn) {
    factories_[id] = std::move(fn);
  }

  // ── Wave config: WaveManager gọi mỗi khi chuyển wave ───
  void setWave(const std::vector<WaveEntry>& entries) {
    waveEntries_ = entries;
    spawnTimers_.clear();
    for (auto& e : waveEntries_) spawnTimers_[e.id] = 0.f;
  }

  // HP scale cho quái thường spawn từ wave (cập nhật mỗi frame từ WaveManager)
  void setHpScale(float scale) { currentHpScale_ = scale; }

  // Thiết lập giới hạn tối đa quái vật
  void setMaxMonsters(int max) { maxMonstersCap_ = max; }

  // ── Spawn tức thì (map event / boss) ────────────────────
  // isBoss = true → bỏ qua giới hạn MAX_MONSTERS
  // hpScale > 1.f → nhân HP sau khi factory tạo (dùng cho late-game scaling)
  bool spawnDirect(const std::string& id, sf::Vector2f pos, bool isBoss = false,
                   float hpScale = 1.f) {
    if (!isBoss && aliveCount() >= maxMonstersCap_) return false;
    auto it = factories_.find(id);
    if (it == factories_.end()) return false;
    auto m = it->second(pos);
    if (isBoss) m->setIsBoss(true);
    if (hpScale > 1.f) m->scaleHp(hpScale);
    monsters_.push_back(std::move(m));
    return true;
  }

  // ── Spawn lúc đầu game ───────────────────────────────────
  void spawnInitial(const Camera& cam, int count = 3);

  // ── Update chính (implement trong MonsterManager.cpp) ────
  std::vector<KillInfo> update(float dt, sf::Vector2f playerPos,
                               const Camera& cam);

  // ── Vẽ ───────────────────────────────────────────────────
  void draw(sf::RenderTarget& target) const;
  void drawDebug(sf::RenderTarget& target) const;

  // ── Query ────────────────────────────────────────────────
  std::vector<IMonster*> getLiveMonsters();

  int collectPendingDamage() {
    int total = 0;
    for (auto& m : monsters_) total += m->getAndResetPendingDamage();
    return total;
  }

  int aliveCount() const {
    int n = 0;
    for (auto& m : monsters_)
      if (!m->isDead()) ++n;
    return n;
  }

  int countAliveOfType(const std::string& id) const {
    int n = 0;
    for (auto& m : monsters_)
      if (!m->isDead() && m->getTypeId() == id) ++n;
    return n;
  }

  bool atCap() const { return aliveCount() >= maxMonstersCap_; }

  // Trả về true nếu còn ít nhất 1 final boss sống
  bool hasFinalBoss() const {
    for (auto& m : monsters_)
      if (m && !m->isDead() && m->getTypeId() == "final_boss") return true;
    return false;
  }

 private:
  void applySeparation();
  void spawnOne(const std::string& id, sf::Vector2f playerPos,
                const Camera& cam, bool isBoss);
  sf::Vector2f randomSpawnPos(sf::Vector2f playerPos, const Camera& cam) const;

  static float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
  }

  std::unordered_map<std::string, MonsterFactory> factories_;
  std::vector<WaveEntry> waveEntries_;
  std::unordered_map<std::string, float> spawnTimers_;
  std::vector<std::unique_ptr<IMonster>> monsters_;
  float gameTime_ = 0.f;
  float currentHpScale_ = 1.f;  // set bởi WaveManager mỗi frame
  int maxMonstersCap_ = 300;
};