#include <filesystem>
#include <fstream>
#include <raylib.h>
#include <vector>

using namespace std;
using namespace std::filesystem;

class HitObject {
public:
  long startTime, endTime;
};

class Map {
public:
  long startOffset;
  vector<HitObject> hitObjects;

  Map(path filePath) {
    ifstream file(filePath.string().data());
    // TODO: Read file and process map data
    if (file.is_open()) {
    }
  }
};

class Mapset {
public:
  string title, artist;
  vector<Map> maps;
};

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray4K");
  SetTargetFPS(240);

  // TODO: Load songs from 'songs' folder
  path songsPath("./songs");
  vector<Mapset> songs;

  for (const auto &entry : directory_iterator(songsPath)) {
    if (entry.is_directory()) {
      Mapset mapset;

      for (const auto &e : directory_iterator(entry)) {
        if (e.is_regular_file() && e.path().extension() == ".qua") {
        }
      }
    }
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
