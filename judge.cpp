#include "judge.h"

Judgement calcScore(int hitDiff) {
  if (hitDiff <= (int)JudgementHitWindow::GREAT)
    return Judgement::GREAT;
  else if (hitDiff <= (int)JudgementHitWindow::GOOD)
    return Judgement::GOOD;
  else if (hitDiff <= (int)JudgementHitWindow::BAD)
    return Judgement::BAD;
  else if (hitDiff <= (int)JudgementHitWindow::MISS)
    return Judgement::MISS;
  return Judgement::GHOST;
}
