#include "map.hpp"
#include <fstream>
#include <utility>
#include <vector>

using namespace std;
using namespace std::filesystem;

bool startsWith(string str, string prefix) { return str.rfind(prefix) == 0; }

Note::Note() { y = -1000; }

Beatmap::Beatmap(path _filePath) {
  filePath = _filePath;

  ifstream file(filePath.string().data());
  if (file.is_open()) {
    string line;

    while (getline(file, line, '\n')) {
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
        break;
    }
  }
}

pair<vector<Note>, int> Beatmap::loadNotes() {
  ifstream file(filePath.string().data());
  vector<Note> notes;
  bool isNotes = false;
  int longNoteCount = 0;

  if (file.is_open()) {
    string line;
    int c = -1;

    while (getline(file, line, '\n')) {
      if (line == "HitObjects:")
        isNotes = true;

      if (isNotes) {
        if (startsWith(line, "- StartTime: ")) {
          notes.push_back(Note());
          c++;
          notes[c].startTime = stoi(line.substr(13));
          notes[c].endTime = -1;
        } else if (startsWith(line, "- Lane: ")) {
          notes.push_back(Note());
          c++;
          notes[c].startTime = 1;
          notes[c].endTime = -1;
          notes[c].lane = stoi(line.substr(8));
        } else if (startsWith(line, "  EndTime: ")) {
          notes[c].endTime = stoi(line.substr(11));
          longNoteCount++;
        } else if (startsWith(line, "  Lane: "))
          notes[c].lane = stoi(line.substr(8));
      }
    }
  }

  return make_pair(notes, longNoteCount);
}

vector<Mapset> loadMaps(path songsPath) {
  vector<Mapset> mapsets;

  for (const auto &entry : directory_iterator(songsPath)) {
    if (entry.is_directory()) {
      Mapset mapset;

      for (const auto &e : directory_iterator(entry)) {
        if (e.is_regular_file() && e.path().extension() == ".qua")
          mapset.maps.push_back(Beatmap(e.path()));
      }

      mapsets.push_back(mapset);
    }
  }

  return mapsets;
}
