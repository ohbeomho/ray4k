#include "button.hpp"
#include <raylib.h>

using namespace std;

Button::Button(string _text, int _x, int _y, int _fontSize,
               function<void()> _onClick, Color _color) {
  x = _x;
  y = _y;
  onClick = _onClick;
  fontSize = _fontSize;
  color = _color;
  maxLen = 0;

  int i = _text.find('\n'), pi = i;
  if (i != string::npos) {
    lines.push_back(_text.substr(0, i + 1));
    while (true) {
      int newLineIndex = _text.substr(i + 1).find('\n');
      if (newLineIndex == string::npos)
        break;

      i += newLineIndex + 1;
      lines.push_back(_text.substr(pi, i - pi));
      if (lines[lines.size() - 1].length() > lines[maxLen].length())
        maxLen = lines.size() - 1;

      pi = i;
    }

    lines.push_back(_text.substr(pi));
    if (lines[lines.size() - 1].length() > lines[maxLen].length())
      maxLen = lines.size() - 1;
  } else
    lines.push_back(_text);
}

bool Button::isButtonArea(int mouseX, int mouseY) {
  // padding 10
  return mouseX >= x - 5 && mouseY >= y - 5 &&
         mouseX <= x + MeasureText(lines[maxLen].c_str(), fontSize) + 5 &&
         mouseY <= y + fontSize * lines.size() + 5;
}

void Button::checkClick() {
  int mouseX = GetMouseX(), mouseY = GetMouseY();

  if (isButtonArea(mouseX, mouseY) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    onClick();
}

void Button::draw() {
  int mouseX = GetMouseX(), mouseY = GetMouseY();

  int i;
  for (i = 0; i < lines.size(); i++)
    DrawText(lines[i].c_str(), x, y + fontSize * i, fontSize, color);

  Color c = Color{255, 255, 255, 25};
  if (isButtonArea(mouseX, mouseY))
    c.a = 50;

  DrawRectangle(x - 15, y - 15,
                MeasureText(lines[maxLen].c_str(), fontSize) + 30,
                fontSize * lines.size() + 30, c);
}
