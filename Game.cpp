#include "Game.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#include "CharacterClass.hpp"
#include "Demonlora.hpp"
#include "FlyEye.hpp"
#include "Ghost.hpp"
#include "IMonster.hpp"
#include "MapData.hpp"
#include "Slime.hpp"
#include "Zombie.hpp"

using namespace Constants;

// ════════════════════════════════════════════════════════════
//  Đạn boss – quản lý riêng trong Game, không dùng BulletManager
// ════════════════════════════════════════════════════════════
struct BossBulletLive {
  sf::Vector2f pos;
  sf::Vector2f dir;
  float speed = 240.f;
  float damage = 5.f;
  float life = 8.f;  // giây tồn tại tối đa
};

static std::vector<BossBulletLive>
    bossBullets_;  // global trong translation unit

// ════════════════════════════════════════════════════════════
//  Constructor
// ════════════════════════════════════════════════════════════
Game::Game()
    : window_(sf::VideoMode({WIN_W, WIN_H}), " Warrior Survivors",
              sf::Style::Default),
      colMap_(),
      tileMap_(),
      player_(colMap_),
      camera_(window_) {
  std::srand(static_cast<unsigned>(std::time(nullptr)));
  window_.setFramerateLimit(60);

  for (int i = 0; i < MapData::TILESET_COUNT; ++i)
    tileMap_.addTileset(MapData::TILESETS[i].filename,
                        MapData::TILESETS[i].firstGid);
  const uint32_t* layerPtrs[] = {MapData::LAYER_FLOOR, MapData::LAYER_CO};
  for (int i = 0; i < MapData::LAYER_COUNT; ++i)
    tileMap_.addLayer(MapData::LAYER_NAMES[i], layerPtrs[i],
                      MapData::LAYER_SIZE);
  tileMap_.loadTilesets();

  if (!player_.load()) std::cerr << "[Game] Player textures missing!\n";

  FlyEye::loadTextures();
  Zombie::loadTextures();
  Ghost::loadTextures();
  DemonLord::loadTextures();
  ExpOrb::loadTexture();

  monsters_.registerFactory(
      "flyeye", [](sf::Vector2f p) { return std::make_unique<FlyEye>(p); });
  monsters_.registerFactory(
      "slime", [](sf::Vector2f p) { return std::make_unique<Slime>(p); });
  monsters_.registerFactory(
      "ghost", [](sf::Vector2f p) { return std::make_unique<Ghost>(p); });
  monsters_.registerFactory(
      "zombie", [](sf::Vector2f p) { return std::make_unique<Zombie>(p); });
  monsters_.registerFactory("final_boss", [](sf::Vector2f p) {
    return std::make_unique<DemonLord>(p, 1000);
  });

  for (auto* path :
       {"C:/Windows/Fonts/seguisym.ttf", "arial.ttf",
        "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/consola.ttf"}) {
    if (font_.openFromFile(path)) {
      fontLoaded_ = true;
      break;
    }
  }

  try {
    saveData_ = SaveSystem::load();
  } catch (const std::runtime_error& e) {
    std::cerr << "[Game] " << e.what() << "\n[Game] Reset ve save mac dinh.\n";
    saveData_ = SaveData{};
  }

  menu_.init();
  SoundManager::get().init();
  HolyBibleSkill::loadTexture("hinh anh\\Sprite-King_Bible.png");
  HolyBibleSkill::loadEvolvedTexture("hinh anh\\Sprite-Unholy_Vespers.png");

  // Tải các ảnh cho bảng nâng cấp kỹ năng
  upgradeIcons_[UpgradeType::Knife].loadFromFile(
      "hinh anh\\icon_knife.png");  // Sửa lại thành icon-Knife.png nếu ảnh của
                                    // bạn tên như vậy
  upgradeIcons_[UpgradeType::LightningRing].loadFromFile(
      "hinh anh\\icon_lightning.png");
  upgradeIcons_[UpgradeType::Garlic].loadFromFile("hinh anh\\icon_Garlic.png");
  upgradeIcons_[UpgradeType::HolyBible].loadFromFile(
      "hinh anh\\icon_bible.png");

  std::cout << "=== Warrior Survivors ===\n"
            << "WASD/Arrow: di chuyen  |  F1: debug  |  ESC: pause\n"
            << "--- DEMO KEYS ---\n"
            << "  F2: Spawn Mini Boss (FlyEye)\n"
            << "  F3: Spawn Mini Boss (Ghost)\n"
            << "  F4: Spawn Final Boss (DemonLord)\n"
            << "  F5/F6: Ep Demon Lord sang Phase 2/3\n"
            << "  L : Win game ngay lap tuc\n"
            << "  N : Len cap / mo bang chon ky nang\n\n";
}

// ════════════════════════════════════════════════════════════
//  applyCharacterClass
// ════════════════════════════════════════════════════════════
void Game::applyCharacterClass(int idx) {
  if (idx < 0 || idx >= CharacterClass::CLASS_COUNT) idx = 0;
  const CharClassDef& def = CharacterClass::DEFS[idx];
  const DifficultyConfig& cfg = DifficultyConfig::get(difficulty_);

  // Load animation theo nhân vật đã chọn (Rogue, Mage,...)
  if (!player_.loadForClass(idx)) {
    std::cerr << "[Game] Khong the load texture cho class: " << def.name
              << "\n";
  }

  stats_.maxHp = cfg.playerStartHp + def.bonusHp;
  stats_.hp = stats_.maxHp;
  stats_.damage = 1 + def.bonusDamage;
  playerSpeed_ = Player::SPEED + def.bonusSpeed;
  player_.setSpeed(playerSpeed_);  // ← đồng bộ tốc độ vào Player

  switch (def.startSkill) {
    case StartingSkill::Knife:
      skillMgr_.applyUpgrade(SkillUpgradeType::Knife);
      break;
    case StartingSkill::Lightning:
      skillMgr_.applyUpgrade(SkillUpgradeType::LightningRing);
      break;
    case StartingSkill::Garlic:
      skillMgr_.applyUpgrade(SkillUpgradeType::Garlic);
      break;
    case StartingSkill::HolyBible:
      skillMgr_.applyUpgrade(SkillUpgradeType::HolyBible);
      break;
  }

  std::cout << "[Game] Class=" << def.name << " Difficulty=" << cfg.name
            << " HP=" << stats_.maxHp << " DMG=" << stats_.damage << "\n";
}

// ════════════════════════════════════════════════════════════
//  applyDifficulty
// ════════════════════════════════════════════════════════════
void Game::applyDifficulty() {
  score_.difficulty = difficulty_;

  const DifficultyConfig& cfg = DifficultyConfig::get(difficulty_);
  monsters_.setMaxMonsters(cfg.maxMonsters);

  // Đăng ký lại factory final_boss với đúng HP theo difficulty
  const int fbHp = cfg.finalBossHp;
  monsters_.registerFactory("final_boss", [fbHp](sf::Vector2f p) {
    return std::make_unique<DemonLord>(p, fbHp);
  });
}

// ════════════════════════════════════════════════════════════
//  endGame
// ════════════════════════════════════════════════════════════
void Game::endGame() {
  gameState_ = GameState::GameOver;
  SoundManager::get().playMusic(SoundManager::BGM::GAMEOVER_BGM, false);

  saveData_.lastScore = score_.score;
  saveData_.lastCharIndex = selectedChar_;
  saveData_.lastDifficulty =
      (difficulty_ == Difficulty::Hard) ? "Hard" : "Easy";
  saveData_.lastKills = score_.kills;
  saveData_.lastTimeAlive = score_.timeAlive;

  SaveSystem::updateHighScore(saveData_, score_.score, difficulty_);
  try {
    SaveSystem::save(saveData_);
  } catch (const std::runtime_error& e) {
    std::cerr << "[Game] Loi luu file: " << e.what() << "\n";
  }

  std::cout << "[Game] Game Over. Score=" << score_.score
            << " Kills=" << score_.kills << " Time=" << score_.formatTime()
            << "\n";
}

