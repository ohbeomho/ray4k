#include "judge.h"
#include "map.h"
#include <cmath>
#include <cstdlib>
#include <map>
#include <raylib.h>
#include <string>

// Window size
#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500

// px/s
#define SCROLL_SPEED 2800

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray4K");
  InitAudioDevice();
  SetTargetFPS(500);

  // Load maps
  vector<Mapset> mapsets = loadMaps(path("./songs"));

  bool playing = false;
  float timePlayed = 0.0f, judgeUpdated = 0.0f, diffUpdated = 0.0f, beatTime;
  Beatmap *currentMap = NULL;
  vector<Note> notes, longNotes;
  Music music;

  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};
  // Only GOOD, BAD
  int prevHitDiff = 0;

  map<Judgement, int> judgements;
  map<Judgement, pair<string, Color>> judgementInfo;
  judgementInfo[Judgement::GREAT] = make_pair("GREAT", SKYBLUE);
  judgementInfo[Judgement::GOOD] = make_pair("GOOD", GREEN);
  judgementInfo[Judgement::BAD] = make_pair("BAD", DARKGRAY);
  judgementInfo[Judgement::MISS] = make_pair("MISS", RED);

  Judgement prevJudge = Judgement::GHOST;
  int score = 0;

  auto judge = [&prevJudge, &prevHitDiff, &judgements, &judgeUpdated, &score,
                &timePlayed, &judgementInfo, &diffUpdated](int hitDiff) {
    int absHitDiff = abs(hitDiff);
    Judgement judge = calcScore(absHitDiff);

    if (judge != Judgement::GHOST) {
      score += (int)prevJudge;
      judgements[prevJudge]++;

      prevJudge = judge;
      judgeUpdated = timePlayed;

      if (prevJudge != Judgement::GREAT && prevJudge != Judgement::MISS) {
        prevHitDiff = hitDiff;
        diffUpdated = timePlayed;
      }
    }

    return judge;
  };

  // TODO: Make result screen
  while (!WindowShouldClose()) {
    if (playing && !IsMusicStreamPlaying(music) && IsMusicReady(music)) {
      SetMusicVolume(music, 0.15f);
      PlayMusicStream(music);
    } else if (playing) {
      UpdateMusicStream(music);

      timePlayed = GetMusicTimePlayed(music);
      int i;
      bool pressedLane[4] = {false};

      for (i = 0; i < notes.size(); i++) {
        Note &note = notes[i];
        if ((timePlayed * 1000 + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) * 1000 <
            note.startTime)
          break;
        else if (timePlayed > note.startTime || note.judged ||
                 pressedLane[note.lane - 1])
          continue;

        note.y =
            -((note.startTime - timePlayed * 1000) / 1000.0 * SCROLL_SPEED) +
            WINDOW_HEIGHT - 300;

        int startTimeDiff = (int)(timePlayed * 1000) - note.startTime;
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
          int endTimeDiff = (int)(timePlayed * 1000) - note.endTime;
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

      if (timePlayed >= GetMusicTimeLength(music) - beatTime) {
        StopMusicStream(music);
        playing = false;
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
                    180, LIGHTGRAY);

      // Previous Judgement
      if (timePlayed - judgeUpdated <= 0.6f) {
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

      // Early/Late
      if (timePlayed - diffUpdated <= 0.6f) {
        int fontSize = 60 + (0.2f - (timePlayed - diffUpdated)) * 100;
        if (fontSize <= 60)
          fontSize = 60;
        string text = (prevHitDiff > 0 ? "LATE " : "EARLY ") +
                      to_string(abs(prevHitDiff)) + "ms";
        DrawText(text.c_str(),
                 WINDOW_WIDTH / 2 - MeasureText(text.c_str(), fontSize) / 2,
                 WINDOW_HEIGHT / 2 - fontSize - 10, fontSize,
                 prevHitDiff > 0 ? Color{150, 20, 20, 255} : LIME);
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
              playing = true;
              currentMap = &map;
              music = LoadMusicStream(map.musicPath.c_str());
              beatTime = 60.0f / map.bpm;
              notes = map.notes;
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
