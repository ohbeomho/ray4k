#include "judge.hpp"
#include "map.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <raylib.h>
#include <string>

// Window size
#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500

#define SCROLL_SPEED 3000

class Hit {
public:
  Judgement judgement;
  int hitDiff;
  float alpha = 255.0f;
};

// TODO: Make able to customize (set offset, colors, etc.)
int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray4K");
  InitAudioDevice();
  SetTargetFPS(500);

  const char *message = NULL;

  // Load maps
  vector<Mapset> mapsets;
  try {
    mapsets = loadMaps(path("./songs"));
  } catch (const char *errMsg) {
    message = errMsg;
    return 1;
  }

  bool playing = false, stop = false, pause = false;
  float timePlayed = -1.5f, judgeUpdated = -1.0f, diffUpdated = -1.0f;
  Beatmap *currentMap = NULL;
  vector<Note> notes, longNotes;
  Music music;

  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};
  vector<Hit> hits;
  int prevGoodBadHit, hitCount = 0;

  map<Judgement, int> judgements;
  map<Judgement, pair<string, Color>> judgementInfo;
  judgementInfo[Judgement::GREAT] = make_pair("GREAT", SKYBLUE);
  judgementInfo[Judgement::GOOD] = make_pair("GOOD", GREEN);
  judgementInfo[Judgement::BAD] = make_pair("BAD", DARKGRAY);
  judgementInfo[Judgement::MISS] = make_pair("MISS", RED);

  Judgement prevJudge = Judgement::GHOST;
  long score = 0;
  int combo = 0;
  float wait = 1.5f;

  // ms
  int offset = 60;

  auto judge = [&prevJudge, &hits, &judgements, &judgeUpdated, &score,
                &timePlayed, &judgementInfo, &diffUpdated, &prevGoodBadHit,
                &hitCount, &combo](int hitDiff) {
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

      score += (int)judge * combo;
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

  // TODO: Make result screen, pause screen
  while (!WindowShouldClose()) {
    if (message) {
      BeginDrawing();
      DrawText(message, WINDOW_WIDTH / 2 - MeasureText(message, 50) / 2,
               WINDOW_HEIGHT / 2 - 25, 50, RED);
      EndDrawing();
      continue;
    }

    if (playing) {
      if (wait > 0) {
        float frameTime = GetFrameTime();
        wait -= frameTime;
        timePlayed += frameTime;

        if (wait <= 0) {
          if (stop) {
            playing = false;
            stop = false;
            StopMusicStream(music);
          }

          wait = 0;
        }
      } else {
        if (!IsMusicStreamPlaying(music) && !stop) {
          SetMusicVolume(music, 0.15f);
          PlayMusicStream(music);
        }

        UpdateMusicStream(music);
        timePlayed = GetMusicTimePlayed(music);
      }

      if (hitCount >= hits.size() && !stop) {
        wait = 1.5f;
        stop = true;
      }

      int i;
      bool pressedLane[4] = {false};

      for (i = 0; i < notes.size(); i++) {
        Note &note = notes[i];
        if ((timePlayed + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) <
            note.startTime / 1000.0)
          break;
        else if (timePlayed > note.startTime || note.judged ||
                 pressedLane[note.lane - 1])
          continue;

        note.y = WINDOW_HEIGHT - 300 -
                 ((note.startTime / 1000.0 - timePlayed + offset / 1000.0) *
                  SCROLL_SPEED);

        int startTimeDiff = (int)(timePlayed * 1000) - note.startTime - offset;
        if (startTimeDiff > (int)JudgementHitWindow::BAD &&
            startTimeDiff < (int)JudgementHitWindow::MISS && !note.pressed) {
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
          int endTimeDiff = (int)(timePlayed * 1000) - note.endTime - offset;
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

      BeginDrawing();

      ClearBackground(BLACK);

      // FPS
      string fpsText = to_string(GetFPS());
      DrawText(fpsText.c_str(),
               WINDOW_WIDTH - MeasureText(fpsText.c_str(), 40) - 10, 5, 40,
               LIGHTGRAY);

      // Score
      DrawText(to_string(score).c_str(), 10, 10, 55, WHITE);

      // Judgements
      int judges[4] = {(int)Judgement::GREAT, (int)Judgement::GOOD,
                       (int)Judgement::BAD, (int)Judgement::MISS};
      for (i = 0; i < 4; i++)
        DrawText((judgementInfo[(Judgement)judges[i]].first + " " +
                  to_string(judgements[(Judgement)judges[i]]))
                     .c_str(),
                 10, 10 + 60 * (i + 1), 55,
                 judgementInfo[(Judgement)judges[i]].second);

      // Long Notes
      for (i = 0; i < notes.size(); i++) {
        Note note = notes[i];

        if (note.y == -1000)
          break;

        if (!note.judged && note.endTime != -1) {
          DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                            (note.lane - 1) * WINDOW_WIDTH / 8,
                        note.y - (note.endTime - note.startTime) / 1000.0 *
                                     SCROLL_SPEED,
                        WINDOW_WIDTH / 8,
                        (note.endTime - note.startTime) / 1000.0 * SCROLL_SPEED,
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

      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 180, WINDOW_WIDTH / 2,
                    180, Color{180, 180, 180, 255});

      // Previous Judgement
      if (timePlayed >= 0 && timePlayed - judgeUpdated <= 0.6f) {
        int fontSize = 80 + (0.2f - (timePlayed - judgeUpdated)) * 100;
        if (fontSize <= 80)
          fontSize = 80;
        DrawText(
            judgementInfo[prevJudge].first.c_str(),
            WINDOW_WIDTH / 2 -
                MeasureText(judgementInfo[prevJudge].first.c_str(), fontSize) /
                    2,
            WINDOW_HEIGHT / 2, fontSize, judgementInfo[prevJudge].second);
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
                 WINDOW_WIDTH / 2 - MeasureText(text.c_str(), fontSize) / 2,
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

      EndDrawing();
    } else {
      int mouseX = GetMouseX(), mouseY = GetMouseY();

      BeginDrawing();

      ClearBackground(BLACK);

      // Show maps
      int y = 10;
      for (Mapset mapset : mapsets) {
        for (Beatmap map : mapset.maps) {
          string text = map.title.append(" [" + map.diffName + "]");
          if (mouseX >= 10 && mouseY >= y && mouseX <= WINDOW_WIDTH - 10 &&
              mouseY <= y + 50) {
            DrawRectangle(5, y - 5, WINDOW_WIDTH - 15, 55,
                          Color{50, 50, 50, 255});

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
              currentMap = &map;
              music = LoadMusicStream(map.musicPath.c_str());
              notes = map.notes;
              hits.resize(notes.size() + map.longNoteCount);
              playing = true;
            }
          }

          DrawText(text.c_str(), 10, y, 50, WHITE);
          y += 60;
        }
      }

      EndDrawing();
    }
  }

  if (currentMap != NULL)
    UnloadMusicStream(music);

  CloseWindow();
  CloseAudioDevice();

  return 0;
}
