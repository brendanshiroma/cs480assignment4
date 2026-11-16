CXX      = g++
CXXFLAGS = -g3 -Wall -Wextra -std=gnu++17
LDFLAGS  = -pthread

TARGET  = carassembly
SRCS    = main.cpp shared.cpp producer.cpp consumer.cpp finalline.cpp log.cpp
OBJS    = $(SRCS:.cpp=.o)
HDRS    = car_assembly.h log.h shared.h producer.h consumer.h finalline.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -pthread -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
