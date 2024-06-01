#ifndef RAY4K_MAP_H_
#define RAY4K_MAP_H_

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace std::filesystem;

class Note {
public:
  int startTime, endTime, lane;
  double y;
  bool judged = false;
  // For long notes
  bool pressed = false;

  Note();
};

class Beatmap {
public:
  string title, artist, creator, diffName, musicPath;
  path filePath;
  int bpm, longNoteCount = 0;

  pair<vector<Note>, int> loadNotes();

  Beatmap(path _filePath);
};

class Mapset {
public:
  vector<Beatmap> maps;
};

vector<Mapset> loadMaps(path songsPath);

#endif
