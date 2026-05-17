#pragma once
// ════════════════════════════════════════════════════════════
//  SoundManager.hpp  —  Quản lý toàn bộ âm thanh game
// ════════════════════════════════════════════════════════════

#include <SFML/Audio.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class SoundManager {
 public:
  // ── Enum SFX ─────────────────────────────────────────────
  enum class SFX {
    SHOOT = 0,
    HIT_MONSTER,
    MONSTER_DIE,
    PLAYER_HIT,
    PLAYER_DIE,
    LEVEL_UP,
    PICKUP_EXP,
    LIGHTNING,
    GARLIC_TICK,
    BOSS_APPEAR,
    UPGRADE_SELECT,
    COUNT
  };

  // ── Enum BGM ─────────────────────────────────────────────
  enum class BGM { NONE, MENU_BGM, GAME_BGM, BOSS_BGM, GAMEOVER_BGM };

  // ── Singleton ─────────────────────────────────────────────
  static SoundManager& get() {
    static SoundManager instance;
    return instance;
  }

  // ── Khởi tạo ─────────────────────────────────────────────
  bool init() {
    bool allOk = true;

    static const char* SFX_FILES[static_cast<int>(SFX::COUNT)] = {
        "audio/shoot.wav",          "audio/hit_monster.wav",
        "audio/monster_die.wav",    "audio/player_hit.wav",
        "audio/player_die.wav",     "audio/level_up.wav",
        "audio/pickup_exp.wav",     "audio/lightning.wav",
        "audio/garlic_tick.wav",    "audio/boss_appear.wav",
        "audio/upgrade_select.wav",
    };

    for (int i = 0; i < static_cast<int>(SFX::COUNT); ++i) {
      if (!buffers_[i].loadFromFile(SFX_FILES[i])) {
        std::cerr << "[SoundManager] Missing SFX: " << SFX_FILES[i] << "\n";
        allOk = false;
        bufferOk_[i] = false;
      } else {
        bufferOk_[i] = true;
      }
    }

    // Khởi tạo sound pool SAU khi buffers_ đã load xong
    soundPool_.clear();
    soundPool_.reserve(MAX_SOUNDS);
    for (int i = 0; i < MAX_SOUNDS; ++i) soundPool_.emplace_back(buffers_[0]);

    music_ = std::make_unique<sf::Music>();
    initialized_ = true;
    std::cout << "[SoundManager] Init xong. SFX ok=" << countOk() << "/"
              << static_cast<int>(SFX::COUNT) << "\n";
    return allOk;
  }

  // ── Phát SFX ─────────────────────────────────────────────
  void play(SFX sfx, float pitch = 1.0f) {
    if (!initialized_ || !sfxEnabled_) return;
    int idx = static_cast<int>(sfx);
    if (!bufferOk_[idx]) return;

    sf::Sound* slot = findFreeSlot();
    if (!slot) return;

    slot->setBuffer(buffers_[idx]);
    slot->setVolume(sfxVolume_);
    slot->setPitch(pitch + pitchVariance());
    slot->play();
  }

  void playVaried(SFX sfx) { play(sfx, 1.0f); }

  // ── tick(): GỌI MỖI FRAME từ Game::update() ──────────────
  // Phát hiện non-loop music đã kết thúc tự nhiên để reset musicOpened_.
  // Đây là fix cốt lõi: GAMEOVER_BGM phát với loop=false, khi hết bài
  // SFML đóng internal reader. Nếu không tick() để reset flag, lần sau
  // stopAll() gọi music_->stop() trên reader đã null → crash m_reader assert.
  void tick() {
    if (musicOpened_) {
      if (music_->getStatus() == sf::Music::Status::Stopped) {
        musicOpened_ = false;
        currentBgm_ = BGM::NONE;
      }
    }
  }

  // ── BGM ──────────────────────────────────────────────────
  void playMusic(BGM bgm, bool loop = true) {
    // Chỉ skip nếu đúng track VÀ stream đang thực sự mở.
    // KHÔNG chỉ check currentBgm_ == bgm vì sau stopAll() musicOpened_=false
    // nhưng currentBgm_ có thể vẫn là track cũ → skip sai → stream không open.
    if (currentBgm_ == bgm && musicOpened_) return;

    // Dừng stream cũ TRƯỚC khi gán currentBgm_ mới
    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = bgm;

    if (bgm == BGM::NONE) return;

    const char* file = bgmFile(bgm);
    // SFML 3: sau một openFromFile thất bại, m_reader = null.
    // Lần gọi openFromFile tiếp theo SFML gọi close() → deref m_reader null →
    // crash. Giải pháp dứt điểm: reset unique_ptr để destroy sf::Music cũ và
    // tạo mới.
    music_ = std::make_unique<sf::Music>();
    if (!music_->openFromFile(file)) {
      std::cerr << "[SoundManager] Missing BGM: " << file << "\n";
      music_ =
          std::make_unique<sf::Music>();  // reset lại để tránh broken state
      currentBgm_ = BGM::NONE;
      return;
    }
    musicOpened_ = true;
    music_->setLooping(loop);
    music_->setVolume(bgm == BGM::GAMEOVER_BGM ? 20.f : musicVolume_);
    music_->play();
  }

  void stopMusic() {
    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = BGM::NONE;
  }

  void pauseMusic() {
    if (musicOpened_) music_->pause();
  }

  void resumeMusic() {
    if (musicOpened_) music_->play();
  }

  void stopAll() {
    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = BGM::NONE;
    for (auto& s : soundPool_) s.stop();
    std::cout << "[SoundManager] Stopped all sounds and music.\n";
  }

  // ── Volume ───────────────────────────────────────────────
  void setSfxVolume(float v) { sfxVolume_ = std::max(0.f, std::min(100.f, v)); }

  void setMusicVolume(float v) {
    musicVolume_ = std::max(0.f, std::min(100.f, v));
    if (musicOpened_) music_->setVolume(musicVolume_);
  }

  float getSfxVolume() const { return sfxVolume_; }
  float getMusicVolume() const { return musicVolume_; }

  // ── Mute ─────────────────────────────────────────────────
  void setMuted(bool muted) {
    muted_ = muted;
    setSfxVolume(muted ? 0.f : savedSfxVol_);
    setMusicVolume(muted ? 0.f : savedMusicVol_);
  }

  void toggleMute() {
    if (!muted_) {
      savedSfxVol_ = sfxVolume_;
      savedMusicVol_ = musicVolume_;
    }
    setMuted(!muted_);
  }

  bool isMuted() const { return muted_; }

  // ── SFX enable/disable ───────────────────────────────────
  bool isSfxEnabled() const { return sfxEnabled_; }

  void toggleSfx() {
    sfxEnabled_ = !sfxEnabled_;
    if (!sfxEnabled_)
      for (auto& s : soundPool_) s.stop();
  }

  // ── Music enable/disable ─────────────────────────────────
  bool isMusicEnabled() const { return musicEnabled_; }

  void toggleMusic() {
    musicEnabled_ = !musicEnabled_;
    if (!musicOpened_) return;
    if (musicEnabled_) {
      music_->setVolume(musicVolume_);
      if (currentBgm_ != BGM::NONE) music_->play();
    } else {
      music_->pause();
    }
  }

 private:
  SoundManager() = default;
  SoundManager(const SoundManager&) = delete;
  SoundManager& operator=(const SoundManager&) = delete;

  static constexpr int MAX_SOUNDS = 16;

  std::array<sf::SoundBuffer, static_cast<int>(SFX::COUNT)> buffers_;
  std::array<bool, static_cast<int>(SFX::COUNT)> bufferOk_ = {};
  std::vector<sf::Sound> soundPool_;

  std::unique_ptr<sf::Music> music_;
  BGM currentBgm_ = BGM::NONE;
  bool musicOpened_ = false;

  float sfxVolume_ = 100.f;
  float musicVolume_ = 30.f;
  float savedSfxVol_ = 100.f;
  float savedMusicVol_ = 30.f;
  bool muted_ = false;
  bool initialized_ = false;
  bool sfxEnabled_ = true;
  bool musicEnabled_ = true;

  sf::Sound* findFreeSlot() {
    for (auto& s : soundPool_)
      if (s.getStatus() == sf::Sound::Status::Stopped) return &s;
    soundPool_[0].stop();
    return &soundPool_[0];
  }

  static float pitchVariance() {
    return (static_cast<float>(std::rand() % 11) - 5) * 0.01f;
  }

  int countOk() const {
    int n = 0;
    for (bool ok : bufferOk_) n += ok ? 1 : 0;
    return n;
  }

  static const char* bgmFile(BGM bgm) {
    switch (bgm) {
      case BGM::MENU_BGM:
        return "audio/bgm_menu.ogg";
      case BGM::GAME_BGM:
        return "audio/bgm_game.ogg";
      case BGM::BOSS_BGM:
        return "audio/bgm_boss.ogg";
      case BGM::GAMEOVER_BGM:
        return "audio/bgm_gameover.mp3";
      default:
        return "";
    }
  }
};