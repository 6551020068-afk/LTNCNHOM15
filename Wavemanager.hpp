#pragma once
// ════════════════════════════════════════════════════════════
//  WaveManager.hpp  —  Game 10 phút (600 giây)
//
//  Timeline theo thiết kế:
//    0:00  – Giai đoạn 1 Khởi động : chỉ FlyEye, 5 con, spawn 2s
//    1:00  – Giai đoạn 2 Thức tỉnh : thêm Skeleton (chậm, HP 5)
//    2:00  – Boss mini 1 Skeleton King : HP×8, DMG×3
//    4:00  – Boss mini 2 Giant Eye    : HP×10, bắn projectile
//    6:00  – Boss mini 3 Slime Lord   : HP×12, nở Slime con
//    8:00  – Boss mini 4 Death Knight : HP×20, shield×2, speed×1.5
//   10:00  – FINAL BOSS Demon Lord (isFinalBoss=true)
//            Quái thường VẪN tiếp tục spawn. Hạ boss = WIN.
//
//  CÁCH DÙNG:
//    waveMgr_.init(monsters_, difficulty_);
//    waveMgr_.update(dt, playerPos, camera_, monsters_);
//    if (waveMgr_.isFinalBossDefeated()) → showVictoryScreen();
//    if (auto msg = waveMgr_.popMessage()) hudMsg_ = *msg;
// ════════════════════════════════════════════════════════════

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "MonsterManager.hpp"
#include "dokho.hpp"

// ── Spawn pattern cho Map Event ──────────────────────────────
enum class MapEventPattern {
  Horde,     // tập trung ở 1 điểm rồi tỏa ra
  Surround,  // spawn thành vòng tròn xung quanh player
  Ring,      // spawn thành vòng tròn ở khoảng cách nhất định
  Cross,     // spawn 4 con theo 4 hướng chính
  Line,      // spawn thành 1 đường thẳng đi qua player, hướng ngẫu nhiên
  Sweep,     // spawn thành 1 nửa vòng tròn, hướng ngẫu nhiên
};

struct MapEvent {
  float triggerTime;
  std::string monsterId;
  int count;
  MapEventPattern pattern;
  float radius = 550.f;
  std::string message;
};

struct BossSpawn {
  float triggerTime;
  std::string bossId;
  sf::Vector2f offset = {0.f, -400.f};
  std::string message = "BOSS INCOMING!";
  bool isFinalBoss = false;
  // Stat override cho boss mini (0 = dùng default của factory)
  float hpMultiplier = 1.f;  // áp dụng khi spawnDirect set stats
};

struct WaveDef {
  float startTime;
  std::vector<WaveEntry> entries;
  std::string name;
};

class WaveManager {
 public:
  // ────────────────────────────────────────────────────────
  void init(MonsterManager& mm, Difficulty diff = Difficulty::Easy) {
    difficulty_ = diff;
    buildWaves();
    buildBossScript();
    buildMapEvents();
    if (!waves_.empty()) mm.setWave(waves_[0].entries);
  }

  void update(float dt, sf::Vector2f playerPos, const Camera& cam,
              MonsterManager& mm, bool /*finalBossJustKilled*/ = false) {
    gameTime_ += dt;

    // HP scale theo keyframe:
    //   phút 0-5 → ×1.0  |  phút 5 → ×2.0  |  phút 7 → ×2.5  |  phút 9+ → ×3.0
    float hpScale;
    if (gameTime_ < 300.f)
      hpScale = 1.f;
    else if (gameTime_ < 420.f)
      hpScale = 1.5f + (gameTime_ - 300.f) / 120.f * 0.5f;  // 5→7 phút: 2.0→2.5
    else if (gameTime_ < 540.f)
      hpScale = 1.8f + (gameTime_ - 420.f) / 120.f * 0.5f;  // 7→9 phút: 2.5→3.0
    else
      hpScale = 2.f;
    mm.setHpScale(hpScale);  // spawnOne (wave thường) sẽ dùng giá trị này

    // Kiểm tra Final Boss đã bị hạ chưa
    if (finalBossAlive_ && !mm.hasFinalBoss()) {
      finalBossAlive_ = false;
      finalBossDefeated_ = true;
      pendingMessage_ = " VICTORY! You defeated the Final Boss! ";
    }

    // 1. Chuyển wave theo thời gian
    while (nextWave_ < waves_.size() &&
           gameTime_ >= waves_[nextWave_].startTime) {
      mm.setWave(waves_[nextWave_].entries);
      pendingMessage_ = waves_[nextWave_].name;
      ++nextWave_;
    }

    // 2. Boss spawns (mini + final)
    while (nextBoss_ < bossScript_.size() &&
           gameTime_ >= bossScript_[nextBoss_].triggerTime) {
      const auto& bs = bossScript_[nextBoss_];
      sf::Vector2f bossPos = playerPos + bs.offset;
      // Boss mini dùng hpMultiplier từ script; final boss tự set HP
      float bossHpScale = bs.isFinalBoss ? 1.f : bs.hpMultiplier;
      mm.spawnDirect(bs.bossId, bossPos, /*isBoss=*/true, bossHpScale);
      pendingMessage_ = bs.message;

      if (bs.isFinalBoss) finalBossAlive_ = true;
      ++nextBoss_;
    }

    // 3. Map events — truyền hpScale để quái map event cũng scale
    while (nextEvent_ < mapEvents_.size() &&
           gameTime_ >= mapEvents_[nextEvent_].triggerTime) {
      const auto& ev = mapEvents_[nextEvent_];
      auto positions = calcPositions(ev, playerPos, cam);
      for (auto& pos : positions)
        mm.spawnDirect(ev.monsterId, pos, false, hpScale);
      if (!ev.message.empty()) pendingMessage_ = ev.message;
      ++nextEvent_;
    }
  }

