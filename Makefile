CXX=g++
INC=-IC:/raylib/include
LIBS=-LC:/raylib/lib
CXXFLAGS=-lraylib -lgdi32 -lwinmm -mwindows

ray4k.exe: main.o map.o judge.o button.o
	$(CXX) -g main.o map.o judge.o button.o -o ray4k.exe $(INC) $(LIBS) $(CXXFLAGS)

clean:
	del *.o