// ════════════════════════════════════════════════════════════
//  restartGame
// ════════════════════════════════════════════════════════════
void Game::restartGame() {
  upgradeOptions_ = {};
  score_.reset();
  skillMgr_ = SkillManager{};
  expManager_ = ExpManager{};
  bullets_ = BulletManager{};
  bossBullets_.clear();
  monsters_ = MonsterManager{};
  stats_ = PlayerStats{};
  waveMgr_ = WaveManager{};
  paused_ = false;
  pauseMenuOpen_ = false;
  settingsOpen_ = false;
  hoveredCard_ = -1;
  levelUpTimer_ = 0.f;
  hudMessage_.clear();
  hudMessageTimer_ = 0.f;
  victoryShown_ = false;
  victoryAnimTime_ = 0.f;

  monsters_.registerFactory(
      "flyeye", [](sf::Vector2f p) { return std::make_unique<FlyEye>(p); });
  monsters_.registerFactory(
      "slime", [](sf::Vector2f p) { return std::make_unique<Slime>(p); });
  monsters_.registerFactory(
      "zombie", [](sf::Vector2f p) { return std::make_unique<Zombie>(p); });
  monsters_.registerFactory(
      "ghost", [](sf::Vector2f p) { return std::make_unique<Ghost>(p); });
  player_.setPosition(
      {MAP_WIDTH * TILE_RENDER_W / 2.f, MAP_HEIGHT * TILE_RENDER_H / 2.f});
  camera_.reset();

  applyDifficulty();                // đăng ký final_boss factory với HP đúng
  player_.setSpeed(Player::SPEED);  // reset speed trước
  applyCharacterClass(selectedChar_);

  waveMgr_.init(monsters_, difficulty_);
  monsters_.spawnInitial(camera_, 3);

  SoundManager::get().playMusic(SoundManager::BGM::GAME_BGM);
  gameState_ = GameState::Playing;
}

// ════════════════════════════════════════════════════════════
//  run
// ════════════════════════════════════════════════════════════
void Game::run() {
  while (window_.isOpen()) {
    float dt = clock_.restart().asSeconds();
    if (dt > MAX_DT) dt = MAX_DT;

    SoundManager::get().tick();

    while (const auto event = window_.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window_.close();
        return;
      }
      if (gameState_ == GameState::Menu)
        menu_.handleEvent(*event);
      else
        processEvents(*event);
    }

    if (gameState_ == GameState::Menu) {
      menu_.update(dt);
      window_.clear();
      menu_.render();
      window_.display();

      if (menu_.isDone()) {
        auto res = menu_.getResult();
        if (res.quit) return;
        selectedChar_ = res.charIndex;
        difficulty_ = res.difficulty;
        restartGame();
        window_.setView(camera_.getView());
        gameState_ = GameState::Playing;
      }
      continue;
    }

    if (gameState_ == GameState::Playing || gameState_ == GameState::Victory)
      update(dt);
    render();
  }
}

// ════════════════════════════════════════════════════════════
//  orbValue  —  nhân đôi exp orb từ phút thứ 3 (180 giây)
// ════════════════════════════════════════════════════════════
int Game::orbValue(int base) const {
  return (score_.timeAlive >= 180.f) ? base * 2 : base;
}

// ════════════════════════════════════════════════════════════
//  update
// ════════════════════════════════════════════════════════════
void Game::update(float dt) {
  if (paused_) return;
  if (gameState_ == GameState::Victory) {
    victoryAnimTime_ += dt;
    return;
  }

  score_.update(dt);
  player_.update(dt);

  sf::Vector2f pPos = player_.getPosition();
  sf::Vector2f facing = player_.getFacingVector();
  sf::Vector2f moveDir = player_.getMoveVector();

  auto liveMonsters = monsters_.getLiveMonsters();
  std::vector<std::pair<sf::Vector2f, void*>> monsterData;
  monsterData.reserve(liveMonsters.size());
  for (auto* m : liveMonsters)
    monsterData.push_back({m->getPosition(), static_cast<void*>(m)});

  // ── Skills ───────────────────────────────────────────────
  auto skillResult =
      skillMgr_.update(pPos, facing, moveDir, dt, stats_.damage, monsterData);

  bullets_.spawnFromShots(skillResult.shots);
  if (!skillResult.shots.empty())
    SoundManager::get().play(SoundManager::SFX::SHOOT);

  for (auto& zap : skillResult.zaps) {
    auto* m = static_cast<IMonster*>(zap.monster);
    if (!m || !m->isAlive()) continue;
    m->takeHit(zap.damage);
    SoundManager::get().playVaried(SoundManager::SFX::HIT_MONSTER);
    if (!m->isAlive()) {
      expManager_.spawnOrb(zap.pos, orbValue(m->getExpValue()));
      score_.addKill(m->getTypeId());
    }
  }

  for (auto& hit : skillResult.garlic) {
    auto* m = static_cast<IMonster*>(hit.monster);
    if (!m || !m->isAlive()) continue;
    m->applyKnockback(hit.knockbackDir, hit.knockbackForce);
    m->takeHit(hit.damage);
    SoundManager::get().playVaried(SoundManager::SFX::HIT_MONSTER);
    if (!m->isAlive()) {
      expManager_.spawnOrb(hit.monsterPos, orbValue(m->getExpValue()));
      score_.addKill(m->getTypeId());
      if (skillMgr_.garlicCanHeal()) stats_.heal(0.2f);
    }
  }

  for (auto& hit : skillResult.bible) {
    auto* m = static_cast<IMonster*>(hit.monster);
    if (!m || !m->isAlive()) continue;
    m->applyKnockback(hit.knockbackDir, hit.knockback);
    m->takeHit(hit.damage);
    SoundManager::get().playVaried(SoundManager::SFX::HIT_MONSTER);
    if (!m->isAlive()) {
      expManager_.spawnOrb(hit.monsterPos, orbValue(m->getExpValue()));
      score_.addKill(m->getTypeId());
    }
  }

  auto bulletKills = bullets_.update(dt, liveMonsters, stats_.damage);
  for (auto& k : bulletKills) {
    expManager_.spawnOrb(k.pos, orbValue(k.expValue));
    score_.addKill(k.typeId);
  }

  // ── DemonLord: poll outputs trước waveMgr.update ─────────
  bool finalBossJustKilled = false;
  finalBossPtr_ = nullptr;

  for (auto* m : monsters_.getLiveMonsters()) {
    auto* demon = dynamic_cast<DemonLord*>(m);
    if (!demon) continue;
    finalBossPtr_ = demon;

    // Projectiles — Boss bắn đạn: push thẳng vào bossBullets_
    for (auto& proj : demon->getPendingProjectiles()) {
      bossBullets_.push_back(
          {proj.origin, proj.direction, proj.speed, proj.damage, 4.f});
    }
    demon->clearProjectiles();

    // Summon
    for (auto& s : demon->getPendingSummons())
      monsters_.spawnDirect(s.typeId, s.pos, false);
    demon->clearSummons();

    // AoE pulse (Phase 3)
    if (demon->hasPendingAoe()) {
      for (auto& pulse : demon->getPendingAoe()) {
        sf::Vector2f diff = pPos - pulse.origin;
        float dist2 = diff.x * diff.x + diff.y * diff.y;
        if (dist2 < pulse.radius * pulse.radius && dist2 > 0.f) {
          stats_.takeDamage(pulse.damage);
          SoundManager::get().play(SoundManager::SFX::PLAYER_HIT);
          // Knockback player: player_.addExternalVelocity nếu có,
          // nếu không có thì bỏ qua — Player tự xử lý collision
        }
      }
      demon->clearAoe();
    }

    // Melee damage boss gây cho player
    int meleeDmg = demon->getAndResetPendingMelee();
    if (meleeDmg > 0) {
      stats_.takeDamage(meleeDmg);
      SoundManager::get().play(SoundManager::SFX::PLAYER_HIT);
    }

    // Phase change HUD
    DemonPhase curPhase = demon->getPhase();
    if (curPhase != lastDemonPhase_) {
      lastDemonPhase_ = curPhase;
      if (curPhase == DemonPhase::Phase2) {
        hudMessage_ = "DEMON LORD - PHASE 2: Ban 4 dan chu thap!";
        hudMessageTimer_ = 3.f;
      } else if (curPhase == DemonPhase::Phase3) {
        hudMessage_ = "DEMON LORD - ENRAGE! Toc do 120px/s!";
        hudMessageTimer_ = 3.f;
      }
    }

    // Đánh dấu nếu vừa bị hạ
    if (demon->isKilledFlag()) finalBossJustKilled = true;
  }

  // ── WaveManager — truyền finalBossJustKilled ─────────────
  waveMgr_.update(dt, pPos, camera_, monsters_, finalBossJustKilled);
  if (auto msg = waveMgr_.popMessage()) {
    hudMessage_ = *msg;
    hudMessageTimer_ = 3.0f;
  }

  // ── Chiến thắng ──────────────────────────────────────────
  if (waveMgr_.isFinalBossDefeated() && !victoryShown_) {
    victoryShown_ = true;
    victoryAnimTime_ = 0.f;

    // Lưu điểm chiến thắng
    saveData_.lastScore = score_.score;
    saveData_.lastCharIndex = selectedChar_;
    saveData_.lastDifficulty =
        (difficulty_ == Difficulty::Hard) ? "Hard" : "Easy";
    saveData_.lastKills = score_.kills;
    saveData_.lastTimeAlive = score_.timeAlive;
    SaveSystem::updateHighScore(saveData_, score_.score, difficulty_);
    try {
      SaveSystem::save(saveData_);
    } catch (const std::runtime_error& e) {
      std::cerr << "[Game] Loi luu file: " << e.what() << "\n";
    }

    SoundManager::get().stopMusic();
    gameState_ = GameState::Victory;
    std::cout << "[Game] VICTORY! Score=" << score_.score
              << " Time=" << score_.formatTime() << "\n";
  }

  // ── MonsterManager update ────────────────────────────────
  auto monsterKills = monsters_.update(dt, pPos, camera_);
  for (auto& k : monsterKills) {
    expManager_.spawnOrb(k.pos, orbValue(k.expValue));
    score_.addKill(k.typeId);
    if (k.isBoss) {
      stats_.heal(2.0f);
      hudMessage_ = "Boss ha! HP +2 khoi phuc!";
      hudMessageTimer_ = 3.0f;
    }
  }

  // ── Damage quái → player (scaled theo difficulty) ────────
  int monsterDmg = monsters_.collectPendingDamage();
  if (monsterDmg > 0) {
    const DifficultyConfig& cfg = DifficultyConfig::get(difficulty_);
    int scaledDmg = static_cast<int>(monsterDmg * cfg.monsterDamageMult + 0.5f);
    stats_.takeDamage(scaledDmg);
    SoundManager::get().play(SoundManager::SFX::PLAYER_HIT);
  }

  // ── Hồi máu từ nâng cấp Regen ────────────────────────────
  if (stats_.regenLevel > 0 && stats_.hp < stats_.maxHp) {
    stats_.regenTimer += dt;
    if (stats_.regenTimer >= 1.0f) {
      stats_.regenTimer -= 1.0f;  // Trừ đi 1 giây
      stats_.heal(0.2f * stats_.regenLevel);
    }
  }

  if (stats_.isDead()) {
    endGame();
    return;
  }

  // ── EXP & Level up ───────────────────────────────────────
  int gained = expManager_.update(dt, pPos);
  if (gained > 0) {
    SoundManager::get().play(SoundManager::SFX::PICKUP_EXP);
    bool leveledUp = expManager_.addExp(gained);
    if (leveledUp) {
      score_.addLevelUp(expManager_.getLevel());
      levelUpTimer_ = 2.5f;
      paused_ = true;
      buildUpgradeOptions();
      SoundManager::get().play(SoundManager::SFX::LEVEL_UP);
    }
  }
  if (levelUpTimer_ > 0.f) levelUpTimer_ -= dt;
  if (hudMessageTimer_ > 0.f) hudMessageTimer_ -= dt;

  // ── Boss bullets: update vị trí + va chạm player ────────
  {
    sf::Vector2f pp = player_.getPosition();
    const float HIT_R = 28.f;  // bán kính va chạm với player
    for (auto& b : bossBullets_) {
      b.pos += b.dir * b.speed * dt;
      b.life -= dt;
      // Va chạm player
      sf::Vector2f d = pp - b.pos;
      float dist2 = d.x * d.x + d.y * d.y;
      if (dist2 < HIT_R * HIT_R) {
        stats_.takeDamage(static_cast<int>(b.damage));
        SoundManager::get().play(SoundManager::SFX::PLAYER_HIT);
        b.life = -1.f;  // đánh dấu xóa
      }
    }
    // Xóa đạn hết hạn hoặc trúng player
    bossBullets_.erase(
        std::remove_if(bossBullets_.begin(), bossBullets_.end(),
                       [](const BossBulletLive& b) { return b.life <= 0.f; }),
        bossBullets_.end());
  }

  camera_.update(dt, pPos);
}