  std::optional<std::string> popMessage() {
    if (pendingMessage_.empty()) return std::nullopt;
    auto msg = pendingMessage_;
    pendingMessage_.clear();
    return msg;
  }

  float getGameTime() const { return gameTime_; }
  bool isFinalBossAlive() const { return finalBossAlive_; }
  bool isFinalBossDefeated() const { return finalBossDefeated_; }
  float progressRatio() const { return std::min(gameTime_ / 600.f, 1.f); }

  std::string currentWaveName() const {
    if (nextWave_ == 0) return "Start";
    return waves_[nextWave_ - 1].name;
  }

  std::optional<float> nextBossIn() const {
    if (nextBoss_ >= bossScript_.size()) return std::nullopt;
    float t = bossScript_[nextBoss_].triggerTime - gameTime_;
    return t > 0.f ? std::optional<float>(t) : std::nullopt;
  }

  void forceMapEvent(MapEventPattern pat, const std::string& monsterId,
                     int count, float radius, const std::string& msg = "") {
    mapEvents_.insert(mapEvents_.begin() + nextEvent_,
                      {gameTime_, monsterId, count, pat, radius, msg});
  }

 private:
  // ════════════════════════════════════════════════════════
  //  WAVE SCRIPT
  // ════════════════════════════════════════════════════════
  void buildWaves() {
    waves_.clear();
    // ── 0:00  Giai đoạn 1 — Khởi động ───────────────────────
    // Chỉ FlyEye, 5 con, spawn interval 2s
    pushWave(0.f, "Wave 1 : Bat dau",
             {{"flyeye", 20, 1.0f},  // spawn interval 2s → spawnInterval = 1.0f
              {"zombie", 2, 3.0f}});
    pushWave(30.f, "Wave 2 : Thuc tinh",
             {{"flyeye", 30, 1.0f}, {"zombie", 2, 2.5f}});
    // ── 1:00  Giai đoạn 2 — Thức tỉnh ───────────────────────
    pushWave(60.f, "Wave 3 : Thuc tinh 1",
             {{"flyeye", 30, 1.0f}, {"zombie", 4, 2.5f}});
    // ── 2:00  (sau boss mini 1) ──────────────────────────────
    pushWave(120.f, "Wave 4 : Ap luc den",
             {{"flyeye", 30, 1.0f}, {"zombie", 5, 1.8f}, {"slime", 4, 3.0f}});
    // ── 3:00 ─────────────────────────────────────────────────
    pushWave(180.f, "Wave 5 : Chaos",
             {
                 {"flyeye", 30, 1.2f},
                 {"zombie", 10, 1.4f},
                 {"slime", 7, 2.0f},
             });
    // ── 4:00  (sau boss mini 2) ──────────────────────────────
    pushWave(240.f, "Wave 6 : Nguy hiem",
             {
                 {"flyeye", 30, 1.0f},
                 {"zombie", 14, 1.1f},
                 {"slime", 10, 1.5f},
             });
    // ── 5:00 ─────────────────────────────────────────────────
    pushWave(300.f, "Wave 7 : Khong ngung nghi",
             {
                 {"flyeye", 30, 0.8f},
                 {"zombie", 18, 0.9f},
                 {"slime", 14, 1.2f},
             });
    // ── 6:00  (sau boss mini 3) ──────────────────────────────
    pushWave(360.f, "Wave 8 : Tinh nhue",
             {{"flyeye", 30, 0.7f},
              {"zombie", 20, 0.8f},
              {"slime", 16, 1.0f},
              {"ghost", 20, 2.0f}});
    // ── 7:00 ─────────────────────────────────────────────────
    pushWave(420.f, "Wave 9 : Hon loan",
             {{"flyeye", 50, 0.6f},
              {"zombie", 40, 0.7f},
              {"slime", 18, 0.9f},
              {"ghost", 30, 1.8f}});
    // ── 8:00  (sau boss mini 4) ──────────────────────────────
    pushWave(480.f, "Wave 10 : Can tu",
             {{"flyeye", 100, 0.0f},
              {"zombie", 100, 0.0f},
              {"slime", 80, 0.0f},
              {"ghost", 30, 1.5f}});
    // ── 9:00  Đỉnh điểm trước Final Boss ────────────────────
    pushWave(540.f, "Wave 11 : Truoc Ngay Tan",
             {{"flyeye", 150, 0.05f},
              {"zombie", 150, 0.05f},
              {"slime", 120, 0.08f},
              {"ghost", 40, 0.08f}});
    pushWave(540.f, "Wave 11 : Truoc Ngay Tan",
             {{"flyeye", 150, 0.05f},
              {"zombie", 150, 0.05f},
              {"slime", 120, 0.08f},
              {"ghost", 50, 0.08f}});
    pushWave(540.f, "Wave 11 : Truoc Ngay Tan",
             {{"flyeye", 150, 0.05f},
              {"zombie", 150, 0.05f},
              {"slime", 120, 0.08f},
              {"ghost", 60, 0.08f}});
    // 10:00 : Final Boss spawn → wave thường vẫn giữ nguyên
    std::sort(waves_.begin(), waves_.end(),
              [](const WaveDef& a, const WaveDef& b) {
                return a.startTime < b.startTime;
              });
  }

