#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

// Window size
#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500

// px/s
#define SCROLL_SPEED 3000

// Hit window
// ms
#define GREAT 55
#define GOOD 85
#define BAD 130
#define MISS 200

using namespace std;
using namespace std::filesystem;

bool startsWith(string str, string prefix) { return str.rfind(prefix) == 0; }

class Note {
public:
  int startTime, endTime, lane;
  bool judged = false, pressed = false;
  double y;

  Note(int _st, int _et, int _lane) {
    startTime = _st;
    endTime = _et;
    lane = _lane;
    y = -1000;
  }
};

class Beatmap {
public:
  int startOffset, bpm;
  string title, artist, creator, diffName, musicPath;
  vector<Note> notes;

  Beatmap(path filePath) {
    ifstream file(filePath.string().data());
    bool isNotes = false;

    if (file.is_open()) {
      string line;
      int c = -1;

      while (getline(file, line)) {
        if (isNotes) {
          if (startsWith(line, "-")) {
            notes.push_back(Note(stoi(line.substr(13)), -1, -1));
            c++;
          } else if (startsWith(line, "  EndTime: "))
            notes[c].endTime = stoi(line.substr(11));
          else if (startsWith(line, "  Lane: "))
            notes[c].lane = stoi(line.substr(8));
        } else {
          if (startsWith(line, "Mode: ") && line.substr(6) != "Keys4")
            throw "Ray4k only supports 4k maps.";

          if (startsWith(line, "AudioFile: "))
            musicPath =
                filePath.parent_path().concat("\\" + line.substr(11)).string();
          else if (startsWith(line, "Title: "))
            title = line.substr(7);
          else if (startsWith(line, "Artist: "))
            artist = line.substr(8);
          else if (startsWith(line, "Creator: "))
            creator = line.substr(9);
          else if (startsWith(line, "DifficultyName: "))
            diffName = line.substr(16);
          else if (startsWith(line, "  Bpm: "))
            bpm = stoi(line.substr(7));
          else if (line == "HitObjects:")
            isNotes = true;
        }
      }
    }
  }
};

