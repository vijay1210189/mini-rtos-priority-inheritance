CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
SRCS := src/main.cpp src/Task.cpp src/Scheduler.cpp src/PriorityInheritanceMutex.cpp
TARGET := rtos_demo

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