// ════════════════════════════════════════════════════════════
//  processEvents
// ════════════════════════════════════════════════════════════
void Game::processEvents(const sf::Event& event) {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);
  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  sf::Vector2f mouseUI =
      window_.mapPixelToCoords(sf::Mouse::getPosition(window_), uiView);
  updateHover(
      sf::Vector2i(static_cast<int>(mouseUI.x), static_cast<int>(mouseUI.y)));

  if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
    switch (key->code) {
      case sf::Keyboard::Key::Escape:
        if (gameState_ == GameState::Playing) {
          if (paused_ && pauseMenuOpen_) {
            pauseMenuOpen_ = false;
            paused_ = false;
          } else if (!paused_) {
            pauseMenuOpen_ = true;
            paused_ = true;
          }
        }
        break;
      case sf::Keyboard::Key::F1:
        debugMode_ = !debugMode_;
        break;

      // ── DEMO KEYS (dùng khi demo cho giảng viên) ──────────
      // F2: Triệu hồi mini boss FlyEye ngay tại vị trí player
      case sf::Keyboard::Key::F2:
        if (gameState_ == GameState::Playing) {
          sf::Vector2f pp = player_.getPosition();
          sf::Vector2f spawnOff = {120.f, 0.f};
          monsters_.spawnDirect("flyeye", pp + spawnOff, true);  // isBoss=true
          hudMessage_ = "[F2] MINI BOSS xuat hien!";
          hudMessageTimer_ = 2.5f;
        }
        break;

      // F3: Triệu hồi mini boss Ghost ngay tại vị trí player
      case sf::Keyboard::Key::F3:
        if (gameState_ == GameState::Playing) {
          sf::Vector2f pp = player_.getPosition();
          monsters_.spawnDirect("ghost", pp + sf::Vector2f{-120.f, 0.f}, true);
          hudMessage_ = "[F3] MINI BOSS Ghost xuat hien!";
          hudMessageTimer_ = 2.5f;
        }
        break;

      // F4: Triệu hồi final boss DemonLord
      case sf::Keyboard::Key::F4:
        if (gameState_ == GameState::Playing) {
          sf::Vector2f pp = player_.getPosition();
          monsters_.spawnDirect("final_boss", pp + sf::Vector2f{200.f, 0.f},
                                false);
          hudMessage_ = "[F4] DEMON LORD XUAT HIEN! CHUC MAY MAN!";
          hudMessageTimer_ = 3.0f;
        }
        break;

      // F5: Ép DemonLord sang Phase 2
      case sf::Keyboard::Key::F5:
        if (gameState_ == GameState::Playing && finalBossPtr_) {
          finalBossPtr_->debugForcePhase(2);
          hudMessage_ = "[F5] FORCE PHASE 2!";
          hudMessageTimer_ = 2.0f;
        }
        break;

      // F6: Ép DemonLord sang Phase 3
      case sf::Keyboard::Key::F6:
        if (gameState_ == GameState::Playing && finalBossPtr_) {
          finalBossPtr_->debugForcePhase(3);
          hudMessage_ = "[F6] FORCE PHASE 3!";
          hudMessageTimer_ = 2.0f;
        }
        break;

      // L: Win game ngay lập tức (demo thắng game)
      case sf::Keyboard::Key::L:
        if (gameState_ == GameState::Playing && !victoryShown_) {
          victoryShown_ = true;
          victoryAnimTime_ = 0.f;
          saveData_.lastScore = score_.score;
          saveData_.lastCharIndex = selectedChar_;
          saveData_.lastDifficulty =
              (difficulty_ == Difficulty::Hard) ? "Hard" : "Easy";
          saveData_.lastKills = score_.kills;
          saveData_.lastTimeAlive = score_.timeAlive;
          SaveSystem::updateHighScore(saveData_, score_.score, difficulty_);
          try {
            SaveSystem::save(saveData_);
          } catch (const std::runtime_error& e) {
            std::cerr << "[Game] Loi luu file: " << e.what() << "\n";
          }
          SoundManager::get().stopMusic();
          gameState_ = GameState::Victory;
          paused_ = false;
          pauseMenuOpen_ = false;
          std::cout << "[DEMO] L pressed — VICTORY forced!\n";
        }
        break;

      // N: Lên cấp ngay lập tức (mở bảng chọn upgrade)
      case sf::Keyboard::Key::N:
        if (gameState_ == GameState::Playing && !paused_) {
          score_.addLevelUp(expManager_.getLevel() + 1);
          levelUpTimer_ = 2.5f;
          paused_ = true;
          buildUpgradeOptions();
          SoundManager::get().play(SoundManager::SFX::LEVEL_UP);
          hudMessage_ = "[N] LEN CAP! Chon ky nang.";
          hudMessageTimer_ = 2.0f;
        }
        break;
        // ── END DEMO KEYS ──────────────────────────────────────

      case sf::Keyboard::Key::Num1:
        if (paused_ && upgradeOptions_.size() > 0)
          applyUpgrade(upgradeOptions_[0].type);
        break;
      case sf::Keyboard::Key::Num2:
        if (paused_ && upgradeOptions_.size() > 1)
          applyUpgrade(upgradeOptions_[1].type);
        break;
      case sf::Keyboard::Key::Num3:
        if (paused_ && upgradeOptions_.size() > 2)
          applyUpgrade(upgradeOptions_[2].type);
        break;
      case sf::Keyboard::Key::R:
        if (gameState_ == GameState::GameOver ||
            gameState_ == GameState::Victory)
          restartGame();
        break;
      case sf::Keyboard::Key::M:
        if (gameState_ == GameState::GameOver ||
            gameState_ == GameState::Victory) {
          gameState_ = GameState::Menu;
          window_.setView(window_.getDefaultView());
          SoundManager::get().stopAll();
          menu_.init();
        }
        break;
      default:
        break;
    }
  }

  if (paused_ && gameState_ == GameState::Playing) {
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (click->button == sf::Mouse::Button::Left) {
        if (pauseMenuOpen_)
          handlePauseMenuClick(mouseUI);
        else if (hoveredCard_ >= 0)
          applyUpgrade(upgradeOptions_[hoveredCard_].type);
      }
    }
  }

  camera_.handleEvent(event);
}

