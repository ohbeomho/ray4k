#ifndef RAY4K_JUDGE_H_
#define RAY4K_JUDGE_H_

enum class Judgement { GREAT = 100, GOOD = 80, BAD = 50, MISS = 0, GHOST = -1 };
enum class JudgementHitWindow { GREAT = 50, GOOD = 80, BAD = 120, MISS = 160 };
enum class Grade { S = 95, A = 90, B = 85, C = 80, D = 70, F = 0 };

Judgement calcScore(int hitDiff);
Grade calcGrade(double acc);

#endif
