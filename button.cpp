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
         mouseY <= y + fontSize + 5 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Button::checkClick(int mouseX, int mouseY) {
  if (isButtonArea(mouseX, mouseY))
    onClick();
}

void Button::draw(int mouseX, int mouseY) {
  DrawText(text.c_str(), x, y, fontSize, color);

  if (isButtonArea(mouseX, mouseY)) {
    DrawRectangle(x - 5, y - 5, MeasureText(text.c_str(), 80) + 10, 90,
                  Color{255, 255, 255, 50});
  }
}
