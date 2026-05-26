#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "CharacterClass.hpp"
#include "ScoreSystem.hpp"
#include "SoundManager.hpp"
#include "dokho.hpp"

enum class MenuScreen { MainMenu, CharSelect, Settings, FadeOut };

class MenuSystem {
 public:
  struct Result {
    int charIndex = 0;
    Difficulty difficulty = Difficulty::Easy;
    bool quit = false;
  };

  MenuSystem(sf::RenderWindow& window, sf::Font& font)
      : window_(window), font_(font) {}

  void init() {
    updateWinSize();
    fadeAlpha_ = 255.f;
    screen_ = MenuScreen::MainMenu;
    active_ = true;
    done_ = false;
    hovered_ = -1;
    selectedChar_ = 0;
    selectedDiff_ = Difficulty::Easy;
    animTime_ = 0.f;

    // Load ảnh icon cho các class
    for (int i = 0; i < CharacterClass::CLASS_COUNT; ++i) {
      weaponIconTextures_[i].loadFromFile(
          CharacterClass::DEFS[i].weaponIconPath);
      weaponIconTextures_[i].setSmooth(true);
      charSpriteTextures_[i].loadFromFile(
          CharacterClass::DEFS[i].spriteSheetPath);
      charSpriteTextures_[i].setSmooth(false);  // pixel art — không smooth
    }

    // Load ảnh nền main menu
    bgTexture_.loadFromFile("hinh anh\\main_menu_bg.jpg");
    bgTexture_.setSmooth(true);
  }

  bool isActive() const { return active_; }
  bool isDone() const { return done_; }
  Result getResult() const { return result_; }

