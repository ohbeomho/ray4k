CXX=g++
INC=-IC:/raylib/include
LIBS=-LC:/raylib/lib
CXXFLAGS=-std=c++20 -lraylib -lgdi32 -lwinmm -mwindows
TARGET=ray4k.exe
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

all: $(TARGET)
	$(CXX) -o $(TARGET) $(OBJS) $(INC) $(LIBS) $(CXXFLAGS)

$(TARGET):
	$(CXX) -c $(SRCS) $(INC) $(LIBS) $(CXXFLAGS)

clean:
	del *.o