class Mapset {
public:
  vector<Beatmap> maps;
};

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray4K");
  InitAudioDevice();
  SetTargetFPS(500);

  path songsPath("./songs");
  vector<Mapset> mapsets;

  for (const auto &entry : directory_iterator(songsPath)) {
    if (entry.is_directory()) {
      Mapset mapset;

      for (const auto &e : directory_iterator(entry)) {
        if (e.is_regular_file() && e.path().extension() == ".qua") {
          try {
            mapset.maps.push_back(Beatmap(e.path()));
          } catch (string message) {
            cerr << message;
            return 1;
          }
        }
      }

      mapsets.push_back(mapset);
    }
  }

  bool playing = false;
  float timePlayed = 0.0f;
  float judgeUpdated = 0.0f;
  float beatTime;
  Beatmap *currentMap = NULL;
  vector<Note> notes, longNotes;
  Music music;
  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};
  int judgements[4] = {0};
  int prevJudge = -1;
  string judgeTexts[4] = {"GREAT", "GOOD", "BAD", "MISS"};
  Color judgeColors[4] = {SKYBLUE, GREEN, DARKGRAY, RED};

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

      if (timePlayed - judgeUpdated > 0.5f)
        prevJudge = -1;

      for (i = 0; i < notes.size(); i++) {
        Note &note = notes[i];
        if ((timePlayed * 1000 + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) * 1000 <
            note.startTime)
          break;
        else if (timePlayed > note.startTime || note.judged)
          continue;

        note.y =
            -((note.startTime - timePlayed * 1000) / 1000.0 * SCROLL_SPEED) +
            WINDOW_HEIGHT - 300;

        int startTimeDiff = note.startTime - (int)(timePlayed * 1000);
        if (startTimeDiff < -BAD && startTimeDiff >= -MISS && !note.pressed) {
          judgements[3]++;
          judgeUpdated = timePlayed;
          if (note.endTime == -1)
            note.judged = true;
        }

        if (IsKeyPressed(keys[note.lane - 1]) && !pressedLane[note.lane - 1]) {
          startTimeDiff = abs(startTimeDiff);
          if (startTimeDiff > MISS)
            continue;

          pressedLane[note.lane - 1] = true;
          if (note.endTime == -1)
            note.judged = true;
          else
            note.pressed = true;

          if (startTimeDiff <= GREAT)
            prevJudge = 0;
          else if (startTimeDiff <= GOOD)
            prevJudge = 1;
          else if (startTimeDiff <= BAD)
            prevJudge = 2;
          else if (startTimeDiff < MISS)
            prevJudge = 3;

          judgements[prevJudge]++;
          judgeUpdated = timePlayed;
        }

        if (note.endTime != -1 && note.pressed) {
          int endTimeDiff = note.endTime - (int)(timePlayed * 1000);
          if (endTimeDiff < -BAD && endTimeDiff >= -MISS &&
              IsKeyDown(keys[note.lane - 1])) {
            judgements[3]++;
            judgeUpdated = timePlayed;
            note.judged = true;
          }

          if (IsKeyReleased(keys[note.lane - 1])) {
            if (endTimeDiff > MISS) {
              judgements[3]++;
              judgeUpdated = timePlayed;
              note.judged = true;
              continue;
            }

            endTimeDiff = abs(endTimeDiff);

            pressedLane[note.lane - 1] = true;
            note.judged = true;

            if (endTimeDiff <= GREAT)
              prevJudge = 0;
            else if (endTimeDiff <= GOOD)
              prevJudge = 1;
            else if (endTimeDiff <= BAD)
              prevJudge = 2;
            else if (endTimeDiff < MISS)
              prevJudge = 3;

            judgements[prevJudge]++;
            judgeUpdated = timePlayed;
          }
        }
      }

      if (timePlayed >= GetMusicTimeLength(music) - beatTime) {
        StopMusicStream(music);
        playing = false;
      }

      int fps = GetFPS();

      BeginDrawing();

      ClearBackground(BLACK);

      // FPS
      DrawText(to_string(fps).c_str(), WINDOW_WIDTH - 100, 5, 40, LIGHTGRAY);

      // Judgements
      for (i = 0; i < 4; i++)
        DrawText((judgeTexts[i] + " " + to_string(judgements[i])).c_str(), 10,
                 10 + 60 * i, 55, judgeColors[i]);

      // Notes
      for (i = 0; i < notes.size(); i++) {
        Note note = notes[i];

        // Long Notes
        if (!note.judged && note.endTime != -1) {
          DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                            (note.lane - 1) * WINDOW_WIDTH / 8,
                        note.y - (note.endTime - note.startTime) / 1000.0 *
                                     SCROLL_SPEED,
                        WINDOW_WIDTH / 8,
                        (note.endTime - note.startTime) / 1000.0 * SCROLL_SPEED,
                        LIGHTGRAY);
        }

        if (note.y > WINDOW_HEIGHT || note.judged)
          continue;
        else if (note.y == -1000)
          break;

        DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                          (note.lane - 1) * WINDOW_WIDTH / 8,
                      note.y, WINDOW_WIDTH / 8, 120, WHITE);
      }

      for (i = 0; i < 4; i++)
        DrawRectangle(WINDOW_WIDTH / 4 + (WINDOW_WIDTH / 8) * i,
                      WINDOW_HEIGHT - 300, WINDOW_WIDTH / 8, 120,
                      IsKeyDown(keys[i]) ? YELLOW : GRAY);
      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 180, WINDOW_WIDTH / 2,
                    180, LIGHTGRAY);

      // Previous Judgement
      if (prevJudge != -1) {
        DrawText(judgeTexts[prevJudge].c_str(),
                 WINDOW_WIDTH / 2 -
                     MeasureText(judgeTexts[prevJudge].c_str(), 80) / 2,
                 WINDOW_HEIGHT / 2, 80, judgeColors[prevJudge]);
      }

      EndDrawing();
    } else {
      int mouseX = GetMouseX(), mouseY = GetMouseY();

      BeginDrawing();
      ClearBackground(BLACK);
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