// ════════════════════════════════════════════════════════════
//  render
// ════════════════════════════════════════════════════════════
void Game::render() {
  window_.setView(camera_.getView());
  window_.clear(sf::Color(20, 20, 30));

  tileMap_.renderLayers(window_, camera_.getView(), 0, 1);
  tileMap_.renderLayers(window_, camera_.getView(), 1, 2);

  expManager_.draw(window_);
  bullets_.draw(window_);

  // ── Vẽ đạn boss (chấm đỏ cam) ───────────────────────────
  for (const auto& b : bossBullets_) {
    sf::CircleShape dot(8.f);
    dot.setFillColor(sf::Color(255, 60, 30, 230));
    dot.setOutlineColor(sf::Color(255, 200, 50, 180));
    dot.setOutlineThickness(2.f);
    dot.setOrigin({8.f, 8.f});
    dot.setPosition(b.pos);
    window_.draw(dot);
  }
  monsters_.draw(window_);
  skillMgr_.drawEffects(window_, player_.getPosition());
  player_.draw(window_);

  if (debugMode_) {
    player_.drawDebug(window_);
    monsters_.drawDebug(window_);
    skillMgr_.drawDebugRanges(window_, player_.getPosition());
  }

  renderHUD();
  if (paused_ && gameState_ == GameState::Playing) {
    if (pauseMenuOpen_)
      renderPauseMenu();
    else
      renderUpgradeScreen();
  }
  if (gameState_ == GameState::GameOver) renderGameOver();
  if (gameState_ == GameState::Victory) renderVictory();

  window_.display();
}

// ════════════════════════════════════════════════════════════
//  renderHUD
// ════════════════════════════════════════════════════════════
static void drawBar(sf::RenderWindow& win, sf::Vector2f pos, sf::Vector2f size,
                    float ratio, sf::Color bg, sf::Color fill, sf::Color glow) {
  sf::RectangleShape glowBox(size + sf::Vector2f(4.f, 4.f));
  glowBox.setFillColor(sf::Color(0, 0, 0, 0));
  glowBox.setOutlineThickness(2.f);
  glowBox.setOutlineColor(sf::Color(glow.r, glow.g, glow.b, 80));
  glowBox.setPosition(pos - sf::Vector2f(2.f, 2.f));
  win.draw(glowBox);

  sf::RectangleShape bgBox(size);
  bgBox.setFillColor(bg);
  bgBox.setPosition(pos);
  win.draw(bgBox);

  if (ratio > 0.f) {
    sf::RectangleShape fillBox({size.x * std::min(ratio, 1.f), size.y});
    fillBox.setFillColor(fill);
    fillBox.setPosition(pos);
    win.draw(fillBox);
  }
}

