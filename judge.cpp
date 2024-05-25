#include "judge.hpp"

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

Grade calcGrade(double acc) {
  if (acc >= (double)Grade::S)
    return Grade::S;
  else if (acc >= (double)Grade::A)
    return Grade::A;
  else if (acc >= (double)Grade::B)
    return Grade::B;
  else if (acc >= (double)Grade::C)
    return Grade::C;
  else if (acc >= (double)Grade::D)
    return Grade::D;
  return Grade::F;
}