  // ════════════════════════════════════════════════════════
  //  BOSS SCRIPT
  //
  //  Boss mini HP multiplier (dựa theo thiết kế):
  //    Zombie King    : HP×8  (base HP 5 → 40)  , DMG×3
  //    Giant Eye      : HP×10 (base HP 2 → 20)  , projectile
  //    Slime Lord     : HP×12 (base HP 5 → 60)  , split at 50%
  //    Death Knight   : HP×20 (base HP 5 → 100) , shield×2, speed×1.5
  //
  //  Hard mode: mỗi boss mini còn ×bossMiniHpMult thêm 1 lần nữa
  //  → Zombie King Hard: 40 × 1.5 = 60 HP
  //
  //  Lưu ý: hpMultiplier ở đây là multiplier SO VỚI base class HP.
  //  MonsterManager::spawnDirect cần đọc field này để set HP ngay
  //  sau khi factory tạo ra monster (xem MonsterManager.cpp).
  // ════════════════════════════════════════════════════════
  void buildBossScript() {
    bossScript_.clear();

    const float diffMult = DifficultyConfig::get(difficulty_).bossMiniHpMult;

    // 2:00 – Boss mini 1: Zombie King
    bossScript_.push_back({
        120.f,
        "zombie",
        {0.f, -500.f},
        "BOSS: ZOMBIE KING xuat hien!",
        false,
        16.f * diffMult  // Tăng từ 8.f lên 16.f
    });

    // 4:00 – Boss mini 2: Giant Eye
    bossScript_.push_back({
        240.f,
        "flyeye",
        {0.f, -500.f},
        "BOSS: GIANT EYE xuat hien!",
        false,
        20.f * diffMult  // Tăng từ 10.f lên 20.f
    });

    // 6:00 – Boss mini 3: Slime Lord
    bossScript_.push_back({
        360.f,
        "slime",
        {0.f, -500.f},
        "BOSS: SLIME LORD xuat hien!",
        false,
        100.f * diffMult  // Tăng từ 50.f lên 100.f
                          // để dễ nhớ, spawnDirect sẽ nhân tiếp ×1.2)
    });

    // 8:00 – Boss mini 4: Death Knight
    bossScript_.push_back({
        480.f,
        "zombie",
        {0.f, -500.f},
        "BOSS: DEATH KNIGHT xuat hien!",
        false,
        100.f * diffMult  // Tăng từ 50.f lên 100.f
                          // 50 để dễ nhớ, spawnDirect sẽ nhân tiếp ×2.0)
    });

    // 10:00 – FINAL BOSS: Demon Lord
    // HP được set trực tiếp trong DemonLord constructor từ DifficultyConfig
    bossScript_.push_back({
        600.f,
        "final_boss",
        {0.f, -600.f},
        "FINAL BOSS: DEMON LORD DA XUAT HIEN! CHIEN DAU DEN HEP THO CUOI!",
        true,
        1.f  // không dùng, DemonLord tự lấy finalBossHp từ cfg
    });

    std::sort(bossScript_.begin(), bossScript_.end(),
              [](const BossSpawn& a, const BossSpawn& b) {
                return a.triggerTime < b.triggerTime;
              });
  }