void Game::renderHUD() {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);

  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  window_.setView(uiView);

  const float mg = 14.f, bW = 340.f, bH = 24.f, gap = 10.f;
  const float hpY = mg, expY = hpY + bH + gap;

  float hpR = static_cast<float>(stats_.hp) / static_cast<float>(stats_.maxHp);
  sf::Color hpCol = (hpR > 0.5f)    ? sf::Color(50, 220, 80)
                    : (hpR > 0.25f) ? sf::Color(240, 190, 30)
                                    : sf::Color(220, 40, 40);
  drawBar(window_, {mg, hpY}, {bW, bH}, hpR, sf::Color(20, 5, 5, 210), hpCol,
          hpCol);

  float expR = static_cast<float>(expManager_.getExp()) /
               static_cast<float>(expManager_.getExpReq());
  drawBar(window_, {mg, expY}, {bW, bH}, expR, sf::Color(8, 5, 20, 210),
          sf::Color(140, 70, 255), sf::Color(170, 110, 255));

  if (!fontLoaded_) {
    window_.setView(camera_.getView());
    return;
  }

  // HP text
  {
    std::ostringstream ss;
    ss << "HP " << stats_.hp << "/" << stats_.maxHp;
    sf::Text t(font_, ss.str(), 13);
    t.setFillColor(sf::Color(240, 240, 240));
    auto b = t.getLocalBounds();
    t.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    t.setPosition({mg + bW / 2.f, hpY + bH / 2.f});
    window_.draw(t);
  }

  // EXP text
  {
    std::ostringstream ss;
    ss << "Lv" << expManager_.getLevel() << "  " << expManager_.getExp() << "/"
       << expManager_.getExpReq();
    sf::Text t(font_, ss.str(), 11);
    t.setFillColor(sf::Color(210, 190, 255));
    auto b = t.getLocalBounds();
    t.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    t.setPosition({mg + bW / 2.f, expY + bH / 2.f});
    window_.draw(t);
  }

  // Score panel
  {
    const float iconSize = 36.f;
    const float iconGap = 6.f;
    const float panelPad = 6.f;
    float startX = mg;
    float startY = expY + bH + 8.f;

    // Thu thập danh sách skill đã unlock cùng level
    struct SkillSlot {
      UpgradeType type;
      int level;
      bool evolved;
    };
    std::vector<SkillSlot> slots;
    if (skillMgr_.hasKnife())
      slots.push_back({UpgradeType::Knife, skillMgr_.getKnifeLevel(),
                       skillMgr_.knifeEvolved()});
    if (skillMgr_.hasLightning())
      slots.push_back({UpgradeType::LightningRing,
                       skillMgr_.getLightningLevel(),
                       skillMgr_.lightningEvolved()});
    if (skillMgr_.hasGarlic())
      slots.push_back({UpgradeType::Garlic, skillMgr_.getGarlicLevel(),
                       skillMgr_.garlicEvolved()});
    if (skillMgr_.hasBible())
      slots.push_back(
          {UpgradeType::HolyBible, skillMgr_.getBibleLevel(), false});

    for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
      float ix = startX + i * (iconSize + iconGap);
      float iy = startY;

      // Nền ô — vàng nếu evolved, tối bình thường
      sf::Color bgCol = slots[i].evolved ? sf::Color(80, 60, 10, 210)
                                         : sf::Color(15, 10, 30, 210);
      sf::Color rimCol = slots[i].evolved ? sf::Color(255, 200, 40, 230)
                                          : sf::Color(120, 100, 200, 180);

      sf::RectangleShape box({iconSize, iconSize});
      box.setFillColor(bgCol);
      box.setOutlineColor(rimCol);
      box.setOutlineThickness(2.f);
      box.setPosition({ix, iy});
      window_.draw(box);

      // Icon texture
      auto it = upgradeIcons_.find(slots[i].type);
      if (it != upgradeIcons_.end()) {
        sf::Sprite icon(it->second);
        auto ts = it->second.getSize();
        float scale =
            (iconSize - 4.f) / static_cast<float>(std::max(ts.x, ts.y));
        icon.setScale({scale, scale});
        // Căn giữa trong ô
        float sw = ts.x * scale, sh = ts.y * scale;
        icon.setPosition(
            {ix + (iconSize - sw) / 2.f, iy + (iconSize - sh) / 2.f});
        window_.draw(icon);
      }

      // Level badge (góc dưới phải)
      if (fontLoaded_) {
        sf::Text lvTxt(font_, std::to_string(slots[i].level), 9);
        lvTxt.setFillColor(sf::Color(255, 230, 60));
        auto lb = lvTxt.getLocalBounds();
        lvTxt.setOrigin({lb.size.x, lb.size.y + lb.position.y});
        lvTxt.setPosition({ix + iconSize - 3.f, iy + iconSize - 3.f});
        window_.draw(lvTxt);
      }
    }

    // Kills + Wave info — ngay dưới hàng icon
    float textY = startY + iconSize + 6.f;
    {
      std::ostringstream kills;
      kills << "Kills: " << score_.kills
            << "   Alive: " << monsters_.aliveCount();
      sf::Text t(font_, kills.str(), 11);
      t.setFillColor(sf::Color(200, 200, 200));
      t.setPosition({mg, textY});
      window_.draw(t);
    }
    {
      sf::Text wt(font_, "Wave: " + waveMgr_.currentWaveName(), 11);
      wt.setFillColor(sf::Color(150, 200, 255, 200));
      wt.setPosition({mg, textY + 16.f});
      window_.draw(wt);
    }
  }

  // HUD Message
  if (hudMessageTimer_ > 0.f) {
    float alpha = std::min(hudMessageTimer_ / 0.5f, 1.f) * 255.f;
    auto a = static_cast<uint8_t>(alpha),
         a2 = static_cast<uint8_t>(alpha * 0.6f);
    sf::Text shadow(font_, hudMessage_, 22);
    shadow.setFillColor(sf::Color(0, 0, 0, a2));
    auto b = shadow.getLocalBounds();
    shadow.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    shadow.setPosition({winW / 2.f + 2.f, 82.f});
    window_.draw(shadow);
    sf::Text msg(font_, hudMessage_, 22);
    msg.setFillColor(sf::Color(255, 220, 60, a));
    b = msg.getLocalBounds();
    msg.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    msg.setPosition({winW / 2.f, 80.f});
    window_.draw(msg);
  }

  window_.setView(camera_.getView());
}

// ════════════════════════════════════════════════════════════
//  renderGameOver
// ════════════════════════════════════════════════════════════
void Game::renderGameOver() {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);

  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  window_.setView(uiView);

  sf::RectangleShape overlay({winW, winH});
  overlay.setFillColor(sf::Color(10, 5, 15, 220));
  window_.draw(overlay);
  if (!fontLoaded_) return;

  float cx = winW / 2.f, cy = winH / 2.f;

  auto drawText = [&](const std::string& s, unsigned sz, float x, float y,
                      sf::Color c, bool center = true) {
    sf::Text t(font_, s, sz);
    auto b = t.getLocalBounds();
    if (center) t.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    t.setFillColor(sf::Color(0, 0, 0, 150));
    t.setPosition({x + 2.f, y + 2.f});
    window_.draw(t);
    t.setFillColor(c);
    t.setPosition({x, y});
    window_.draw(t);
  };

  drawText("G A M E   O V E R", 60, cx, cy - 200.f, sf::Color(255, 70, 70));

  float panW = 440.f, panH = 260.f;
  sf::RectangleShape panel({panW, panH});
  panel.setFillColor(sf::Color(30, 25, 45, 240));
  panel.setOutlineColor(sf::Color(120, 100, 200, 200));
  panel.setOutlineThickness(3.f);
  panel.setOrigin({panW / 2.f, panH / 2.f});
  panel.setPosition({cx, cy - 20.f});
  window_.draw(panel);

  const DifficultyConfig& cfg = DifficultyConfig::get(difficulty_);
  int hiScore = (difficulty_ == Difficulty::Hard) ? saveData_.highScoreHard
                                                  : saveData_.highScoreEasy;
  float startX = cx - 180.f, startY = cy - 110.f;
  drawText(cfg.name + " MODE", 14, cx, startY, sf::Color(150, 150, 150));

  auto drawStat = [&](const std::string& key, const std::string& val, float y,
                      sf::Color valCol) {
    drawText(key, 15, startX, y, sf::Color(180, 180, 180), false);
    sf::Text tmp(font_, val, 16);
    float valX = cx + 180.f - tmp.getLocalBounds().size.x;
    drawText(val, 16, valX, y, valCol, false);
  };

  drawStat("CHARACTER", CharacterClass::DEFS[selectedChar_].name, startY + 40.f,
           sf::Color(150, 200, 255));
  drawStat("SCORE", std::to_string(score_.score), startY + 75.f,
           sf::Color(255, 230, 60));
  drawStat("BEST", std::to_string(hiScore), startY + 105.f,
           sf::Color(100, 200, 255));
  drawStat("KILLS", std::to_string(score_.kills), startY + 135.f,
           sf::Color(255, 130, 100));
  drawStat("SURVIVED", score_.formatTime(), startY + 165.f,
           sf::Color(150, 255, 150));

  if (score_.score >= hiScore && score_.score > 0) {
    static sf::Clock flashClock;
    float flash =
        std::abs(std::sin(flashClock.getElapsedTime().asSeconds() * 5.f));
    drawText(" NEW RECORD ", 20, cx, cy + 90.f,
             sf::Color(255, 255, 0, static_cast<uint8_t>(150 + 105 * flash)));
  }
  drawText("[R] RESTART      [M] MAIN MENU", 16, cx, cy + 180.f,
           sf::Color(130, 130, 130));
  window_.setView(camera_.getView());
}

