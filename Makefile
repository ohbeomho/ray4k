CXX=g++
INC=-IC:/raylib/include
LIBS=-LC:/raylib/lib
CXXFLAGS=-lraylib -lgdi32 -lwinmm -mwindows -std=c++20

ray4k.exe: main.o map.o judge.o button.o screen.o
	$(CXX) -g *.o -o ray4k.exe $(INC) $(LIBS) $(CXXFLAGS)

clean:
	del *.o
