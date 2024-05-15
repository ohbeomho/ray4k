#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

using namespace std;
using namespace std::filesystem;

bool startsWith(string str, string prefix) { return str.rfind(prefix) == 0; }

class HitObject {
public:
  int startTime, endTime;
  int lane;
  int y = -30;

  HitObject(int _st, int _et, int _lane) {
    startTime = _st;
    endTime = _et;
    lane = _lane;
  }
};

class Beatmap {
public:
  int startOffset, bpm;
  string title, artist, creator, diffName, musicPath;
  vector<HitObject> hitObjects;

  Beatmap(path filePath) {
    ifstream file(filePath.string().data());
    bool isHitObjects = false;

    if (file.is_open()) {
      string line;
      int c = -1;

      while (getline(file, line)) {
        if (isHitObjects) {
          if (startsWith(line, "-")) {
            hitObjects.push_back(HitObject(stoi(line.substr(13)), -1, -1));
            c++;
          } else if (startsWith(line, "  EndTime: "))
            hitObjects[c].endTime = stoi(line.substr(11));
          else if (startsWith(line, "  Lane: "))
            hitObjects[c].lane = stoi(line.substr(8));
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
            isHitObjects = true;
        }
      }

      reverse(hitObjects.begin(), hitObjects.end());
    }
  }
};

class Mapset {
public:
  vector<Beatmap> maps;
};

#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 1500
// px/s
#define SCROLL_SPEED 3000

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
  vector<HitObject> hitObjects;
  vector<HitObject> visibleObjects;
  Music music;

  // TODO: Implement actual gameplay
  while (!WindowShouldClose()) {
    if (playing && !IsMusicStreamPlaying(music) && IsMusicReady(music)) {
      SetMusicVolume(music, 0.15f);
      PlayMusicStream(music);
    } else if (playing) {
      UpdateMusicStream(music);

      timePlayed = GetMusicTimePlayed(music);

      int i;
      for (i = hitObjects.size() - 1; i >= 0; i--) {
        if (hitObjects[i].startTime <=
            timePlayed * 1000 +
                ((float)(WINDOW_HEIGHT - 520) / SCROLL_SPEED) * 1000) {
          visibleObjects.push_back(hitObjects[i]);
          hitObjects.pop_back();
        }
      }

      for (HitObject &obj : visibleObjects) {
        obj.y += 1.0f / 240 * SCROLL_SPEED;
      }

      if (timePlayed >= GetMusicTimeLength(music) - beatTime) {
        StopMusicStream(music);
        playing = false;
      }

      BeginDrawing();
      ClearBackground(BLACK);
      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 280, WINDOW_WIDTH / 2,
                    280, LIGHTGRAY);
      DrawRectangle(WINDOW_WIDTH / 4, WINDOW_HEIGHT - 400, WINDOW_WIDTH / 2,
                    120, GRAY);
      for (i = 0; i < visibleObjects.size(); i++) {
        HitObject obj = visibleObjects[i];
        if (obj.endTime != -1) {
          DrawRectangle(
              WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                  (obj.lane - 1) * WINDOW_WIDTH / 8,
              obj.y - (obj.endTime - obj.startTime) / 1000 * SCROLL_SPEED,
              WINDOW_WIDTH / 8,
              (obj.endTime - obj.startTime) / 1000 * SCROLL_SPEED, LIGHTGRAY);
        }

        DrawRectangle(WINDOW_WIDTH / 2 - WINDOW_WIDTH / 4 +
                          (obj.lane - 1) * WINDOW_WIDTH / 8,
                      obj.y, WINDOW_WIDTH / 8, 120, WHITE);
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
              hitObjects = map.hitObjects;
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