// ════════════════════════════════════════════════════════════
//  Upgrade system
// ════════════════════════════════════════════════════════════
void Game::buildUpgradeOptions() {
  std::vector<UpgradeOption> pool;

  pool.push_back({UpgradeType::Damage, "TANG DAME", "Tang sat thuong +0.2",
                  sf::Color(220, 60, 60)});

  // Regen (Hồi máu)
  if (stats_.regenLevel < 5) {  // Cấu hình tối đa 5 level
    pool.push_back(
        {UpgradeType::Regen,
         "HOI MAU Lv" + std::to_string(stats_.regenLevel + 1),
         "Hoi " + std::to_string((stats_.regenLevel + 1) * 0.2f).substr(0, 3) +
             " HP moi giay",
         sf::Color(50, 220, 80)});
  }

  // Knife
  if (!skillMgr_.hasKnife())
    pool.push_back({UpgradeType::Knife, "KNIFE", "Dao bay theo huong",
                    sf::Color(200, 200, 60)});
  else if (skillMgr_.knifeMaxed() && !skillMgr_.knifeEvolved())
    pool.push_back({UpgradeType::Knife, "KNIFE EVOLVE", "Thousand Edge",
                    sf::Color(255, 220, 0)});
  else if (!skillMgr_.knifeEvolved())
    pool.push_back({UpgradeType::Knife,
                    "KNIFE Lv" + std::to_string(skillMgr_.getKnifeLevel() + 1),
                    "Tang so dao & toc do", sf::Color(200, 200, 60)});

  // Lightning
  if (!skillMgr_.hasLightning())
    pool.push_back({UpgradeType::LightningRing, "LIGHTNING",
                    "Set danh quai gan", sf::Color(100, 180, 255)});
  else if (skillMgr_.lightningMaxed() && !skillMgr_.lightningEvolved())
    pool.push_back({UpgradeType::LightningRing, "LIGHTNING EVOLVE",
                    "Thunder Loop", sf::Color(180, 230, 255)});
  else if (!skillMgr_.lightningEvolved())
    pool.push_back(
        {UpgradeType::LightningRing,
         "LIGHTNING Lv" + std::to_string(skillMgr_.getLightningLevel() + 1),
         "Tang dame & vung set", sf::Color(100, 180, 255)});

  // Garlic
  if (!skillMgr_.hasGarlic())
    pool.push_back({UpgradeType::Garlic, "GARLIC", "Vung AoE day lui quai",
                    sf::Color(180, 255, 100)});
  else if (skillMgr_.garlicMaxed() && !skillMgr_.garlicEvolved())
    pool.push_back({UpgradeType::Garlic, "GARLIC EVOLVE", "Soul Eater: hut mau",
                    sf::Color(100, 255, 80)});
  else if (!skillMgr_.garlicEvolved())
    pool.push_back(
        {UpgradeType::Garlic,
         "GARLIC Lv" + std::to_string(skillMgr_.getGarlicLevel() + 1),
         "Tang range & knockback", sf::Color(160, 230, 80)});

  // ── Bible (MỚI) ──────────────────────────────────────────────────────────
  if (!skillMgr_.hasBible())
    pool.push_back({UpgradeType::HolyBible, "SACH THANH",
                    "Mo khoa: sach bay orbit, gay damage khi cham quai",
                    sf::Color(200, 170, 255)});
  else if (skillMgr_.bibleMaxed() && !skillMgr_.bibleEvolved())
    pool.push_back({UpgradeType::HolyBible, "THANH KINH QUY [EVO]",
                    "Tien hoa: damage x2, them sach, xoay nhanh hon!",
                    sf::Color(255, 180, 255)});
  else if (!skillMgr_.bibleEvolved())
    pool.push_back({UpgradeType::HolyBible, skillMgr_.getBibleUpgradeTitle(),
                    skillMgr_.getBibleUpgradeDesc(), sf::Color(200, 170, 255)});

  auto rng = std::default_random_engine{std::random_device{}()};
  std::shuffle(pool.begin(), pool.end(), rng);
  for (int i = 0; i < 3; ++i) upgradeOptions_[i] = pool[i % pool.size()];
}

void Game::applyUpgrade(UpgradeType t) {
  switch (t) {
    case UpgradeType::Damage:
      // Thay vì gọi stats_.upgradeDamage() (mặc định +1)
      // Ta cộng trực tiếp lượng damage mong muốn
      stats_.damage += 0.2f;
      break;
    case UpgradeType::AttackSpeed:
      stats_.upgradeAttackSpeed();
      break;
    case UpgradeType::Regen:
      stats_.regenLevel++;
      break;
    case UpgradeType::Knife:
      skillMgr_.applyUpgrade(SkillUpgradeType::Knife);
      break;
    case UpgradeType::LightningRing:
      skillMgr_.applyUpgrade(SkillUpgradeType::LightningRing);
      break;
    case UpgradeType::Garlic:
      skillMgr_.applyUpgrade(SkillUpgradeType::Garlic);
      break;
    case UpgradeType::HolyBible:
      skillMgr_.applyUpgrade(SkillUpgradeType::HolyBible);
      break;  // ← đổi
  }
  paused_ = false;
  hoveredCard_ = -1;
}

// ════════════════════════════════════════════════════════════
//  renderUpgradeScreen helpers
// ════════════════════════════════════════════════════════════
void Game::updateHover(sf::Vector2i mouse) {
  hoveredCard_ = -1;
  if (!paused_) return;
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);
  const float cardW = 220.f, cardH = 280.f, gap = 30.f;
  float totalW = 3 * cardW + 2 * gap;
  float startX = (winW - totalW) / 2.f;
  float cardY = winH / 2.f - cardH / 2.f + 20.f;
  for (int i = 0; i < 3; ++i) {
    float cx = startX + i * (cardW + gap);
    if (mouse.x >= cx && mouse.x <= cx + cardW && mouse.y >= cardY &&
        mouse.y <= cardY + cardH) {
      hoveredCard_ = i;
      break;
    }
  }
}

sf::Vector2f Game::mouseToWorld() const {
  return window_.mapPixelToCoords(sf::Mouse::getPosition(window_),
                                  camera_.getView());
}

void Game::renderUpgradeScreen() {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);

  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  window_.setView(uiView);

  sf::RectangleShape overlay({winW, winH});
  overlay.setFillColor(sf::Color(0, 0, 0, 170));
  window_.draw(overlay);

  if (fontLoaded_) {
    sf::Text title(font_, "LEVEL  UP!", 38);
    title.setFillColor(sf::Color(255, 230, 60));
    auto b = title.getLocalBounds();
    title.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    title.setPosition({winW / 2.f, winH / 2.f - 180.f});
    window_.draw(title);
    sf::Text sub(font_, "Chon nang cap  (1 / 2 / 3)", 14);
    sub.setFillColor(sf::Color(180, 180, 180));
    b = sub.getLocalBounds();
    sub.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    sub.setPosition({winW / 2.f, winH / 2.f - 135.f});
    window_.draw(sub);
  }

  const float cardW = 220.f, cardH = 280.f, gap = 30.f;
  float totalW = 3 * cardW + 2 * gap;
  float startX = (winW - totalW) / 2.f;
  float cardY = winH / 2.f - cardH / 2.f + 20.f;
  const char* keys[] = {"1", "2", "3"};

  for (int i = 0; i < 3; ++i) {
    float cx = startX + i * (cardW + gap);
    bool hov = (hoveredCard_ == i);
    const auto& opt = upgradeOptions_[i];

    sf::RectangleShape card({cardW, cardH});
    card.setFillColor(hov ? sf::Color(40, 40, 60, 240)
                          : sf::Color(18, 18, 30, 230));
    card.setOutlineThickness(hov ? 2.5f : 1.5f);
    card.setOutlineColor(
        hov ? sf::Color(opt.color.r, opt.color.g, opt.color.b, 255)
            : sf::Color(opt.color.r, opt.color.g, opt.color.b, 130));
    card.setPosition({cx, cardY});
    window_.draw(card);

    sf::RectangleShape bar({cardW, 5.f});
    bar.setFillColor(opt.color);
    bar.setPosition({cx, cardY});
    window_.draw(bar);

    if (!fontLoaded_) continue;

    sf::Text keyT(font_, keys[i], 20);
    keyT.setFillColor(opt.color);
    auto b = keyT.getLocalBounds();
    keyT.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    keyT.setPosition({cx + cardW / 2.f, cardY + 40.f});
    window_.draw(keyT);

    // Vẽ ảnh Icon kỹ năng
    auto iconIt = upgradeIcons_.find(opt.type);
    if (iconIt != upgradeIcons_.end()) {
      sf::Sprite iconSpr(iconIt->second);
      auto b2 = iconSpr.getLocalBounds();
      iconSpr.setOrigin({b2.size.x / 2.f, b2.size.y / 2.f});
      iconSpr.setPosition({cx + cardW / 2.f, cardY + 95.f});
      // Giữ tỉ lệ tự động lọt vừa khoảng không (64x64)
      float scale = 64.f / std::max(b2.size.x, b2.size.y);
      iconSpr.setScale({scale, scale});
      window_.draw(iconSpr);
    }

    sf::Text titleT(font_, opt.title, 16);
    titleT.setFillColor(sf::Color::White);
    b = titleT.getLocalBounds();
    titleT.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    titleT.setPosition({cx + cardW / 2.f, cardY + 145.f});
    window_.draw(titleT);

    sf::Text descT(font_, opt.desc, 12);
    descT.setFillColor(sf::Color(180, 180, 180));
    b = descT.getLocalBounds();
    descT.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    descT.setPosition({cx + cardW / 2.f, cardY + 185.f});
    window_.draw(descT);

    if (hov) {
      sf::Text hint(font_, "[ CLICK ]", 11);
      hint.setFillColor(sf::Color(opt.color.r, opt.color.g, opt.color.b, 200));
      b = hint.getLocalBounds();
      hint.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
      hint.setPosition({cx + cardW / 2.f, cardY + cardH - 20.f});
      window_.draw(hint);
    }
  }
  window_.setView(camera_.getView());
}

