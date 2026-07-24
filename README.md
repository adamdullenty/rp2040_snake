# Snake for Adafruit RP2040 + OLED Display

Classic Snake clone on a 128×64 OLED.

## Screenshot (mockup)

![Gameplay](docs/screenshot.png)

_(TODO: Real screenshot + video)_


## Hardware

- **Board:** [Adafruit Feather RP2040](https://www.adafruit.com/product/4883)
- **Display:** [128×64 OLED FeatherWing](https://www.adafruit.com/product/4650) (SH1107 controller)
- **Core:** [Earle Philhower](https://github.com/earlephilhower/arduino-pico) Arduino core for RP2040

## Libraries

Install via Arduino Library Manager:

- Adafruit SH110x
- Adafruit GFX Library
- Adafruit BusIO
- Adafruit NeoPixel

## Build & upload

1. Open `rp2040_snake.ino` in the Arduino IDE.
2. Select **Board → Adafruit Feather RP2040** (Philhower core).
3. Install the libraries above if prompted.
4. Upload over USB.

## Controls

| Button | Action |
|--------|--------|
| **A** | Turn left |
| **C** | Turn right |
| **B** | Pause / resume (restart from title or game over) |
| **BOOT** | Restart when paused or game over |
