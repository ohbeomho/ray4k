#ifndef _RAY4K_BUTTON_H_
#define _RAY4K_BUTTON_H_

#include <functional>
#include <raylib.h>
#include <string>

using namespace std;

class Button {
private:
  vector<string> lines;
  int maxLen;

public:
  int x, y, fontSize;
  function<void()> onClick;
  Color color;

  void checkClick();

  void draw();

  bool isButtonArea(int mouseX, int mouseY);

  Button(string _text, int _x, int _y, int _fontSize, function<void()> _onClick,
         Color _color);
};

#endif