// ════════════════════════════════════════════════════════════
//  renderVictory
// ════════════════════════════════════════════════════════════
void Game::renderVictory() {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);

  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  window_.setView(uiView);

  // ── Nền tối gradient vàng/tím ────────────────────────────
  sf::RectangleShape overlay({winW, winH});
  overlay.setFillColor(sf::Color(8, 5, 18, 210));
  window_.draw(overlay);

  if (!fontLoaded_) {
    window_.setView(camera_.getView());
    return;
  }

  float t = victoryAnimTime_;
  float cx = winW / 2.f;
  float cy = winH / 2.f;

  // ── Hiệu ứng tia sáng xoay ───────────────────────────────
  {
    const int RAYS = 12;
    for (int i = 0; i < RAYS; ++i) {
      float angle = t * 25.f + i * (360.f / RAYS);
      float rad = angle * 3.14159f / 180.f;
      float len = 420.f + std::sin(t * 2.f + i) * 60.f;
      sf::RectangleShape ray({len, 2.f});
      uint8_t alpha = static_cast<uint8_t>(
          std::min(1.f, t / 1.2f) * (55.f + 25.f * std::sin(t * 3.f + i)));
      ray.setFillColor(sf::Color(255, 210, 60, alpha));
      ray.setOrigin({0.f, 1.f});
      ray.setPosition({cx, cy - 60.f});
      ray.setRotation(sf::degrees(angle));
      window_.draw(ray);
    }
  }

  // ── Tiêu đề VICTORY ──────────────────────────────────────
  {
    float bob = std::sin(t * 2.2f) * 6.f;
    float appear = std::min(1.f, t / 0.8f);
    uint8_t a = static_cast<uint8_t>(appear * 255);

    // Shadow
    sf::Text shadow(font_, "CHIEN THANG!", 68);
    shadow.setFillColor(sf::Color(180, 100, 0, static_cast<uint8_t>(a * 0.5f)));
    auto b = shadow.getLocalBounds();
    shadow.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    shadow.setPosition({cx + 4.f, cy - 220.f + bob + 4.f});
    window_.draw(shadow);

    sf::Text title(font_, "CHIEN THANG!", 68);
    title.setFillColor(sf::Color(255, 215, 0, a));
    b = title.getLocalBounds();
    title.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    title.setPosition({cx, cy - 220.f + bob});
    window_.draw(title);

    sf::Text sub(font_, "Demon Lord da bi tieu diet!", 20);
    sub.setFillColor(sf::Color(220, 180, 255, a));
    b = sub.getLocalBounds();
    sub.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    sub.setPosition({cx, cy - 155.f + bob});
    window_.draw(sub);
  }

  // ── Panel thống kê ────────────────────────────────────────
  {
    float appear = std::min(1.f, std::max(0.f, (t - 0.5f) / 0.8f));
    uint8_t panA = static_cast<uint8_t>(appear * 245);

    const float panW = 460.f, panH = 270.f;
    sf::RectangleShape panel({panW, panH});
    panel.setFillColor(sf::Color(20, 14, 38, panA));
    panel.setOutlineColor(sf::Color(255, 200, 50, panA));
    panel.setOutlineThickness(2.5f);
    panel.setOrigin({panW / 2.f, panH / 2.f});
    panel.setPosition({cx, cy + 20.f});
    window_.draw(panel);

    // Dải vàng trên cùng panel
    sf::RectangleShape topBar({panW, 5.f});
    topBar.setFillColor(sf::Color(255, 200, 50, panA));
    topBar.setOrigin({panW / 2.f, 0.f});
    topBar.setPosition({cx, cy - panH / 2.f + 20.f});
    window_.draw(topBar);

    auto drawStat = [&](const std::string& key, const std::string& val, float y,
                        sf::Color valCol) {
      float alpha_f = appear;
      sf::Color kc(180, 180, 180, static_cast<uint8_t>(alpha_f * 200));
      sf::Color vc(valCol.r, valCol.g, valCol.b,
                   static_cast<uint8_t>(alpha_f * 255));

      // Key (trái)
      sf::Text kt(font_, key, 15);
      kt.setFillColor(kc);
      kt.setPosition({cx - panW / 2.f + 24.f, y});
      window_.draw(kt);

      // Value (phải, căn phải)
      sf::Text vt(font_, val, 16);
      vt.setFillColor(vc);
      auto b = vt.getLocalBounds();
      vt.setPosition({cx + panW / 2.f - 24.f - b.size.x, y});
      window_.draw(vt);
    };

    const DifficultyConfig& cfg = DifficultyConfig::get(difficulty_);
    int hiScore = (difficulty_ == Difficulty::Hard) ? saveData_.highScoreHard
                                                    : saveData_.highScoreEasy;

    float sy = cy - panH / 2.f + 32.f;
    sf::Text modeT(font_, cfg.name + " MODE", 13);
    modeT.setFillColor(sf::Color(150, 150, 180, panA));
    auto mb = modeT.getLocalBounds();
    modeT.setOrigin({mb.size.x / 2.f, mb.size.y / 2.f});
    modeT.setPosition({cx, sy});
    window_.draw(modeT);

    drawStat("NHAN VAT", CharacterClass::DEFS[selectedChar_].name, sy + 36.f,
             sf::Color(150, 200, 255));
    drawStat("DIEM SO", std::to_string(score_.score), sy + 72.f,
             sf::Color(255, 230, 60));
    drawStat("KY LUC", std::to_string(hiScore), sy + 104.f,
             sf::Color(100, 200, 255));
    drawStat("TIEU DIET", std::to_string(score_.kills), sy + 136.f,
             sf::Color(255, 130, 100));
    drawStat("THOI GIAN SONG SOT", score_.formatTime(), sy + 168.f,
             sf::Color(150, 255, 150));

    // "NEW RECORD" flash
    if (score_.score >= hiScore && score_.score > 0 && appear >= 1.f) {
      float flash = std::abs(std::sin(t * 5.f));
      sf::Text rec(font_, "  ** KY LUC MOI **  ", 20);
      rec.setFillColor(
          sf::Color(255, 255, 0, static_cast<uint8_t>(150 + 105 * flash)));
      auto rb = rec.getLocalBounds();
      rec.setOrigin({rb.size.x / 2.f, rb.size.y / 2.f});
      rec.setPosition({cx, cy + panH / 2.f + 20.f - 80.f + 24.f});
      window_.draw(rec);
    }
  }

  // ── Particle bắn lên (pháo hoa giả) ─────────────────────
  if (t > 0.3f) {
    const int PARTS = 30;
    for (int i = 0; i < PARTS; ++i) {
      float phase = static_cast<float>(i) / PARTS;
      float lt = std::fmod(t * 0.7f + phase, 1.f);
      float px = cx + std::sin(phase * 6.28f + t) * (winW * 0.42f);
      float py = winH * (1.f - lt) - 20.f;
      float alpha = std::sin(lt * 3.14159f) * 200.f;
      float r2 = 2.f + std::sin(phase * 11.f + t) * 1.5f;
      int ci = i % 4;
      sf::Color pc =
          (ci == 0)   ? sf::Color(255, 220, 50, static_cast<uint8_t>(alpha))
          : (ci == 1) ? sf::Color(100, 200, 255, static_cast<uint8_t>(alpha))
          : (ci == 2) ? sf::Color(200, 100, 255, static_cast<uint8_t>(alpha))
                      : sf::Color(100, 255, 160, static_cast<uint8_t>(alpha));
      sf::CircleShape p(r2);
      p.setFillColor(pc);
      p.setPosition({px, py});
      window_.draw(p);
    }
  }

  // ── Hướng dẫn ─────────────────────────────────────────────
  if (t > 1.2f) {
    float flash = std::abs(std::sin(t * 2.5f));
    uint8_t ha = static_cast<uint8_t>(120 + 100 * flash);
    sf::Text hint(font_, "[R] CHOI LAI      [M] MENU CHINH", 16);
    hint.setFillColor(sf::Color(180, 170, 200, ha));
    auto b = hint.getLocalBounds();
    hint.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    hint.setPosition({cx, cy + winH * 0.38f});
    window_.draw(hint);
  }

  window_.setView(camera_.getView());
}

