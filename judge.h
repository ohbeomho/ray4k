#ifndef RAY4K_JUDGE_H_
#define RAY4K_JUDGE_H_

enum class Judgement { GREAT = 100, GOOD = 75, BAD = 35, MISS = 0, GHOST = -1 };
enum class JudgementHitWindow { GREAT = 60, GOOD = 110, BAD = 180, MISS = 220 };

Judgement calcScore(int hitDiff);

#endif
