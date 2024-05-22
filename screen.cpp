#include "screen.hpp"
#include "raylib.h"

void Screen::draw() {
  BeginDrawing();
  ClearBackground(BLACK);

  drawScreen();

  EndDrawing();
}

void Screen::update() {
  for (Button &button : buttons)
    button.checkClick();

  updateScreen();
}

Screen::Screen(function<void()> _drawScreen, function<void()> _updateScreen) {
  drawScreen = _drawScreen;
  updateScreen = _updateScreen;
}