// ════════════════════════════════════════════════════════════
//  renderPauseMenu
// ════════════════════════════════════════════════════════════
void Game::renderPauseMenu() {
  auto winSize = window_.getSize();
  float winW = static_cast<float>(winSize.x);
  float winH = static_cast<float>(winSize.y);

  sf::View uiView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
  window_.setView(uiView);

  sf::RectangleShape overlay({winW, winH});
  overlay.setFillColor(sf::Color(0, 0, 0, 180));
  window_.draw(overlay);
  if (!fontLoaded_) {
    window_.setView(camera_.getView());
    return;
  }

  float cx = winW / 2.f, cy = winH / 2.f;
  const float panW = 320.f, panH = 420.f;
  sf::RectangleShape panel({panW, panH});
  panel.setFillColor(sf::Color(18, 14, 32, 245));
  panel.setOutlineColor(sf::Color(120, 80, 200, 220));
  panel.setOutlineThickness(2.f);
  panel.setOrigin({panW / 2.f, panH / 2.f});
  panel.setPosition({cx, cy});
  window_.draw(panel);

  sf::Text title(font_, "PAUSED", 36);
  title.setFillColor(sf::Color(255, 230, 60));
  auto b = title.getLocalBounds();
  title.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
  title.setPosition({cx, cy - 165.f});
  window_.draw(title);

  auto drawButton = [&](const std::string& label, float y, sf::Color outline,
                        sf::Color textCol) {
    const float bW = 240.f, bH = 48.f;
    sf::Vector2f mp =
        window_.mapPixelToCoords(sf::Mouse::getPosition(window_), uiView);
    bool hov = (mp.x >= cx - bW / 2.f && mp.x <= cx + bW / 2.f &&
                mp.y >= y - bH / 2.f && mp.y <= y + bH / 2.f);
    sf::RectangleShape btn({bW, bH});
    btn.setFillColor(hov ? sf::Color(50, 40, 80, 220)
                         : sf::Color(28, 22, 48, 200));
    btn.setOutlineColor(hov ? outline
                            : sf::Color(outline.r, outline.g, outline.b, 120));
    btn.setOutlineThickness(hov ? 2.f : 1.f);
    btn.setOrigin({bW / 2.f, bH / 2.f});
    btn.setPosition({cx, y});
    window_.draw(btn);
    sf::Text t(font_, label, 16);
    t.setFillColor(textCol);
    auto tb = t.getLocalBounds();
    t.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
    t.setPosition({cx, y});
    window_.draw(t);
  };

  drawButton("TIEP TUC", cy - 80.f, sf::Color(80, 220, 120),
             sf::Color(100, 255, 140));
  drawButton("CAI DAT AM THANH", cy, sf::Color(100, 160, 255),
             sf::Color(140, 190, 255));
  drawButton("VE MENU CHINH", cy + 80.f, sf::Color(220, 160, 60),
             sf::Color(255, 190, 80));
  drawButton("THOAT GAME", cy + 160.f, sf::Color(220, 60, 60),
             sf::Color(255, 90, 90));

  if (settingsOpen_) {
    const float sW = 280.f, sH = 160.f;
    float sX = cx + panW / 2.f + 20.f, sY = cy - sH / 2.f;
    sf::RectangleShape sp({sW, sH});
    sp.setFillColor(sf::Color(20, 16, 36, 250));
    sp.setOutlineColor(sf::Color(100, 160, 255, 200));
    sp.setOutlineThickness(2.f);
    sp.setPosition({sX, sY});
    window_.draw(sp);

    sf::Text stitle(font_, "AM THANH", 18);
    stitle.setFillColor(sf::Color(140, 190, 255));
    stitle.setPosition({sX + 14.f, sY + 12.f});
    window_.draw(stitle);

    auto& sm = SoundManager::get();
    bool sfxOn = sm.isSfxEnabled(), musicOn = sm.isMusicEnabled();
    const float pillW = 52.f, pillH = 24.f;
    float pillX = sX + sW - pillW - 14.f;

    auto drawToggle = [&](const std::string& label, bool on, float ty) {
      sf::Text lbl(font_, label, 13);
      lbl.setFillColor(sf::Color(200, 200, 200));
      lbl.setPosition({sX + 14.f, ty + 4.f});
      window_.draw(lbl);
      sf::RectangleShape pill({pillW, pillH});
      pill.setFillColor(on ? sf::Color(60, 200, 90) : sf::Color(80, 40, 40));
      pill.setOutlineColor(sf::Color(255, 255, 255, 60));
      pill.setOutlineThickness(1.f);
      pill.setPosition({pillX, ty});
      window_.draw(pill);
      sf::CircleShape knob(10.f);
      knob.setFillColor(sf::Color(240, 240, 240));
      knob.setPosition({on ? (pillX + pillW - 22.f) : (pillX + 2.f), ty + 2.f});
      window_.draw(knob);
      sf::Text onoff(font_, on ? "ON" : "OFF", 10);
      onoff.setFillColor(on ? sf::Color(200, 255, 200)
                            : sf::Color(200, 120, 120));
      auto ob = onoff.getLocalBounds();
      onoff.setOrigin({ob.size.x / 2.f, ob.size.y / 2.f});
      onoff.setPosition({pillX + pillW / 2.f, ty + 12.f});
      window_.draw(onoff);
    };

    drawToggle("HIEU UNG AM THANH (SFX)", sfxOn, sY + 56.f);
    drawToggle("NHAC NEN (MUSIC)", musicOn, sY + 100.f);
  }

  window_.setView(camera_.getView());
}

// ════════════════════════════════════════════════════════════
//  handlePauseMenuClick
// ════════════════════════════════════════════════════════════
void Game::handlePauseMenuClick(sf::Vector2f mouseUI) {
  float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
  const float bW = 240.f, bH = 48.f;
  auto hit = [&](float btnY) {
    return mouseUI.x >= cx - bW / 2.f && mouseUI.x <= cx + bW / 2.f &&
           mouseUI.y >= btnY - bH / 2.f && mouseUI.y <= btnY + bH / 2.f;
  };

  if (hit(cy - 80.f)) {
    SoundManager::get().play(SoundManager::SFX::UI_CLICK);
    pauseMenuOpen_ = false;
    paused_ = false;
    settingsOpen_ = false;
    return;
  }
  if (hit(cy)) {
    SoundManager::get().play(SoundManager::SFX::UI_CLICK);
    settingsOpen_ = !settingsOpen_;
    return;
  }
  if (hit(cy + 80.f)) {
    SoundManager::get().play(SoundManager::SFX::UI_CLICK);
    pauseMenuOpen_ = false;
    paused_ = false;
    settingsOpen_ = false;
    gameState_ = GameState::Menu;
    window_.setView(window_.getDefaultView());
    SoundManager::get().stopAll();
    menu_.init();
    return;
  }
  if (hit(cy + 160.f)) {
    SoundManager::get().play(SoundManager::SFX::UI_CLICK);
    window_.close();
    return;
  }

  if (settingsOpen_) {
    const float panW = 320.f, sW = 280.f, sH = 160.f;
    float sX = cx + panW / 2.f + 20.f, sY = cy - sH / 2.f;
    const float pillW = 52.f, pillH = 24.f;
    float pillX = sX + sW - pillW - 14.f;
    if (mouseUI.x >= pillX && mouseUI.x <= pillX + pillW &&
        mouseUI.y >= sY + 56.f && mouseUI.y <= sY + 56.f + pillH) {
      SoundManager::get().play(SoundManager::SFX::UI_CLICK);
      SoundManager::get().toggleSfx();
      return;
    }
    if (mouseUI.x >= pillX && mouseUI.x <= pillX + pillW &&
        mouseUI.y >= sY + 100.f && mouseUI.y <= sY + 100.f + pillH) {
      SoundManager::get().play(SoundManager::SFX::UI_CLICK);
      SoundManager::get().toggleMusic();
      return;
    }
  }
}