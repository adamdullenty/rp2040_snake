// Snake for Adafruit Feather RP2040 + 128x64 OLED FeatherWing (SH1107)
//
// Controls:
//   A     = turn left
//   C     = turn right
//   B     = pause / resume  (or restart when game over)
//   BOOT  = restart when game over
//
// Libraries: Adafruit SH110x, Adafruit GFX, Adafruit NeoPixel, Adafruit BusIO
// Board: Adafruit Feather RP2040 (Earle Philhower core)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>

// Feather RP2040 OLED FeatherWing buttons
#define BUTTON_A 9
#define BUTTON_B 8
#define BUTTON_C 7

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 16
#endif

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Playfield: 4x4 pixel cells, score bar on top (8 px)
static const int CELL = 4;
static const int COLS = 32;          // 128 / 4
static const int ROWS = 14;          // (64 - 8) / 4
static const int MAX_LEN = COLS * ROWS;
static const int SCORE_H = 8;

enum Dir : int8_t { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };
enum GameState : uint8_t { READY, PLAYING, PAUSED, DEAD };

struct Point {
  int8_t x;
  int8_t y;
};

Point snake[MAX_LEN];
uint16_t length = 0;
Dir dir = RIGHT;
Dir nextDir = RIGHT;
Point food = {0, 0};
uint16_t score = 0;
uint16_t highScore = 0;
GameState state = READY;

unsigned long lastTick = 0;
unsigned long lastPixel = 0;
uint16_t hue = 0;
uint16_t tickMs = 140;

bool aDown = false, bDown = false, cDown = false, bootDown = false;

