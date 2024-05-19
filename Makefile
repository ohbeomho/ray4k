CXX=g++
INC=-IC:/raylib/include
LIBS=-LC:/raylib/lib
CXXFLAGS=-lraylib -lgdi32 -lwinmm -mwindows

ray4k.exe: main.o map.o judge.o
	$(CXX) -g main.o map.o judge.o -o ray4k.exe $(INC) $(LIBS) $(CXXFLAGS)

main.o:
	$(CXX) -c main.cpp $(INC) $(LIBS)

map.o:
	$(CXX) -c map.cpp $(INC) $(LIBS)

judge.o:
	$(CXX) -c judge.cpp $(INC) $(LIBS)

clean:
	rm *.o
