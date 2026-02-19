#include <Wire.h>
#include <U8g2lib.h>

// ===================== OLED =====================
#define SDA_PIN 4
#define SCL_PIN 5
static const uint8_t OLED_ADDR_8BIT = (0x3C << 1); // try 0x3D<<1 if blank
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===================== Pot =====================
#define POT_PIN 0
float potEMA = 0.0f;
const float POT_ALPHA = 0.10f;

int readPotSmooth() {
  int raw = analogRead(POT_PIN); // 0..4095
  potEMA = (1.0f - POT_ALPHA) * potEMA + POT_ALPHA * raw;
  return (int)(potEMA + 0.5f);
}

static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ===================== Game =====================
const int W = 128;
const int H = 64;

enum State { S_TITLE, S_PLAY, S_OVER };
State state = S_TITLE;
unsigned long stateMs = 0;

// Paddle
int padX = 0;
const int padY = H - 6;
int padW = 26;
const int padH = 3;

// Ball
float ballX = W / 2.0f;
float ballY = H / 2.0f;
float ballVX = 1.6f;
float ballVY = -1.8f;

// Bricks
const int brickRows = 4;
const int brickCols = 10;
bool brick[brickRows][brickCols];

const int brickW = 12; // 10*12 = 120 fits nicely
const int brickH = 6;
const int bricksX0 = 4;
const int bricksY0 = 12;

int score = 0;
int lives = 3;

bool hitRect(float x, float y, int rx, int ry, int rw, int rh) {
  return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

void resetBricks() {
  for (int r = 0; r < brickRows; r++)
    for (int c = 0; c < brickCols; c++)
      brick[r][c] = true;
}

bool anyBricksLeft() {
  for (int r = 0; r < brickRows; r++)
    for (int c = 0; c < brickCols; c++)
      if (brick[r][c]) return true;
  return false;
}

void resetBall(bool serveUp) {
  ballX = padX + padW / 2.0f;
  ballY = padY - 3;

  // slightly random-ish
  float dir = (millis() & 1) ? 1.4f : -1.4f;
  ballVX = dir;
  ballVY = serveUp ? -1.9f : 1.9f;
}

void startGame() {
  score = 0;
  lives = 3;
  padW = 26;
  resetBricks();
  state = S_PLAY;
  resetBall(true);
}

void loseLife() {
  lives--;
  if (lives <= 0) {
    state = S_OVER;
    stateMs = millis();
  } else {
    resetBall(true);
  }
}

void drawHUD() {
  u8g2.setFont(u8g2_font_5x8_tr);
  char buf[32];
  snprintf(buf, sizeof(buf), "S:%d  L:%d", score, lives);
  u8g2.drawStr(0, 8, buf);
  u8g2.drawHLine(0, 10, W);
}

void drawTitle() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(38, 18, "BREAKOUT");
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(10, 36, "Pot: Move paddle");
  u8g2.drawStr(10, 48, "Move to RIGHT end");
  u8g2.drawStr(10, 58, "to Start");
  u8g2.sendBuffer();
}

void drawGameOver() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(30, 18, "GAME OVER");
  u8g2.setFont(u8g2_font_5x8_tr);
  char buf[24];
  snprintf(buf, sizeof(buf), "Score: %d", score);
  u8g2.drawStr(38, 36, buf);
  u8g2.drawStr(10, 58, "Right end = Restart");
  u8g2.sendBuffer();
}

void drawPlay() {
  u8g2.clearBuffer();
  drawHUD();

  // Bricks
  for (int r = 0; r < brickRows; r++) {
    for (int c = 0; c < brickCols; c++) {
      if (!brick[r][c]) continue;
      int x = bricksX0 + c * brickW;
      int y = bricksY0 + r * brickH;
      u8g2.drawFrame(x, y, brickW - 1, brickH - 1);
    }
  }

  // Paddle
  u8g2.drawBox(padX, padY, padW, padH);

  // Ball
  u8g2.drawDisc((int)ballX, (int)ballY, 2, U8G2_DRAW_ALL);

  u8g2.sendBuffer();
}

// Start gesture: pot at far right for a bit
const int START_THRESHOLD = 4000;
const unsigned long START_HOLD_MS = 600;
unsigned long holdStartMs = 0;

bool potHoldStart(int pot) {
  if (pot >= START_THRESHOLD) {
    if (holdStartMs == 0) holdStartMs = millis();
    if (millis() - holdStartMs >= START_HOLD_MS) {
      holdStartMs = 0;
      return true;
    }
  } else {
    holdStartMs = 0;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(SDA_PIN, SCL_PIN);

  u8g2.begin();
  u8g2.setI2CAddress(OLED_ADDR_8BIT);

  pinMode(POT_PIN, INPUT);
  potEMA = analogRead(POT_PIN);

  state = S_TITLE;
  drawTitle();
}

void loop() {
  // Pot controls paddle
  int pot = readPotSmooth();
  padX = map(pot, 0, 4095, 0, W - padW);
  padX = clampi(padX, 0, W - padW);

  // State handling
  if (state == S_TITLE) {
    if (potHoldStart(pot)) startGame();
    drawTitle();
    delay(30);
    return;
  }

  if (state == S_OVER) {
    if (potHoldStart(pot)) {
      state = S_TITLE;
      drawTitle();
    } else {
      drawGameOver();
    }
    delay(30);
    return;
  }

  // ===== PLAY =====
  // Move ball
  ballX += ballVX;
  ballY += ballVY;

  // Wall bounce
  if (ballX <= 2) { ballX = 2; ballVX = -ballVX; }
  if (ballX >= W - 3) { ballX = W - 3; ballVX = -ballVX; }

  // Top bounce (below HUD line)
  if (ballY <= 12) { ballY = 12; ballVY = -ballVY; }

  // Paddle collision
  if (ballVY > 0 && hitRect(ballX, ballY, padX, padY - 2, padW, padH + 2)) {
    ballY = padY - 3;
    ballVY = -ballVY;

    // Add "spin" based on hit position
    float offset = (ballX - (padX + padW / 2.0f)) / (padW / 2.0f); // -1..1
    ballVX += offset * 0.8f;

    // Clamp vx so it doesn't go insane
    if (ballVX > 3.2f) ballVX = 3.2f;
    if (ballVX < -3.2f) ballVX = -3.2f;
  }

  // Brick collision (simple)
  for (int r = 0; r < brickRows; r++) {
    for (int c = 0; c < brickCols; c++) {
      if (!brick[r][c]) continue;
      int bx = bricksX0 + c * brickW;
      int by = bricksY0 + r * brickH;

      if (hitRect(ballX, ballY, bx, by, brickW - 1, brickH - 1)) {
        brick[r][c] = false;
        score += 5;
        ballVY = -ballVY; // simple bounce
        goto bricksDone;
      }
    }
  }
bricksDone:

  // Missed ball
  if (ballY > H + 2) {
    loseLife();
  }

  // Win condition: no bricks left
  if (!anyBricksLeft()) {
    // next level: refill bricks + slightly smaller paddle
    resetBricks();
    if (padW > 16) padW -= 2;
    resetBall(true);
  }

  drawPlay();
  delay(12);
}