  // ════════════════════════════════════════════════════════
  //  MAP EVENTS
  // ════════════════════════════════════════════════════════
  void buildMapEvents() {
    mapEvents_.clear();

    // Phút 0

    pushEvent(10.f, "flyeye", 6, MapEventPattern::Horde, 800.f, "");

    pushEvent(30.f, "flyeye", 8, MapEventPattern::Surround, 700.f,
              "Bi bao vay!");
    pushEvent(50.f, "zombie", 5, MapEventPattern::Line, 800.f, "");

    // Phút 1
    pushEvent(70.f, "flyeye", 20, MapEventPattern::Ring, 600.f,
              "Ring of Eyes!");
    pushEvent(90.f, "zombie", 8, MapEventPattern::Horde, 700.f, "");
    pushEvent(110.f, "flyeye", 10, MapEventPattern::Cross, 650.f, "");

    // Phút 2 (quanh boss mini 1)
    pushEvent(120.f, "flyeye", 20, MapEventPattern::Horde, 800.f, "");
    pushEvent(140.f, "zombie", 12, MapEventPattern::Surround, 700.f,
              "Encircled!");
    pushEvent(130.f, "zombie", 10, MapEventPattern::Line, 800.f, "");
    pushEvent(160.f, "flyeye", 14, MapEventPattern::Sweep, 800.f, "SWARM!");

    // Phút 3
    pushEvent(190.f, "flyeye", 16, MapEventPattern::Ring, 700.f, "");
    pushEvent(215.f, "zombie", 14, MapEventPattern::Horde, 800.f,
              "Zombie Rush!");
    pushEvent(235.f, "slime", 10, MapEventPattern::Cross, 600.f, "");
    pushEvent(250.f, "ghost", 12, MapEventPattern::Surround, 800.f,
              "GHOST AMBUSH!");

    // Phút 4 (quanh boss mini 2)
    pushEvent(260.f, "flyeye", 18, MapEventPattern::Sweep, 900.f,
              "MEGA SWARM!");
    pushEvent(285.f, "zombie", 16, MapEventPattern::Surround, 800.f, "");

    // Phút 5
    pushEvent(310.f, "flyeye", 40, MapEventPattern::Ring, 800.f, "Death Ring!");
    pushEvent(335.f, "slime", 12, MapEventPattern::Horde, 700.f, "Slime Tide!");
    pushEvent(355.f, "zombie", 18, MapEventPattern::Cross, 750.f, "");

    // Phút 6 (quanh boss mini 3)
    pushEvent(375.f, "flyeye", 50, MapEventPattern::Sweep, 900.f,
              "TOTAL SWARM!");
    pushEvent(400.f, "zombie", 20, MapEventPattern::Ring, 800.f,
              "Zombie Circle!");

    // Phút 7
    pushEvent(430.f, "flyeye", 25, MapEventPattern::Surround, 900.f,
              "TOTAL SIEGE!");
    pushEvent(455.f, "slime", 16, MapEventPattern::Horde, 800.f, "");
    pushEvent(475.f, "zombie", 22, MapEventPattern::Sweep, 900.f, "");

    // Phút 8 (quanh boss mini 4)
    pushEvent(500.f, "flyeye", 28, MapEventPattern::Ring, 900.f, "EYE STORM!");
    pushEvent(520.f, "zombie", 25, MapEventPattern::Horde, 850.f,
              "ARMY OF DEATH!");

    // Phút 9 – chuẩn bị trước Final Boss
    pushEvent(545.f, "flyeye", 30, MapEventPattern::Sweep, 950.f,
              "FINAL WAVE! Chuan bi!");
    pushEvent(560.f, "zombie", 28, MapEventPattern::Surround, 900.f, "");
    pushEvent(575.f, "slime", 20, MapEventPattern::Ring, 850.f,
              "Gio tan den roi...");

    std::sort(mapEvents_.begin(), mapEvents_.end(),
              [](const MapEvent& a, const MapEvent& b) {
                return a.triggerTime < b.triggerTime;
              });
  }

