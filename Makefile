.SUFFIXES:
.PHONY: all true_all clean
.DELETE_ON_ERROR:

BIN?=bin

all: true_all

TARGETS+=$(BIN)/GeomSandbox.exe

PKGS+=sdl2

HOST:=$(shell $(CXX) -dumpmachine | sed 's/.*-//')

CXXFLAGS+=$(shell pkg-config $(PKGS) --cflags)
LDFLAGS+=$(shell pkg-config $(PKGS) --libs)

CXXFLAGS+=-g3

SRCS:=\
			src/main.cpp\
			src/collide2d.cpp\
			src/visualizer.cpp\
			src/random.cpp\
			src/fiber_$(HOST).cpp\

SRCS+=\
			src/app_example.cpp\
			src/app_collide2d.cpp\
			src/app_triangulate2.cpp\
			src/app_triangulate_bowyerwatson.cpp\
			src/app_triangulate_flip.cpp\
			src/triangulate_bowyerwatson.cpp\
			src/triangulate_flip.cpp\

$(BIN)/GeomSandbox.exe: $(SRCS:%=$(BIN)/%.o)

#------------------------------------------------------------------------------

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) -o "$@" $<
	@$(CXX) -MM -MT "$@" -c $(CXXFLAGS) -o "$@.dep" $<

include $(shell test -d $(BIN) && find $(BIN) -name "*.dep")

clean:
	rm -rf $(BIN)

true_all: $(TARGETS)