  void handleEvent(const sf::Event& event) {
    if (!active_ || done_) return;

    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
      // FIX: Chuyển pixel → tọa độ UI thông qua uiView_ thay vì dùng
      //      tọa độ pixel thô. Khi fullscreen/resize, pixel ≠ UI coords.
      mousePos_ = window_.mapPixelToCoords(mm->position, uiView_);
      updateHover();
    }
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (mb->button == sf::Mouse::Button::Left) {
        // FIX: Tương tự — map qua uiView_
        mousePos_ = window_.mapPixelToCoords(mb->position, uiView_);
        handleClick();
      }
    }
    if (const auto* kb = event.getIf<sf::Event::KeyPressed>()) {
      if (kb->code == sf::Keyboard::Key::Escape) {
        if (screen_ == MenuScreen::CharSelect ||
            screen_ == MenuScreen::Settings)
          screen_ = MenuScreen::MainMenu;
      }
    }
    // FIX: Cập nhật view khi cửa sổ thay đổi kích thước
    if (event.is<sf::Event::Resized>()) {
      updateWinSize();
    }
  }

  void update(float dt) {
    if (!active_) return;
    // FIX: Gọi updateWinSize mỗi frame để winW_/winH_ luôn đúng
    updateWinSize();
    animTime_ += dt;
    if (fadeAlpha_ > 0.f && screen_ != MenuScreen::FadeOut)
      fadeAlpha_ = std::max(0.f, fadeAlpha_ - dt * 400.f);
    if (screen_ == MenuScreen::FadeOut) {
      fadeAlpha_ = std::min(255.f, fadeAlpha_ + dt * 300.f);
      if (fadeAlpha_ >= 255.f) {
        active_ = false;
        done_ = true;
      }
    }
  }

  void render() {
    if (!active_) return;

    // FIX: Áp dụng uiView_ trước khi vẽ, đảm bảo tất cả vẽ trong không gian
    //      logic cố định (winW_ x winH_) bất kể kích thước cửa sổ thực tế.
    window_.setView(uiView_);

    sf::RectangleShape bg({winW_, winH_});
    bg.setFillColor(sf::Color(15, 12, 20));
    window_.draw(bg);

    sf::Sprite bgSpr(bgTexture_);
    auto ts = bgTexture_.getSize();
    bgSpr.setScale({winW_ / ts.x, winH_ / ts.y});
    uint8_t alpha = (screen_ == MenuScreen::MainMenu) ? 255 : 160;
    bgSpr.setColor(sf::Color(255, 255, 255, alpha));
    window_.draw(bgSpr);

    if (screen_ != MenuScreen::MainMenu) {
      sf::RectangleShape dim({winW_, winH_});
      dim.setFillColor(sf::Color(0, 0, 0, 100));
      window_.draw(dim);
    }

    drawParticles();
    if (screen_ == MenuScreen::MainMenu)
      drawMainMenu();
    else if (screen_ == MenuScreen::Settings)
      drawSettings();
    else
      drawCharSelect();
    if (fadeAlpha_ > 0.f) {
      sf::RectangleShape fade({winW_, winH_});
      fade.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(fadeAlpha_)));
      window_.draw(fade);
    }
  }

  // FIX: Expose uiView để Game.cpp có thể restore view sau menu
  const sf::View& getUIView() const { return uiView_; }

 private:
  // FIX: Hàm cập nhật kích thước cửa sổ + rebuild uiView_ letterbox
  void updateWinSize() {
    auto sz = window_.getSize();
    float newW = static_cast<float>(sz.x);
    float newH = static_cast<float>(sz.y);

    // Kích thước thiết kế cố định (giống WIN_W / WIN_H trong Game.hpp)
    // Thay đổi 2 giá trị này nếu project dùng resolution khác
    constexpr float DESIGN_W = 1280.f;
    constexpr float DESIGN_H = 720.f;

    winW_ = DESIGN_W;
    winH_ = DESIGN_H;

    // Letterbox: giữ tỉ lệ DESIGN, thêm pillarbox/letterbox nếu cần
    float scaleX = newW / DESIGN_W;
    float scaleY = newH / DESIGN_H;
    float scale = std::min(scaleX, scaleY);

    float vpW = (DESIGN_W * scale) / newW;
    float vpH = (DESIGN_H * scale) / newH;
    float vpX = (1.f - vpW) / 2.f;
    float vpY = (1.f - vpH) / 2.f;

    uiView_.setSize({DESIGN_W, DESIGN_H});
    uiView_.setCenter({DESIGN_W / 2.f, DESIGN_H / 2.f});
    uiView_.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));
  }

  // ── Settings Screen ───────────────────────────────────────
  void drawSettings() {
    float cx = winW_ * 0.5f, cy = winH_ * 0.5f;
    const float panW = 320.f, panH = 260.f;
    float panX = cx - panW * 0.5f, panY = cy - panH * 0.5f;
    drawPanel(panX, panY, panW, panH, sf::Color(18, 14, 30));

    sf::RectangleShape border({panW, panH});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(100, 160, 255, 180));
    border.setOutlineThickness(2.f);
    border.setPosition({panX, panY});
    window_.draw(border);

    drawText("CAI DAT AM THANH", 22, {cx, panY + 28.f},
             sf::Color(140, 190, 255), true);

    sf::RectangleShape div({panW - 40.f, 1.f});
    div.setFillColor(sf::Color(80, 80, 120, 150));
    div.setPosition({panX + 20.f, panY + 52.f});
    window_.draw(div);

    auto& sm = SoundManager::get();
    drawSettingsToggle("HIEU UNG AM THANH (SFX)", sm.isSfxEnabled(), cx,
                       panY + 90.f, 400);
    drawSettingsToggle("NHAC NEN (MUSIC)", sm.isMusicEnabled(), cx,
                       panY + 155.f, 401);

    float backW = 160.f, backH = 40.f;
    float backX = cx - backW * 0.5f, backY = panY + panH - 58.f;
    drawButton(backX, backY, backW, backH, "< QUAY LAI",
               sf::Color(160, 120, 200), hovered_ == 402);
    settingsBackRect_ = {backX, backY, backW, backH};

    drawText("ESC de quay lai", 11, {cx, panY + panH - 10.f},
             sf::Color(70, 65, 85), true);
  }

  void drawSettingsToggle(const std::string& label, bool on, float cx,
                          float rowY, int hoverId) {
    bool hov = (hovered_ == hoverId);
    if (hov) {
      sf::RectangleShape rowBg({280.f, 44.f});
      rowBg.setFillColor(sf::Color(60, 50, 90, 100));
      rowBg.setOrigin({140.f, 22.f});
      rowBg.setPosition({cx, rowY});
      window_.draw(rowBg);
    }
    drawText(label, 13, {cx - 120.f, rowY - 8.f},
             hov ? sf::Color(220, 220, 255) : sf::Color(190, 185, 210));

    const float pillW = 56.f, pillH = 26.f;
    float pillX = cx + 70.f, pillY = rowY - pillH * 0.5f;

    sf::RectangleShape pill({pillW, pillH});
    pill.setFillColor(on ? sf::Color(50, 200, 80) : sf::Color(70, 35, 35));
    pill.setOutlineColor(on ? sf::Color(80, 255, 110, 180)
                            : sf::Color(140, 60, 60, 180));
    pill.setOutlineThickness(1.5f);
    pill.setPosition({pillX, pillY});
    window_.draw(pill);

    sf::CircleShape knob(11.f);
    knob.setFillColor(sf::Color(240, 240, 240));
    knob.setOutlineColor(sf::Color(180, 180, 180, 120));
    knob.setOutlineThickness(1.f);
    knob.setPosition(
        {on ? (pillX + pillW - 24.f) : (pillX + 2.f), pillY + 2.f});
    window_.draw(knob);

    sf::Text onoff(font_, on ? "ON" : "OFF", 10);
    onoff.setFillColor(on ? sf::Color(180, 255, 190)
                          : sf::Color(200, 100, 100));
    auto ob = onoff.getLocalBounds();
    onoff.setOrigin(
        {ob.position.x + ob.size.x * 0.5f, ob.position.y + ob.size.y * 0.5f});
    onoff.setPosition({pillX + pillW * 0.5f, pillY + pillH * 0.5f - 1.f});
    window_.draw(onoff);

    std::array<float, 4> rect = {cx - 140.f, rowY - 22.f, 280.f, 44.f};
    if (hoverId == 400)
      settingsSfxRect_ = rect;
    else
      settingsMusicRect_ = rect;
  }

  // ── Main Menu ─────────────────────────────────────────────
  void drawMainMenu() {
    float cx = winW_ * 0.5f;
    float titY = winH_ * 0.22f;
    float bob = std::sin(animTime_ * 1.8f) * 5.f;

    float panW = 280.f, panH = 220.f;
    float panX = cx - panW * 0.5f, panY = winH_ * 0.46f;

    struct Btn {
      std::string label;
      int id;
      sf::Color col;
    };
    std::vector<Btn> btns = {
        {"PLAY", 0, sf::Color(220, 120, 60)},
        {"SETTINGS", 1, sf::Color(100, 140, 200)},
        {"QUIT", 2, sf::Color(160, 80, 80)},
    };

    mainMenuRects_.clear();
    float btnW = 200.f, btnH = 44.f;
    float btnX = cx - btnW * 0.5f, startY = panY + 28.f;
    for (int i = 0; i < (int)btns.size(); ++i) {
      float by = startY + i * (btnH + 14.f);
      drawButton(btnX, by, btnW, btnH, btns[i].label, btns[i].col,
                 hovered_ == i);
      mainMenuRects_.push_back({btnX, by, btnW, btnH});
    }
    drawText("Press ESC to quit", 11, {cx, winH_ - 22.f}, sf::Color(80, 70, 90),
             true);
  }

  // ── Char Select ───────────────────────────────────────────
  void drawCharSelect() {
    float cx = winW_ * 0.5f;
    drawText("CHON NHAN VAT", 28, {cx, winH_ * 0.08f}, sf::Color(220, 180, 100),
             true);
    drawText("Chon nhan vat va do kho", 13, {cx, winH_ * 0.08f + 34.f},
             sf::Color(140, 120, 150), true);

    const int N = 4;
    const float cardW = 155.f, cardH = 270.f, gap = 14.f;
    float totalW = N * cardW + (N - 1) * gap;
    float startX = cx - totalW * 0.5f;
    float cardY = winH_ * 0.18f;

    charCardRects_.clear();
    for (int i = 0; i < N; ++i) {
      float xcx = startX + i * (cardW + gap);
      bool hov = (hovered_ == 100 + i), sel = (selectedChar_ == i);
      drawCharCard(xcx, cardY, cardW, cardH, CharacterClass::DEFS[i], hov, sel);
      charCardRects_.push_back({xcx, cardY, cardW, cardH});
    }

    float diffLabelY = cardY + cardH + 22.f;
    drawText("Do Kho:", 14, {cx, diffLabelY}, sf::Color(200, 200, 200), true);

    float btnW = 100.f, btnH = 36.f, bGap = 12.f;
    float diffY = diffLabelY + 22.f;
    float diffBtnX = cx - btnW - bGap * 0.5f;

    drawDiffButton(diffBtnX, diffY, btnW, btnH, "EASY", sf::Color(80, 220, 80),
                   hovered_ == 300, selectedDiff_ == Difficulty::Easy);
    diffEasyRect_ = {diffBtnX, diffY, btnW, btnH};

    float hardBtnX = cx + bGap * 0.5f;
    drawDiffButton(hardBtnX, diffY, btnW, btnH, "HARD", sf::Color(220, 80, 80),
                   hovered_ == 301, selectedDiff_ == Difficulty::Hard);
    diffHardRect_ = {hardBtnX, diffY, btnW, btnH};

    const DifficultyConfig& cfg = DifficultyConfig::get(selectedDiff_);
    drawText(cfg.description, 11, {cx, diffY + btnH + 16.f},
             sf::Color(140, 140, 160), true);

    float startBtnW = 200.f, startBtnH = 48.f;
    float startBtnX = cx - startBtnW * 0.5f;
    float startBtnY = diffY + btnH + 42.f;
    drawButton(startBtnX, startBtnY, startBtnW, startBtnH, "BAT DAU",
               sf::Color(220, 120, 60), hovered_ == 200, true);
    startBtnRect_ = {startBtnX, startBtnY, startBtnW, startBtnH};

    drawText("< Quay lai  (ESC)", 12, {cx, startBtnY + startBtnH + 18.f},
             sf::Color(100, 90, 110), true);
  }

  void drawDiffButton(float x, float y, float w, float h,
                      const std::string& label, sf::Color col, bool hovered,
                      bool selected) {
    sf::Color bg = selected ? sf::Color(col.r / 2, col.g / 2, col.b / 2, 240)
                            : sf::Color(30, 25, 40, 220);
    sf::RectangleShape btn({w, h});
    btn.setFillColor(bg);
    btn.setOutlineColor(selected || hovered ? col : sf::Color(70, 60, 85));
    btn.setOutlineThickness(selected ? 2.5f : 1.f);
    btn.setPosition({x, y});
    window_.draw(btn);
    if (selected) {
      sf::RectangleShape bar({w, 4.f});
      bar.setFillColor(col);
      bar.setPosition({x, y});
      window_.draw(bar);
    }
    drawText(label, 14, {x + w / 2.f, y + h / 2.f - 8.f},
             selected ? sf::Color::White : sf::Color(180, 180, 180), true);
  }

  void drawCharCard(float x, float y, float w, float h, const CharClassDef& ch,
                    bool hovered, bool selected) {
    float lift = selected ? 8.f : 0.f;
    if (hovered) lift = std::min(lift + 5.f, 12.f);
    float bob = selected ? std::sin(animTime_ * 2.5f) * 3.f : 0.f;
    y -= (lift + bob);

    // ── Nền card ─────────────────────────────────────────────
    drawPanel(x, y, w, h,
              selected ? sf::Color(45, 35, 60) : sf::Color(28, 22, 38));
    if (selected || hovered) {
      sf::RectangleShape border({w, h});
      border.setFillColor(sf::Color(0, 0, 0, 0));
      border.setOutlineColor(selected ? ch.color : sf::Color(100, 90, 120));
      border.setOutlineThickness(selected ? 2.5f : 1.f);
      border.setPosition({x, y});
      window_.draw(border);
    }
    // Thanh màu class trên đỉnh card
    sf::RectangleShape topBar({w, 4.f});
    topBar.setFillColor(
        selected ? ch.color
                 : sf::Color(ch.color.r, ch.color.g, ch.color.b, 80));
    topBar.setPosition({x, y});
    window_.draw(topBar);

    float cxc = x + w * 0.5f;

    // ── Sprite nhân vật (to, pixel art) ──────────────────────
    int charIdx = &ch - CharacterClass::DEFS;
    const float spriteAreaH = 110.f;  // chiều cao vùng dành cho sprite
    const float spriteY = y + 10.f;

    auto& tex = charSpriteTextures_[charIdx];
    auto ts = tex.getSize();

    unsigned frameW = ts.x / 4;
    sf::IntRect frameRect({0, 0},
                          {static_cast<int>(frameW), static_cast<int>(ts.y)});

    sf::Sprite spr(tex, frameRect);
    float scale = std::min((w - 20.f) / static_cast<float>(frameW),
                           spriteAreaH / static_cast<float>(ts.y));
    if (scale > 2.f) scale = 2.f;
    if (scale < 0.5f) scale = 0.5f;

    spr.setScale({scale, scale});
    float sw = frameW * scale, sh = ts.y * scale;
    spr.setPosition({cxc - sw * 0.5f, spriteY + (spriteAreaH - sh) * 0.5f});
    window_.draw(spr);

    // ── Đường kẻ phân cách ───────────────────────────────────
    float divY = spriteY + spriteAreaH + 6.f;
    sf::RectangleShape div({w - 20.f, 1.f});
    div.setFillColor(sf::Color(ch.color.r, ch.color.g, ch.color.b, 60));
    div.setPosition({x + 10.f, divY});
    window_.draw(div);

    // ── Tên + vũ khí ─────────────────────────────────────────
    float nameY = divY + 10.f;
    drawText(ch.name, 17, {cxc, nameY}, sf::Color(230, 220, 240), true);

    // Icon vũ khí nhỏ + tên vũ khí
    float weapY = nameY + 24.f;
    sf::Sprite wSpr(weaponIconTextures_[charIdx]);
    auto wb = wSpr.getLocalBounds();
    float ws = 18.f / std::max(wb.size.x, wb.size.y);
    wSpr.setScale({ws, ws});
    wSpr.setPosition({cxc - 36.f, weapY - 9.f});
    window_.draw(wSpr);
    drawText(ch.weaponName, 11, {cxc + 4.f, weapY}, ch.color, true);

    // ── Mô tả ────────────────────────────────────────────────
    drawTextWrapped(ch.description, 10, x + 10.f, weapY + 20.f, w - 20.f,
                    sf::Color(150, 140, 160));

    // ── Stat box ─────────────────────────────────────────────
    float statY = y + h - 44.f;
    sf::RectangleShape sb({w - 16.f, 38.f});
    sb.setFillColor(sf::Color(20, 15, 30, 200));
    sb.setOutlineColor(sf::Color(ch.color.r, ch.color.g, ch.color.b, 60));
    sb.setOutlineThickness(1.f);
    sb.setPosition({x + 8.f, statY});
    window_.draw(sb);
    drawTextWrapped(ch.statLine, 10, x + 12.f, statY + 6.f, w - 24.f,
                    sf::Color(180, 200, 160));
  }

  // ── Helpers ───────────────────────────────────────────────
  void drawPanel(float x, float y, float w, float h,
                 sf::Color col = sf::Color(28, 22, 38)) {
    sf::RectangleShape shadow({w + 8.f, h + 8.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 80));
    shadow.setPosition({x + 4.f, y + 6.f});
    window_.draw(shadow);
    sf::RectangleShape panel({w, h});
    panel.setFillColor(col);
    panel.setOutlineColor(sf::Color(70, 55, 90, 180));
    panel.setOutlineThickness(1.f);
    panel.setPosition({x, y});
    window_.draw(panel);
  }

  void drawButton(float x, float y, float w, float h, const std::string& label,
                  sf::Color acc, bool hovered, bool big = false) {
    // ── Pixel-art button style ────────────────────────────
    // Khi hover: nhấn xuống 2px (press effect)
    float pressY = hovered ? 2.f : 0.f;

    // 1. Drop shadow (pixel style - offset cứng)
    sf::RectangleShape shadow({w, h});
    shadow.setFillColor(sf::Color(0, 0, 0, 160));
    shadow.setPosition({x + 4.f, y + 4.f + pressY});
    window_.draw(shadow);

    // 2. Viền ngoài màu vàng/vàng đồng (pixel border)
    const float B = 3.f;  // độ dày border pixel
    sf::Color borderCol =
        hovered ? sf::Color(255, 230, 80)   // sáng hơn khi hover
                : sf::Color(210, 170, 40);  // vàng đồng bình thường
    sf::RectangleShape outer({w, h});
    outer.setFillColor(borderCol);
    outer.setPosition({x, y + pressY});
    window_.draw(outer);

    // 3. Viền trong màu tối (tạo hiệu ứng khung 2 lớp)
    sf::RectangleShape inner({w - B * 2, h - B * 2});
    inner.setFillColor(sf::Color(30, 20, 80));
    inner.setPosition({x + B, y + B + pressY});
    window_.draw(inner);

    // 4. Nền chính màu xanh pixel
    const float P = B + 2.f;
    sf::Color btnBg = hovered ? sf::Color(70, 90, 210)   // sáng hơn khi hover
                              : sf::Color(45, 60, 175);  // xanh đậm
    sf::RectangleShape face({w - P * 2, h - P * 2});
    face.setFillColor(btnBg);
    face.setPosition({x + P, y + P + pressY});
    window_.draw(face);

    // 5. Highlight trên cùng (pixel shine)
    sf::RectangleShape shine({w - P * 2, 3.f});
    shine.setFillColor(sf::Color(120, 150, 255, hovered ? 80 : 120));
    shine.setPosition({x + P, y + P + pressY});
    window_.draw(shine);

    // 6. Text với shadow pixel
    unsigned sz = big ? 18u : 15u;
    float tx = x + w * 0.5f;
    float ty = y + h * 0.5f - (big ? 10.f : 8.f) + pressY;
    // shadow text
    sf::Text shadow_t(font_, label, sz);
    shadow_t.setFillColor(sf::Color(0, 0, 0, 200));
    auto b = shadow_t.getLocalBounds();
    shadow_t.setOrigin(
        {b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    shadow_t.setPosition({tx + 2.f, ty + 2.f});
    window_.draw(shadow_t);
    // main text
    sf::Text main_t(font_, label, sz);
    main_t.setFillColor(sf::Color::White);
    b = main_t.getLocalBounds();
    main_t.setOrigin(
        {b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    main_t.setPosition({tx, ty});
    window_.draw(main_t);
  }

  void drawText(const std::string& s, unsigned sz, sf::Vector2f pos,
                sf::Color col, bool centered = false) {
    sf::Text t(font_, s, sz);
    t.setFillColor(col);
    if (centered) {
      auto b = t.getLocalBounds();
      t.setOrigin(
          {b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    }
    t.setPosition(pos);
    window_.draw(t);
  }

  void drawTextWrapped(const std::string& str, unsigned sz, float x, float y,
                       float maxW, sf::Color col) {
    std::vector<std::string> words;
    std::string cur;
    for (char c : str) {
      if (c == ' ' || c == '\n') {
        if (!cur.empty()) {
          words.push_back(cur);
          cur.clear();
        }
        if (c == '\n') words.push_back("\n");
      } else
        cur += c;
    }
    if (!cur.empty()) words.push_back(cur);

    std::string line;
    float lineH = sz * 1.4f;
    int row = 0;
    for (auto& w : words) {
      if (w == "\n") {
        drawText(line, sz, {x, y + row * lineH}, col);
        line.clear();
        ++row;
        continue;
      }
      std::string test = line.empty() ? w : line + " " + w;
      sf::Text tmp(font_, test, sz);
      if (tmp.getLocalBounds().size.x > maxW && !line.empty()) {
        drawText(line, sz, {x, y + row * lineH}, col);
        line = w;
        ++row;
      } else
        line = test;
    }
    if (!line.empty()) drawText(line, sz, {x, y + row * lineH}, col);
  }

  void drawParticles() {
    for (int i = 0; i < 40; ++i) {
      float phase = (float)i / 40.f;
      float t = std::fmod(animTime_ * 0.3f + phase, 1.f);
      float px = winW_ * (0.1f + phase * 0.82f);
      float py = winH_ * (1.f - t);
      float a = std::sin(t * 3.14159f) * 120.f;
      float r = 1.5f + std::sin(phase * 7.3f + animTime_) * 1.f;
      sf::CircleShape p(r);
      p.setFillColor(sf::Color(180, 140, 220, static_cast<uint8_t>(a)));
      p.setPosition({px, py});
      window_.draw(p);
    }
  }

  bool inRect(float mx, float my, float rx, float ry, float rw, float rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
  }

  void updateHover() {
    hovered_ = -1;
    float mx = mousePos_.x, my = mousePos_.y;
    if (screen_ == MenuScreen::MainMenu) {
      for (int i = 0; i < (int)mainMenuRects_.size(); ++i) {
        auto& r = mainMenuRects_[i];
        if (inRect(mx, my, r[0], r[1], r[2], r[3])) {
          hovered_ = i;
          break;
        }
      }
    } else if (screen_ == MenuScreen::Settings) {
      if (inRect(mx, my, settingsSfxRect_[0], settingsSfxRect_[1],
                 settingsSfxRect_[2], settingsSfxRect_[3]))
        hovered_ = 400;
      else if (inRect(mx, my, settingsMusicRect_[0], settingsMusicRect_[1],
                      settingsMusicRect_[2], settingsMusicRect_[3]))
        hovered_ = 401;
      else if (inRect(mx, my, settingsBackRect_[0], settingsBackRect_[1],
                      settingsBackRect_[2], settingsBackRect_[3]))
        hovered_ = 402;
    } else {
      for (int i = 0; i < (int)charCardRects_.size(); ++i) {
        auto& r = charCardRects_[i];
        if (inRect(mx, my, r[0], r[1], r[2], r[3])) {
          hovered_ = 100 + i;
          break;
        }
      }
      auto& sb = startBtnRect_;
      if (inRect(mx, my, sb[0], sb[1], sb[2], sb[3])) hovered_ = 200;
      if (inRect(mx, my, diffEasyRect_[0], diffEasyRect_[1], diffEasyRect_[2],
                 diffEasyRect_[3]))
        hovered_ = 300;
      if (inRect(mx, my, diffHardRect_[0], diffHardRect_[1], diffHardRect_[2],
                 diffHardRect_[3]))
        hovered_ = 301;
    }
  }

  void handleClick() {
    float mx = mousePos_.x, my = mousePos_.y;
    bool clickedSomething = false;

    if (screen_ == MenuScreen::MainMenu) {
      for (int i = 0; i < (int)mainMenuRects_.size(); ++i) {
        auto& r = mainMenuRects_[i];
        if (!inRect(mx, my, r[0], r[1], r[2], r[3])) continue;
        clickedSomething = true;
        if (i == 0) {
          screen_ = MenuScreen::CharSelect;
          hovered_ = -1;
        } else if (i == 1) {
          screen_ = MenuScreen::Settings;
          hovered_ = -1;
        } else if (i == 2) {
          result_.quit = true;
          window_.close();
        }
        break;
      }
    } else if (screen_ == MenuScreen::Settings) {
      if (inRect(mx, my, settingsSfxRect_[0], settingsSfxRect_[1],
                 settingsSfxRect_[2], settingsSfxRect_[3])) {
        SoundManager::get().toggleSfx();
        clickedSomething = true;
      } else if (inRect(mx, my, settingsMusicRect_[0], settingsMusicRect_[1],
                        settingsMusicRect_[2], settingsMusicRect_[3])) {
        SoundManager::get().toggleMusic();
        clickedSomething = true;
      } else if (inRect(mx, my, settingsBackRect_[0], settingsBackRect_[1],
                        settingsBackRect_[2], settingsBackRect_[3])) {
        screen_ = MenuScreen::MainMenu;
        hovered_ = -1;
        clickedSomething = true;
      }
    } else {
      for (int i = 0; i < (int)charCardRects_.size(); ++i) {
        auto& r = charCardRects_[i];
        if (inRect(mx, my, r[0], r[1], r[2], r[3])) {
          selectedChar_ = i;
          clickedSomething = true;
          break;
        }
      }
      if (inRect(mx, my, diffEasyRect_[0], diffEasyRect_[1], diffEasyRect_[2],
                 diffEasyRect_[3])) {
        selectedDiff_ = Difficulty::Easy;
        clickedSomething = true;
      }
      if (inRect(mx, my, diffHardRect_[0], diffHardRect_[1], diffHardRect_[2],
                 diffHardRect_[3])) {
        selectedDiff_ = Difficulty::Hard;
        clickedSomething = true;
      }
      auto& sb = startBtnRect_;
      if (inRect(mx, my, sb[0], sb[1], sb[2], sb[3])) {
        result_.charIndex = selectedChar_;
        result_.difficulty = selectedDiff_;
        screen_ = MenuScreen::FadeOut;
        clickedSomething = true;
      }
    }

    if (clickedSomething) {
      SoundManager::get().play(SoundManager::SFX::UI_CLICK);
    }
  }

  // ── Members ───────────────────────────────────────────────
  sf::RenderWindow& window_;
  sf::Font& font_;
  sf::View uiView_;  // FIX: letterbox view

  float winW_ = 1280.f, winH_ = 720.f;
  float animTime_ = 0.f, fadeAlpha_ = 255.f;
  bool active_ = false, done_ = false;
  int hovered_ = -1, selectedChar_ = 0;
  Difficulty selectedDiff_ = Difficulty::Easy;

  MenuScreen screen_ = MenuScreen::MainMenu;
  Result result_;
  sf::Vector2f mousePos_;

  std::vector<std::array<float, 4>> mainMenuRects_;
  std::vector<std::array<float, 4>> charCardRects_;
  std::array<float, 4> startBtnRect_ = {};
  std::array<float, 4> diffEasyRect_ = {};
  std::array<float, 4> diffHardRect_ = {};

  std::array<float, 4> settingsSfxRect_ = {};
  std::array<float, 4> settingsMusicRect_ = {};
  std::array<float, 4> settingsBackRect_ = {};
  std::array<sf::Texture, CharacterClass::CLASS_COUNT> weaponIconTextures_;
  std::array<sf::Texture, CharacterClass::CLASS_COUNT> charSpriteTextures_;
  sf::Texture bgTexture_;
};