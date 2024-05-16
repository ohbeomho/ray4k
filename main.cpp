#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500
// px/s
#define SCROLL_SPEED 3000
#define GREAT 50
#define GOOD 100
#define BAD 130
#define MISS 200

using namespace std;
using namespace std::filesystem;

bool startsWith(string str, string prefix) { return str.rfind(prefix) == 0; }

class Note {
public:
  int startTime, endTime, lane;
  bool pressed = false;
  double y;

  Note(int _st, int _et, int _lane) {
    startTime = _st;
    endTime = _et;
    lane = _lane;
    y = -(startTime / 1000.0 * SCROLL_SPEED) + WINDOW_HEIGHT - 400;
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

      reverse(notes.begin(), notes.end());
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
  SetTargetFPS(240);

  // TODO: Load songs from 'songs' folder
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
  float beatTime;
  Beatmap *currentMap = NULL;
  vector<Note> notes, longNotes;
  string prevJudge = "";
  Music music;
  int keys[4] = {KEY_D, KEY_F, KEY_J, KEY_K};

  // TODO: Fix gameplay
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
        // note.y += 1.0 / 240 * SCROLL_SPEED;
        if (timePlayed > note.startTime)
          continue;
        else if ((timePlayed + ((float)WINDOW_HEIGHT / SCROLL_SPEED)) * 1000 <
                 note.startTime)
          continue;

        note.y =
            -((note.startTime - timePlayed * 1000) / 1000.0 * SCROLL_SPEED) +
            WINDOW_HEIGHT - 400;

        if (IsKeyDown(keys[note.lane - 1]) && !pressedLane[note.lane - 1]) {
          int timeDiff = abs(note.startTime - (int)timePlayed * 1000);

          if (timeDiff > MISS) {
            continue;
          }

          pressedLane[note.lane - 1] = true;
          note.pressed = true;
          if (timeDiff < GREAT) {
            prevJudge = "GREAT";
          } else if (timeDiff < GOOD) {
            prevJudge = "GOOD";
          } else if (timeDiff < BAD) {
            prevJudge = "BAD";
          } else if (timeDiff < MISS) {
            prevJudge = "MISS";
          }
        }
      }

      if (timePlayed >= GetMusicTimeLength(music) - beatTime) {
        StopMusicStream(music);
        playing = false;
      }

      BeginDrawing();
      ClearBackground(BLACK);
      DrawText(prevJudge.c_str(), 10, 10, 70, WHITE);
      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 280, WINDOW_WIDTH / 2,
                    280, LIGHTGRAY);
      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 400, WINDOW_WIDTH / 2,
                    120, GRAY);
      for (i = 0; i < notes.size(); i++) {
        Note note = notes[i];
        if (note.y > WINDOW_HEIGHT || note.y < -30 || note.pressed)
          continue;
        if (note.endTime != -1) {
          DrawRectangle(
              WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                  (note.lane - 1) * WINDOW_WIDTH / 8,
              note.y - (note.endTime - note.startTime) / 1000.0 * SCROLL_SPEED,
              WINDOW_WIDTH / 8,
              (note.endTime - note.startTime) / 1000 * SCROLL_SPEED, LIGHTGRAY);
        }

        DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                          (note.lane - 1) * WINDOW_WIDTH / 8,
                      round(note.y), WINDOW_WIDTH / 8, 120, WHITE);
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
