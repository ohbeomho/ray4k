#include "button.hpp"
#include "judge.hpp"
#include "map.hpp"
#include "screen.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
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
#define MOUSE_SCROLL_SPEED 50

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

  Judgement prevJudge;
  int combo;
  double score, scoreMultiplier;
  float wait;

  // ms
  int offset = 60;

  auto judge = [&](int hitDiff) {
    int absHitDiff = abs(hitDiff);
    Judgement judge = calcScore(absHitDiff);

    if (judge == Judgement::GHOST)
      return judge;

    if (judge != Judgement::MISS) {
      if (judge != Judgement::GREAT) {
        prevGoodBadHit = hitCount;
        diffUpdated = timePlayed;
      }

      combo++;

      score += (double)judge * scoreMultiplier;
    } else
      combo = 0;

    hits[hitCount].judgement = judge;
    hits[hitCount].hitDiff = hitDiff;
    hitCount++;

    prevJudge = judge;
    judgeUpdated = timePlayed;
    judgements[prevJudge]++;

    return judge;
  };

  auto startMap = [&](Beatmap *beatmap) {
    currentBeatmap = beatmap;
    music = LoadMusicStream(beatmap->musicPath.c_str());
    notes = beatmap->notes;
    hits.resize(notes.size() + beatmap->longNoteCount);
    scoreMultiplier = 10000.0 / hits.size();
    fill(hits.begin(), hits.end(), Hit());
    timePlayed = -1.5f;
    judgeUpdated = -1.0f;
    diffUpdated = -1.0f;
    wait = 1.5f;
    combo = 0;
    score = 0l;
    hitCount = 0;
    judgements[Judgement::GREAT] = 0;
    judgements[Judgement::GOOD] = 0;
    judgements[Judgement::BAD] = 0;
    judgements[Judgement::MISS] = 0;
    stop = false;
    pause = false;
    currentScreen = 1;
  };

  int i;

  Screen screens[] = {
      // Main screen (Song select)
      Screen(
          [&screens, &pause]() {
            for (Button &button : screens[SONG_SELECT_SCREEN].buttons)
              button.draw();
          },
          [&]() {
            float mouseWheelMove = GetMouseWheelMove();
            for (Button &button : screens[SONG_SELECT_SCREEN].buttons) {
              button.y += mouseWheelMove * MOUSE_SCROLL_SPEED;

              if (button.y + 80 < 0 || button.y > WINDOW_HEIGHT)
                continue;

              button.checkClick();
            }
          }),
      // Gameplay screen
      Screen(
          [&]() {
            // FPS
            string fpsText = to_string(GetFPS());
            DrawText(fpsText.c_str(),
                     WINDOW_WIDTH - MeasureText(fpsText.c_str(), 40) - 10, 5,
                     40, LIGHTGRAY);

            // Score
            DrawText(to_string((int)round(score)).c_str(), 10, 10, 75, WHITE);

            // Judgements
            int judges[4] = {(int)Judgement::GREAT, (int)Judgement::GOOD,
                             (int)Judgement::BAD, (int)Judgement::MISS};
            for (i = 0; i < 4; i++)
              DrawText((judgementInfo[(Judgement)judges[i]].first + " " +
                        to_string(judgements[(Judgement)judges[i]]))
                           .c_str(),
                       10, 10 + 100 * (i + 1), 55,
                       judgementInfo[(Judgement)judges[i]].second);

            // Long Notes
            for (i = 0; i < notes.size(); i++) {
              Note note = notes[i];

              if (note.y == -1000)
                break;

              if (!note.judged && note.endTime != -1) {
                DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                                  (note.lane - 1) * WINDOW_WIDTH / 8,
                              note.y - (note.endTime - note.startTime) /
                                           1000.0 * SCROLL_SPEED,
                              WINDOW_WIDTH / 8,
                              (note.endTime - note.startTime) / 1000.0 *
                                  SCROLL_SPEED,
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

              string text = (prevHitDiff > 0 ? "LATE " : "EARLY ") +
                            to_string(abs(prevHitDiff)) + "ms";

              DrawText(text.c_str(),
                       WINDOW_WIDTH / 2 -
                           MeasureText(text.c_str(), fontSize) / 2,
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
                            Color{0, 0, 0, 130});

              for (Button &button : screens[GAMEPLAY_SCREEN].buttons)
                button.draw();
            }
          },
          [&]() {
            if (wait > 0) {
              float frameTime = GetFrameTime();
              wait -= frameTime;
              if (!pause)
                timePlayed += frameTime;

              if (wait <= 0) {
                if (stop) {
                  currentScreen = 0;
                  StopMusicStream(music);
                }

                wait = 0;
                pause = false;
              }
            } else {
              if (!IsMusicStreamPlaying(music) && !stop) {
                SetMusicVolume(music, 0.15f);
                PlayMusicStream(music);
              }

              if (!pause)
                UpdateMusicStream(music);

              timePlayed = GetMusicTimePlayed(music);
            }

            if (hitCount >= hits.size() && !stop) {
              wait = 1.5f;
              stop = true;
            }

            int i;
            bool pressedLane[4] = {false};

            if (IsKeyPressed(KEY_ESCAPE)) {
              // Resume after 0.3s delay
              if (pause)
                wait = 0.3f;
              else
                pause = true;
            }

            if (pause)
              return;

            for (i = 0; i < notes.size(); i++) {
              Note &note = notes[i];
              if ((timePlayed + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) <
                  note.startTime / 1000.0)
                break;
              else if (timePlayed > note.startTime || note.judged ||
                       pressedLane[note.lane - 1])
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
          })};

  // Map screen buttons
  int y = 10, j;
  for (Mapset &mapset : mapsets) {
    for (Beatmap &beatmap : mapset.maps) {
      screens[SONG_SELECT_SCREEN].buttons.push_back(Button(
          beatmap.title.append(" [" + beatmap.diffName + "]"), 10, y, 50,
          [&startMap, &beatmap]() { startMap(&beatmap); }, WHITE));
      y += 60;
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

                                      fill(hits.begin(), hits.end(), Hit());
                                      timePlayed = -1.5f;
                                      judgeUpdated = -1.0f;
                                      diffUpdated = -1.0f;
                                      wait = 1.5f;
                                      score = 0l;
                                      hitCount = 0;
                                      combo = 0;
                                      judgements[Judgement::GREAT] = 0;
                                      judgements[Judgement::GOOD] = 0;
                                      judgements[Judgement::BAD] = 0;
                                      judgements[Judgement::MISS] = 0;
                                      stop = false;
                                      pause = false;
                                    },
                                    [&music, &currentScreen]() {
                                      StopMusicStream(music);
                                      currentScreen = 0;
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
