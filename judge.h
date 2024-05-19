#ifndef RAY4K_JUDGE_H_
#define RAY4K_JUDGE_H_

enum class Judgement { GREAT = 100, GOOD = 75, BAD = 35, MISS = 0, GHOST = -1 };
enum class JudgementHitWindow { GREAT = 70, GOOD = 120, BAD = 190, MISS = 230 };

Judgement calcScore(int hitDiff);

#endif
