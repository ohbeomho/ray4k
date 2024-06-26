#include "button.hpp"
#include "judge.hpp"
#include "map.hpp"
#include "screen.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <functional>
#include <map>
#include <raylib.h>
#include <string>

// Window size
#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500

// Gameplay scroll speed
#define SCROLL_SPEED 3000
// Song select screen
#define MOUSE_SCROLL_SPEED 80

// Screen index
#define SONG_SELECT_SCREEN 0
#define GAMEPLAY_SCREEN 1
#define RESULT_SCREEN 2

class Hit {
public:
  Judgement judgement;
  int hitDiff;
  float alpha = 255.0f;
};

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray4K");
  InitAudioDevice();
  SetTargetFPS(500);

  SetExitKey(0);

  const char *message = NULL;

  // Load maps
  vector<Mapset> mapsets;
  try {
    mapsets = loadMaps(path("./songs"));
  } catch (const char *errMsg) {
    message = errMsg;
    return 1;
  }

  bool stop = false, pause = false;
  float timePlayed = -1.5f, judgeUpdated = -1.0f, diffUpdated = -1.0f;
  Beatmap *currentBeatmap = NULL;
  int currentScreen = SONG_SELECT_SCREEN;
  vector<Note> notes, longNotes;
  Music music;

  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};
  vector<Hit> hits;
  int prevGoodBadHit, hitCount;

  map<Judgement, int> judgements;
  map<Judgement, pair<string, Color>> judgementInfo;
  judgementInfo[Judgement::GREAT] = make_pair("GREAT", SKYBLUE);
  judgementInfo[Judgement::GOOD] = make_pair("GOOD", GREEN);
  judgementInfo[Judgement::BAD] = make_pair("BAD", DARKGRAY);
  judgementInfo[Judgement::MISS] = make_pair("MISS", RED);
  int judges[4] = {(int)Judgement::GREAT, (int)Judgement::GOOD,
                   (int)Judgement::BAD, (int)Judgement::MISS};
  int hitWindows[4] = {
      (int)JudgementHitWindow::GREAT, (int)JudgementHitWindow::GOOD,
      (int)JudgementHitWindow::BAD, (int)JudgementHitWindow::MISS};

  map<Grade, pair<char, Color>> gradeInfo;
  gradeInfo[Grade::S] = make_pair('S', YELLOW);
  gradeInfo[Grade::A] = make_pair('A', GREEN);
  gradeInfo[Grade::B] = make_pair('B', BLUE);
  gradeInfo[Grade::C] = make_pair('C', PURPLE);
  gradeInfo[Grade::D] = make_pair('D', GRAY);
  gradeInfo[Grade::F] = make_pair('F', RED);

  Judgement prevJudge;
  Grade grade = Grade::S;
  int combo, accScore;
  double score, scoreMultiplier;
  double acc = 100.0;
  float wait;

  // ms
  int offset = 50;

  auto judge = [&](int hitDiff) {
    int absHitDiff = abs(hitDiff);
    Judgement judge = calcScore(absHitDiff);

    if (judge == Judgement::GHOST)
      return judge;

    hits[hitCount].judgement = judge;
    hits[hitCount].hitDiff = hitDiff;
    hitCount++;

    if (judge != Judgement::MISS) {
      if (judge != Judgement::GREAT) {
        prevGoodBadHit = hitCount - 1;
        diffUpdated = timePlayed;
      }

      combo++;

      score += (double)judge * scoreMultiplier * ((double)combo / hitCount);
    } else
      combo = 0;

    accScore += (int)judge;
    acc = (double)accScore / hitCount;
    grade = calcGrade(acc);

    prevJudge = judge;
    judgeUpdated = timePlayed;
    judgements[prevJudge]++;

    return judge;
  };

  auto initState = [&]() {
    fill(hits.begin(), hits.end(), Hit());
    scoreMultiplier = 10000.0 / hits.size();
    timePlayed = -1.5f;
    judgeUpdated = -1.0f;
    diffUpdated = -1.0f;
    wait = 1.5f;
    combo = 0;
    score = 0;
    acc = 100.0;
    accScore = 0;
    grade = Grade::S;
    hitCount = 0;
    judgements[Judgement::GREAT] = 0;
    judgements[Judgement::GOOD] = 0;
    judgements[Judgement::BAD] = 0;
    judgements[Judgement::MISS] = 0;
    stop = false;
    pause = false;
  };

  auto startMap = [&](Beatmap *beatmap) {
    currentBeatmap = beatmap;
    music = LoadMusicStream(beatmap->musicPath.c_str());
    auto noteData = beatmap->loadNotes();
    notes = noteData.first;
    hits.resize(notes.size() + noteData.second);
    initState();
    currentScreen = GAMEPLAY_SCREEN;
  };

  int i;

  Screen screens[] = {
      // Song select screen
      Screen(
          [&screens, &pause]() {
            for (Button &button : screens[SONG_SELECT_SCREEN].buttons)
              button.draw();
          },
          [&]() {
            float mouseWheelMove = GetMouseWheelMove();
            for (Button &button : screens[SONG_SELECT_SCREEN].buttons) {
              float a = (float)(abs((float)WINDOW_HEIGHT / 2 - button.y) /
                                WINDOW_HEIGHT);
              button.y += mouseWheelMove * MOUSE_SCROLL_SPEED;
              button.x = (float)WINDOW_WIDTH / 3 + a * WINDOW_WIDTH / 2;
              button.color.a = (1 - a * 1.5) * 255;

              if (button.y + 80 < 0 || button.y > WINDOW_HEIGHT)
                continue;

              button.checkClick();
            }
          }),
      // Gameplay screen
      Screen(
          [&]() {
            // FPS
            string fpsStr = to_string(GetFPS());
            DrawText(fpsStr.c_str(),
                     WINDOW_WIDTH - MeasureText(fpsStr.c_str(), 40) - 10, 5, 40,
                     LIGHTGRAY);

            // Accuracy
            string accStr = format("{:.2f}", acc) + "%";
            DrawText(accStr.c_str(), 10, 10, 75, WHITE);

            // Grade
            char gradeStr[2];
            gradeStr[0] = gradeInfo[grade].first;
            gradeStr[1] = '\0';
            DrawText(gradeStr, 20 + MeasureText(accStr.c_str(), 75), 10, 75,
                     gradeInfo[grade].second);

            // Score
            DrawText(to_string((int)round(score)).c_str(), 10, 80, 75, WHITE);

            // Judgements
            for (i = 0; i < 4; i++)
              DrawText((judgementInfo[(Judgement)judges[i]].first + " " +
                        to_string(judgements[(Judgement)judges[i]]))
                           .c_str(),
                       10, 200 + 100 * i, 55,
                       judgementInfo[(Judgement)judges[i]].second);

            // Long Notes
            for (i = 0; i < notes.size(); i++) {
              Note note = notes[i];

              if (note.y == -1000)
                break;

              if (note.endTime != -1 && !note.judged) {
                double height =
                    (note.endTime - note.startTime) / 1000.0 * SCROLL_SPEED;

                DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                                  (note.lane - 1) * WINDOW_WIDTH / 8,
                              note.y - height, WINDOW_WIDTH / 8, height,
                              LIGHTGRAY);
              }
            }

            for (i = 0; i < 4; i++)
              DrawRectangle(WINDOW_WIDTH / 4 + (WINDOW_WIDTH / 8) * i,
                            WINDOW_HEIGHT - 300, WINDOW_WIDTH / 8, 120,
                            IsKeyDown(keys[i]) ? YELLOW : GRAY);

            // Notes
            for (i = 0; i < notes.size(); i++) {
              Note note = notes[i];

              if (note.y > WINDOW_HEIGHT || note.judged)
                continue;
              else if (note.y == -1000)
                break;

              DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                                (note.lane - 1) * WINDOW_WIDTH / 8,
                            note.y, WINDOW_WIDTH / 8, 120, WHITE);
            }

            DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 180,
                          WINDOW_WIDTH / 2, 180, Color{180, 180, 180, 255});

            // Previous Judgement
            if (timePlayed >= 0 && timePlayed - judgeUpdated <= 0.6f) {
              int fontSize = 80 + (0.2f - (timePlayed - judgeUpdated)) * 100;
              if (fontSize <= 80)
                fontSize = 80;
              DrawText(judgementInfo[prevJudge].first.c_str(),
                       WINDOW_WIDTH / 2 -
                           MeasureText(judgementInfo[prevJudge].first.c_str(),
                                       fontSize) /
                               2,
                       WINDOW_HEIGHT / 2, fontSize,
                       judgementInfo[prevJudge].second);
            }

            // Combo
            if (combo > 0) {
              int fontSize = 70 + (0.2f - (timePlayed - judgeUpdated)) * 100;
              if (fontSize < 70)
                fontSize = 70;

              DrawText(to_string(combo).c_str(),
                       WINDOW_WIDTH / 2 -
                           MeasureText(to_string(combo).c_str(), fontSize) / 2,
                       WINDOW_HEIGHT / 3, fontSize, Color{255, 255, 100, 255});
            }

            // Early/Late (Only shows in GOOD, BAD)
            if (timePlayed >= 0 && timePlayed - diffUpdated <= 0.6f) {
              int fontSize = 60 + (0.2f - (timePlayed - diffUpdated)) * 100,
                  prevHitDiff = hits[prevGoodBadHit].hitDiff;
              if (fontSize <= 60)
                fontSize = 60;

              string diffStr = (prevHitDiff > 0 ? "LATE " : "EARLY ") +
                               to_string(abs(prevHitDiff)) + "ms";

              DrawText(diffStr.c_str(),
                       WINDOW_WIDTH / 2 -
                           MeasureText(diffStr.c_str(), fontSize) / 2,
                       WINDOW_HEIGHT / 2 - fontSize - 10, fontSize,
                       prevHitDiff > 0 ? Color{150, 20, 20, 255} : LIME);
            }

            // Hit error meter
            int judgeHitWindows[3] = {(int)JudgementHitWindow::GREAT,
                                      (int)JudgementHitWindow::GOOD,
                                      (int)JudgementHitWindow::BAD};
            for (i = 2; i >= 0; i--) {
              Color color = judgementInfo[(Judgement)judges[i]].second;
              color.a = 200;
              DrawRectangle(WINDOW_WIDTH / 2 - (int)judgeHitWindows[i] * 2,
                            WINDOW_HEIGHT - 70, (int)judgeHitWindows[i] * 4, 20,
                            color);
            }

            for (i = 0; i < hitCount; i++) {
              if (hits[i].alpha <= 0)
                continue;

              Color color = judgementInfo[hits[i].judgement].second;
              color.a = hits[i].alpha;
              hits[i].alpha -= 255.0f / 3 * GetFrameTime();
              DrawRectangle(WINDOW_WIDTH / 2 + hits[i].hitDiff * 2 - 5,
                            WINDOW_HEIGHT - 100, 10, 80, color);
            }

            // Pause screen
            if (pause && wait <= 0) {
              DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                            Color{0, 0, 0, 200});

              for (Button &button : screens[GAMEPLAY_SCREEN].buttons)
                button.draw();
            }
          },
          [&]() {
            float frameTime = GetFrameTime();
            if (wait > 0) {
              wait -= frameTime;
              if (!pause)
                timePlayed += frameTime;

              if (wait <= 0) {
                if (stop) {
                  StopMusicStream(music);
                  currentScreen = RESULT_SCREEN;
                }

                wait = 0;
                pause = false;
              }
            } else {
              if (!IsMusicStreamPlaying(music) && !stop) {
                SetMusicVolume(music, 0.15f);
                PlayMusicStream(music);
              }

              if (!pause) {
                UpdateMusicStream(music);
              }

              timePlayed = GetMusicTimePlayed(music);
            }

            if (hitCount >= hits.size() && !stop) {
              wait = 1.5f;
              stop = true;
            }

            int i;
            bool pressedLane[4] = {false};

            if (IsKeyPressed(KEY_ESCAPE)) {
              // Resume after 0.4s delay
              if (pause)
                wait = 0.4f;
              else {
                pause = true;
              }
            }

            if (pause)
              return;

            for (i = 0; i < notes.size(); i++) {
              Note &note = notes[i];
              if ((timePlayed + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) <
                  note.startTime / 1000.0)
                break;
              else if (timePlayed > note.startTime ||
                       pressedLane[note.lane - 1] || note.judged)
                continue;

              // Scroll
              note.y =
                  WINDOW_HEIGHT - 300 -
                  ((note.startTime / 1000.0 - timePlayed + offset / 1000.0) *
                   SCROLL_SPEED);

              int startTimeDiff =
                  (int)(timePlayed * 1000) - note.startTime - offset;
              if (startTimeDiff > (int)JudgementHitWindow::BAD &&
                  startTimeDiff < (int)JudgementHitWindow::MISS &&
                  !note.pressed) {
                judge(startTimeDiff);
                if (note.endTime != -1)
                  judge(startTimeDiff);
                note.judged = true;
                continue;
              }

              // Note judge
              if (IsKeyPressed(keys[note.lane - 1])) {
                pressedLane[note.lane - 1] = true;

                if (judge(startTimeDiff) != Judgement::GHOST) {
                  if (note.endTime == -1)
                    note.judged = true;
                  else
                    note.pressed = true;
                }
              }

              // Long note release judge
              if (note.endTime != -1 && note.pressed) {
                int endTimeDiff =
                    (int)(timePlayed * 1000) - note.endTime - offset;
                if (endTimeDiff > (int)JudgementHitWindow::BAD &&
                    endTimeDiff < (int)JudgementHitWindow::MISS &&
                    IsKeyDown(keys[note.lane - 1])) {
                  judge(endTimeDiff);
                  note.judged = true;
                }

                if (IsKeyReleased(keys[note.lane - 1])) {
                  note.judged = true;

                  // Released too early
                  if (endTimeDiff < -(int)JudgementHitWindow::MISS) {
                    judge((int)JudgementHitWindow::MISS - 1);
                    continue;
                  }

                  judge(endTimeDiff);
                  pressedLane[note.lane - 1] = true;
                }
              }
            }
          }),
      // Result screen
      Screen(
          [&]() {
            // Score
            string scoreStr = to_string((int)round(score));
            DrawText(scoreStr.c_str(),
                     WINDOW_WIDTH / 4 - MeasureText(scoreStr.c_str(), 90) / 2,
                     100, 90, WHITE);

            // Judgements
            for (i = 0; i < 4; i++) {
              Judgement j = (Judgement)judges[i];
              string judgeStr =
                  judgementInfo[j].first + " " + to_string(judgements[j]);
              DrawText(judgeStr.c_str(),
                       WINDOW_HEIGHT / 4 -
                           MeasureText(judgeStr.c_str(), 70) / 2,
                       WINDOW_HEIGHT / 2 - 225 + (75 * i), 70,
                       judgementInfo[j].second);
            }

            // Hits
            DrawRectangle(WINDOW_WIDTH / 4 - WINDOW_WIDTH / 6,
                          WINDOW_HEIGHT - 400, WINDOW_WIDTH / 3, 300,
                          Color{255, 255, 255, 30});
            DrawLine(WINDOW_WIDTH / 4 - WINDOW_WIDTH / 6, WINDOW_HEIGHT - 250,
                     WINDOW_WIDTH / 4 + WINDOW_WIDTH / 6, WINDOW_HEIGHT - 250,
                     WHITE);

            for (i = 0; i < 3; i++) {
              Judgement j = (Judgement)judges[i];
              int hitWindow = hitWindows[i];
              int d = (double)hitWindow / (int)JudgementHitWindow::MISS * 150;
              int y1 = WINDOW_HEIGHT - 250 - d, y2 = WINDOW_HEIGHT - 250 + d;

              DrawLine(WINDOW_WIDTH / 4 - WINDOW_WIDTH / 6, y1,
                       WINDOW_WIDTH / 4 + WINDOW_WIDTH / 6, y1,
                       judgementInfo[j].second);
              DrawLine(WINDOW_WIDTH / 4 - WINDOW_WIDTH / 6, y2,
                       WINDOW_WIDTH / 4 + WINDOW_WIDTH / 6, y2,
                       judgementInfo[j].second);
            }

            for (i = 0; i < hits.size(); i++) {
              DrawCircle((double)WINDOW_WIDTH / 4 - (double)WINDOW_WIDTH / 6 +
                             10 +
                             (double)((double)(i + 1) / hits.size()) *
                                 ((double)WINDOW_WIDTH / 3 - 20),
                         WINDOW_HEIGHT - 250 +
                             (double)hits[i].hitDiff /
                                 (int)JudgementHitWindow::MISS * 150,
                         2.0f, judgementInfo[hits[i].judgement].second);
            }

            // Accuracy
            string accStr = format("{:.2f}", acc) + "%";
            DrawText(accStr.c_str(),
                     WINDOW_WIDTH / 2 + WINDOW_WIDTH / 4 -
                         MeasureText(accStr.c_str(), 80) / 2,
                     WINDOW_HEIGHT / 2 - 300, 80, WHITE);

            // Grade
            char gradeStr[2];
            gradeStr[0] = gradeInfo[grade].first;
            gradeStr[1] = '\0';
            DrawText(gradeStr,
                     WINDOW_WIDTH / 2 + WINDOW_WIDTH / 4 -
                         MeasureText("S", 500) / 2,
                     WINDOW_HEIGHT / 2 - 250, 500, gradeInfo[grade].second);
          },
          [&]() {
            if (IsKeyPressed(KEY_ESCAPE))
              currentScreen = SONG_SELECT_SCREEN;
          })};

  // Map screen buttons
  int y = 10, j;
  for (Mapset &mapset : mapsets) {
    for (Beatmap &beatmap : mapset.maps) {
      screens[SONG_SELECT_SCREEN].buttons.push_back(Button(
          beatmap.title + "\n" + string(100, ' ') + "\n[" + beatmap.diffName +
              "]",
          0, y, 50, [&startMap, &beatmap]() { startMap(&beatmap); }, WHITE));
      y += 220;
    }
  }

  // Pause screen buttons
  string buttonTexts[] = {"CONTINUE", "RESTART", "EXIT"};
  function<void()> buttonFuncs[] = {[&wait]() { wait = 0.3f; },
                                    [&]() {
                                      SeekMusicStream(music, 0);

                                      int i;
                                      for (i = 0; i < notes.size(); i++) {
                                        notes[i].judged = false;
                                        notes[i].pressed = false;
                                        notes[i].y = -1000;
                                      }

                                      initState();
                                    },
                                    [&music, &currentScreen]() {
                                      StopMusicStream(music);
                                      currentScreen = SONG_SELECT_SCREEN;
                                    }};

  for (i = 0; i < 3; i++) {
    screens[GAMEPLAY_SCREEN].buttons.push_back(
        Button(buttonTexts[i],
               WINDOW_WIDTH / 2 - MeasureText(buttonTexts[i].c_str(), 80) / 2,
               WINDOW_HEIGHT / 4 - 40 + (WINDOW_HEIGHT / 4) * i, 80,
               buttonFuncs[i], i == 2 ? RED : WHITE));
  }

  while (!WindowShouldClose()) {
    if (message) {
      BeginDrawing();
      DrawText(message, WINDOW_WIDTH / 2 - MeasureText(message, 50) / 2,
               WINDOW_HEIGHT / 2 - 25, 50, RED);
      EndDrawing();
      continue;
    }

    screens[currentScreen].update();
    screens[currentScreen].draw();
  }

  if (currentBeatmap != NULL)
    UnloadMusicStream(music);

  CloseWindow();
  CloseAudioDevice();

  return 0;
}