  // ── Helpers ──────────────────────────────────────────────
  void pushWave(float t, const std::string& name,
                std::vector<WaveEntry> entries) {
    waves_.push_back({t, std::move(entries), name});
  }

  void pushEvent(float t, const std::string& id, int count, MapEventPattern pat,
                 float radius, const std::string& msg) {
    mapEvents_.push_back({t, id, count, pat, radius, msg});
  }

  // ── Tính vị trí spawn theo pattern ───────────────────────
  std::vector<sf::Vector2f> calcPositions(const MapEvent& ev,
                                          sf::Vector2f center,
                                          const Camera& /*cam*/) const {
    std::vector<sf::Vector2f> pts;
    pts.reserve(ev.count);
    const float R = ev.radius;
    const int N = ev.count;
    const float PI = 3.14159265f;

    switch (ev.pattern) {
      case MapEventPattern::Horde: {
        int side = std::rand() % 4;
        for (int i = 0; i < N; i++) {
          float spread = R * 0.8f;
          float t = (float(i) / N - 0.5f) * spread;
          sf::Vector2f p;
          switch (side) {
            case 0:
              p = {center.x + t, center.y - R};
              break;
            case 1:
              p = {center.x + t, center.y + R};
              break;
            case 2:
              p = {center.x - R, center.y + t};
              break;
            default:
              p = {center.x + R, center.y + t};
              break;
          }
          pts.push_back(jitter(p, 20.f));
        }
        break;
      }
      case MapEventPattern::Surround: {
        for (int i = 0; i < N; i++) {
          float angle = (float(i) / N) * 2.f * PI + randF() * (PI / N);
          float r = R * (0.85f + randF() * 0.3f);
          pts.push_back(
              {center.x + std::cos(angle) * r, center.y + std::sin(angle) * r});
        }
        break;
      }
      case MapEventPattern::Ring: {
        for (int i = 0; i < N; i++) {
          float angle = (float(i) / N) * 2.f * PI;
          pts.push_back(
              {center.x + std::cos(angle) * R, center.y + std::sin(angle) * R});
        }
        break;
      }
      case MapEventPattern::Cross: {
        int perArm = std::max(1, N / 4);
        float spacing = R / perArm;
        sf::Vector2f dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (int arm = 0; arm < 4; arm++)
          for (int k = 0; k < perArm; k++) {
            float dist = spacing * (k + 1) * 0.8f + R * 0.3f;
            pts.push_back({center.x + dirs[arm].x * dist + jitterV().x,
                           center.y + dirs[arm].y * dist + jitterV().y});
          }
        break;
      }
      case MapEventPattern::Line: {
        bool horiz = (std::rand() % 2 == 0);
        float fromSide = (std::rand() % 2 == 0) ? -R : R;
        float spacing = R * 1.6f / N;
        float start = -R * 0.8f;
        for (int i = 0; i < N; i++) {
          float off = start + spacing * i;
          sf::Vector2f p =
              horiz ? sf::Vector2f{center.x + fromSide, center.y + off}
                    : sf::Vector2f{center.x + off, center.y + fromSide};
          pts.push_back(jitter(p, 15.f));
        }
        break;
      }
      case MapEventPattern::Sweep: {
        int side = std::rand() % 2;
        float fromSide = -R;
        float spacing = (R * 2.f) / N;
        for (int i = 0; i < N; i++) {
          float off = -R + spacing * i;
          sf::Vector2f p =
              (side == 0) ? sf::Vector2f{center.x + fromSide, center.y + off}
                          : sf::Vector2f{center.x + off, center.y + fromSide};
          pts.push_back(jitter(p, 10.f));
        }
        break;
      }
    }
    return pts;
  }

  static float randF() { return static_cast<float>(std::rand()) / RAND_MAX; }
  static sf::Vector2f jitterV(float r = 25.f) {
    return {(randF() - 0.5f) * r * 2.f, (randF() - 0.5f) * r * 2.f};
  }
  static sf::Vector2f jitter(sf::Vector2f p, float r) { return p + jitterV(r); }

  // ── State ─────────────────────────────────────────────────
  Difficulty difficulty_ = Difficulty::Easy;
  std::vector<WaveDef> waves_;
  std::vector<BossSpawn> bossScript_;
  std::vector<MapEvent> mapEvents_;

  size_t nextWave_ = 0;
  size_t nextBoss_ = 0;
  size_t nextEvent_ = 0;

  float gameTime_ = 0.f;
  bool finalBossAlive_ = false;
  bool finalBossDefeated_ = false;
  std::string pendingMessage_;
};