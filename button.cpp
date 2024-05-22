#include "button.hpp"
#include <raylib.h>

using namespace std;

Button::Button(string _text, int _x, int _y, int _fontSize,
               function<void()> _onClick, Color _color) {
  text = _text;
  x = _x;
  y = _y;
  onClick = _onClick;
  fontSize = _fontSize;
  color = _color;
}

bool Button::isButtonArea(int mouseX, int mouseY) {
  // padding 5
  return mouseX >= x - 5 && mouseY >= y - 5 &&
         mouseX <= x + MeasureText(text.c_str(), fontSize) + 5 &&
         mouseY <= y + fontSize + 5;
}

void Button::checkClick() {
  int mouseX = GetMouseX(), mouseY = GetMouseY();

  if (isButtonArea(mouseX, mouseY) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    onClick();
}

void Button::draw() {
  int mouseX = GetMouseX(), mouseY = GetMouseY();

  DrawText(text.c_str(), x, y, fontSize, color);

  if (isButtonArea(mouseX, mouseY)) {
    DrawRectangle(x - 5, y - 5, MeasureText(text.c_str(), fontSize) + 10,
                  fontSize + 10, Color{255, 255, 255, 50});
  }
}
