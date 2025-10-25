.SUFFIXES:
.PHONY: all true_all clean
.DELETE_ON_ERROR:

BIN?=bin

all: true_all

TARGETS+=$(BIN)/GeomSandbox.exe

PKGS+=sdl2

HOST:=$(shell $(CXX) -dumpmachine | sed 's/.*-//')

LDFLAGS+=$(shell pkg-config $(PKGS) --libs)
$(BIN)/src/core/main.cpp.o: CXXFLAGS+=$(shell pkg-config $(PKGS) --cflags)

CXXFLAGS+=-Isrc

#CXXFLAGS+=-g3
#LDFLAGS+=-g

CXXFLAGS+=-O3
CXXFLAGS+=-Wall -Wextra -Werror

# Core
SRCS:=\
			src/core/main.cpp\
			src/core/algorithm_app.cpp\
			src/core/algorithm_app2.cpp\
			src/core/geom.cpp\
			src/core/sandbox.cpp\

# Apps
SRCS+=\
			src/app_collide2d.cpp\
			src/app_cube.cpp\
			src/app_polycut.cpp\
			src/app_sat_continuous.cpp\
			src/app_sat_discrete.cpp\
			src/app_subtract.cpp\
			src/collide2d.cpp\
			src/app_example.cpp\
			src/app_bvh_build.cpp\
			src/app_bvh_raycast.cpp\
			src/app_intersect_segment_vs_polyline.cpp\
			src/app_intersect_segments_using_sweepline.cpp\
			src/app_line_distance.cpp\
			src/app_portals_2d.cpp\
			src/app_frustum_clusters.cpp\
			src/app_raycast_aabb.cpp\
			src/app_sound_sources.cpp\
			src/app_split_polyhedron.cpp\
			src/app_split_polygon.cpp\
			src/app_main.cpp\

# Algos
SRCS+=\
			src/app_bresenham.cpp\
			src/app_bsp_build.cpp\
			src/app_bsp_raycast.cpp\
			src/app_contour_tracing.cpp\
			src/app_douglas_peucker.cpp\
			src/app_fastconvexsplit.cpp\
			src/app_polygon_self_intersections.cpp\
			src/app_random_polyhedron.cpp\
			src/app_random_polygon.cpp\
			src/app_spline_catmullrom.cpp\
			src/app_triangulate_bowyerwatson.cpp\
			src/app_triangulate_flip.cpp\
			src/app_triangulate_fast_ear_clipping.cpp\
			src/app_triangulate_simple_ear_clipping.cpp\
			src/app_visvalingam.cpp\
			src/app_voronoi_fortune.cpp\
			src/pathfind_astar.cpp\
			src/pathfind_dijkstra.cpp\
			src/random_polyhedron.cpp\
			src/random_polygon.cpp\
			src/split_polygon.cpp\
			src/split_polyhedron.cpp\
			src/triangulate_bowyerwatson.cpp\
			src/triangulate_flip.cpp\

# Common stuff
SRCS+=\
			src/random.cpp\
			src/bvh.cpp\
			src/bsp.cpp\
			src/core/serialization.cpp\

# Platform glue
SRCS+=\
			src/platform/glad.cpp\
			src/platform/fiber_$(HOST).cpp\

$(BIN)/GeomSandbox.exe: $(SRCS:%=$(BIN)/%.o)

#------------------------------------------------------------------------------

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) -MMD -MT "$@" -MF "$@.dep" -o "$@" $<

include $(shell test -d $(BIN) && find $(BIN) -name "*.dep")

clean:
	rm -rf $(BIN)

true_all: $(TARGETS)
