#include <sstream>
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "geom.h"

template<>
std::vector<Vec2> deserialize<std::vector<Vec2>>(span<const uint8_t> data)
{
  std::vector<Vec2> r;

  std::istringstream ss(std::string((const char*)data.ptr, data.len));
  std::string line;
  while(std::getline(ss, line))
  {
    std::istringstream lineSS(line);
    Vec2 v;
    char comma;
    if(lineSS >> v.x >> comma >> v.y)
      r.push_back(v);
  }

  if(r.back() == r.front())
    r.pop_back();

  // recenter polygon
  {
    BoundingBox box;
    for(auto& v : r)
      box.add(v);

    Vec2 translate = -(box.min + box.max) * 0.5;
    Vec2 scale;
    scale.x = 30.0 / (box.max.x - box.min.x);
    scale.y = 30.0 / (box.max.y - box.min.y);

    scale.x = scale.y = std::min(scale.x, scale.y);

    for(auto& v : r)
    {
      v = (v + translate);
      v.x *= scale.x;
      v.y *= scale.y;
    }
  }

  return r;
}
