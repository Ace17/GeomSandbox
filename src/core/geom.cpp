#include "geom.h"

#include <cmath>

float magnitude(Vec2 v) { return sqrt(v * v); };
double magnitude(Vec3 v) { return sqrt(dotProduct(v, v)); };
