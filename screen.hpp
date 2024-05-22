#ifndef _RAY4K_SCREEN_H_
#define _RAY4K_SCREEN_H_

#include "button.hpp"
#include <vector>

using namespace std;

class Screen {
private:
  function<void()> drawScreen, updateScreen;

public:
  vector<Button> buttons;

  void draw();
  void update();

  Screen(function<void()> _drawScreen, function<void()> _updateScreen);
};

#endif
