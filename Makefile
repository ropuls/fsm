#CXX ?= clang++-7
CXX = g++-7
CXXFLAGS = -std=c++17 -g -Wall -I. -I./include/ -DASIO_STANDALONE -MD
LDFLAGS = -pthread

all: fsm
clean:
	rm -f *.o
	rm -f fsm

OBJS = fsm.o

fsm: $(OBJS)
	$(CXX) -o fsm $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(OBJS:.o=.d)