void setNeo(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

bool occupied(int8_t x, int8_t y, uint16_t from = 0) {
  for (uint16_t i = from; i < length; i++) {
    if (snake[i].x == x && snake[i].y == y) return true;
  }
  return false;
}

void placeFood() {
  // Random empty cell; fallback scan if RNG keeps hitting the snake
  for (uint16_t attempt = 0; attempt < 200; attempt++) {
    int8_t x = rp2040.hwrand32() % COLS;
    int8_t y = rp2040.hwrand32() % ROWS;
    if (!occupied(x, y)) {
      food = {x, y};
      return;
    }
  }
  for (int8_t y = 0; y < ROWS; y++) {
    for (int8_t x = 0; x < COLS; x++) {
      if (!occupied(x, y)) {
        food = {x, y};
        return;
      }
    }
  }
}

void resetGame() {
  length = 3;
  snake[0] = {8, 7};
  snake[1] = {7, 7};
  snake[2] = {6, 7};
  dir = RIGHT;
  nextDir = RIGHT;
  score = 0;
  tickMs = 140;
  placeFood();
  state = PLAYING;
  lastTick = millis();
}

void turnLeft() {
  nextDir = (Dir)((dir + 3) % 4);
}

void turnRight() {
  nextDir = (Dir)((dir + 1) % 4);
}

void handleInput() {
  bool a = digitalRead(BUTTON_A) == LOW;
  bool b = digitalRead(BUTTON_B) == LOW;
  bool c = digitalRead(BUTTON_C) == LOW;
  bool boot = BOOTSEL;

  if (a && !aDown) {
    if (state == PLAYING) turnLeft();
    else if (state == READY || state == DEAD) resetGame();
  }
  if (c && !cDown) {
    if (state == PLAYING) turnRight();
    else if (state == READY || state == DEAD) resetGame();
  }
  if (b && !bDown) {
    if (state == PLAYING) state = PAUSED;
    else if (state == PAUSED) {
      state = PLAYING;
      lastTick = millis();
    } else if (state == READY || state == DEAD) {
      resetGame();
    }
  }
  if (boot && !bootDown) {
    if (state == READY || state == DEAD || state == PAUSED) resetGame();
  }

  aDown = a;
  bDown = b;
  cDown = c;
  bootDown = boot;
}

void stepSnake() {
  // Reject 180° reverses queued in the same tick window
  if ((nextDir + 2) % 4 == dir) {
    // ignore reverse
  } else {
    dir = nextDir;
  }

  Point head = snake[0];
  switch (dir) {
    case UP:    head.y--; break;
    case DOWN:  head.y++; break;
    case LEFT:  head.x--; break;
    case RIGHT: head.x++; break;
  }

  // Wall collision
  if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
    state = DEAD;
    if (score > highScore) highScore = score;
    return;
  }

  bool ate = (head.x == food.x && head.y == food.y);

  // Self collision (allow moving into tail if it will vacate, unless eating)
  uint16_t checkLen = ate ? length : length - 1;
  for (uint16_t i = 0; i < checkLen; i++) {
    if (snake[i].x == head.x && snake[i].y == head.y) {
      state = DEAD;
      if (score > highScore) highScore = score;
      return;
    }
  }

  for (uint16_t i = length; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = head;

  if (ate) {
    if (length < MAX_LEN) length++;
    score++;
    // Speed up a bit every few apples, floor at 60ms
    if (score % 3 == 0 && tickMs > 60) tickMs -= 8;
    placeFood();
  }
}

void drawCell(int8_t cx, int8_t cy, bool filled) {
  int16_t x = cx * CELL;
  int16_t y = SCORE_H + cy * CELL;
  if (filled) {
    display.fillRect(x, y, CELL, CELL, SH110X_WHITE);
  } else {
    display.drawRect(x, y, CELL, CELL, SH110X_WHITE);
  }
}

void drawFrame() {
  display.clearDisplay();

  // Score bar
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.printf("S:%u", score);
  display.setCursor(64, 0);
  display.printf("Hi:%u", highScore);
  display.drawFastHLine(0, SCORE_H - 1, 128, SH110X_WHITE);

  if (state == READY) {
    display.setTextSize(2);
    display.setCursor(28, 22);
    display.print(F("SNAKE"));
    display.setTextSize(1);
    display.setCursor(10, 46);
    display.print(F("A/C turn  B pause"));
    display.setCursor(22, 56);
    display.print(F("any btn start"));
    display.display();
    return;
  }

  // Food (hollow so it reads differently from body)
  drawCell(food.x, food.y, false);

  // Snake body
  for (uint16_t i = 1; i < length; i++) {
    drawCell(snake[i].x, snake[i].y, true);
  }
  // Head slightly inset so it stands out
  {
    int16_t x = snake[0].x * CELL + 1;
    int16_t y = SCORE_H + snake[0].y * CELL + 1;
    display.fillRect(x, y, CELL - 2, CELL - 2, SH110X_WHITE);
  }

  if (state == PAUSED) {
    display.fillRect(28, 26, 72, 16, SH110X_BLACK);
    display.drawRect(28, 26, 72, 16, SH110X_WHITE);
    display.setCursor(40, 30);
    display.print(F("PAUSED"));
  }

  if (state == DEAD) {
    display.fillRect(16, 20, 96, 32, SH110X_BLACK);
    display.drawRect(16, 20, 96, 32, SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(34, 26);
    display.print(F("GAME OVER"));
    display.setCursor(28, 38);
    display.printf("Score %u", score);
    display.setCursor(22, 48);
    display.print(F("B/BOOT replay"));
  }

  display.display();
}

void updateNeoPixel() {
  unsigned long now = millis();
  if (now - lastPixel < 30) return;
  lastPixel = now;

  switch (state) {
    case READY:
      hue += 200;
      pixel.setPixelColor(0, pixel.gamma32(pixel.ColorHSV(hue, 255, 160)));
      break;
    case PLAYING: {
      // Green -> yellow as you score
      uint8_t g = 180;
      uint8_t r = min(160, score * 12);
      setNeo(r, g, 20);
      return;
    }
    case PAUSED:
      setNeo(20, 20, 160);
      return;
    case DEAD:
      setNeo(((now / 250) % 2) ? 180 : 40, 0, 0);
      return;
  }
  pixel.show();
}

void setup() {
  Serial.begin(115200);

#if defined(NEOPIXEL_POWER)
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif
  pixel.begin();
  pixel.setBrightness(45);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  delay(250);
  if (!display.begin(0x3C, true)) {
    display.begin(0x3D, true);
  }
  display.setRotation(1);
  display.clearDisplay();
  display.display();

  state = READY;
  drawFrame();
  Serial.println(F("Snake ready"));
}

void loop() {
  handleInput();
  updateNeoPixel();

  if (state == PLAYING) {
    unsigned long now = millis();
    if (now - lastTick >= tickMs) {
      lastTick = now;
      stepSnake();
      drawFrame();
    }
  } else {
    // Keep overlays responsive without constant full redraw spam
    static GameState lastDrawn = (GameState)255;
    static unsigned long lastUi = 0;
    if (state != lastDrawn || millis() - lastUi > 200) {
      drawFrame();
      lastDrawn = state;
      lastUi = millis();
    }
  }
}
