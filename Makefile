CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
INCLUDES = -I/usr/include/GL
LIBS = -lGL -lGLEW -lglfw -lm

SOURCES = main.cpp model.cpp shape.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = modeler

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS += -DDEBUG -O0
debug: $(TARGET)

release: CXXFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)
