#include "geom.h"

#include <cmath>

float magnitude(Vec2 v) { return sqrt(dotProduct(v, v)); };
float magnitude(Vec3 v) { return sqrt(dotProduct(v, v)); };
