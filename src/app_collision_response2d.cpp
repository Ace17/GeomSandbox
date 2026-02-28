// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Collision detection and response

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "random.h"

namespace
{
Vec2 rotateRight(Vec2 v) { return Vec2(v.y, -v.x); }
Vec2 rotate(Vec2 v, float angle)
{
  Vec2 r;
  r.x = cos(angle) * v.x - sin(angle) * v.y;
  r.y = sin(angle) * v.x + cos(angle) * v.y;
  return r;
}

float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

float sqr(float x) { return x * x; }

struct Face
{
  Vec2 points[2]; // local ref
  Vec2 normal;
};

struct ConvexPolygon
{
  std::vector<Face> faces;
  Vec2 pos; // world ref
  Vec2 vel; // world ref
  float angle = 0;
  float omega = 0;
  float invMass = 1;
  float invMoi = 1; // inverse moment of inertia
};

void addFace(ConvexPolygon& polygon, Vec2 a, Vec2 b)
{
  Face f;
  f.points[0] = a;
  f.points[1] = b;
  f.normal = rotateRight(normalize(b - a));
  polygon.faces.push_back(f);
}

ConvexPolygon createSquare()
{
  const Vec2 a = {-1.5, -1.5};
  const Vec2 b = {+1.5, -1.5};
  const Vec2 c = {+1.5, +1.5};
  const Vec2 d = {-1.5, +1.5};

  ConvexPolygon r{};

  addFace(r, a, b);
  addFace(r, b, c);
  addFace(r, c, d);
  addFace(r, d, a);

  return r;
}

ConvexPolygon randomConvexPolygon()
{
  ConvexPolygon r;

  const int N = randomInt(3, 12);
  const float radiusX = randomFloat(2, 5);
  const float radiusY = randomFloat(2, 5);
  const float phase = randomFloat(0, 2 * M_PI);
  for(int i = 0; i < N; ++i)
  {
    Vec2 a;
    a.x = cos((i + 0) * M_PI * 2 / N + phase) * radiusX;
    a.y = sin((i + 0) * M_PI * 2 / N + phase) * radiusY;

    Vec2 b;
    b.x = cos((i + 1) * M_PI * 2 / N + phase) * radiusX;
    b.y = sin((i + 1) * M_PI * 2 / N + phase) * radiusY;

    addFace(r, a, b);
  }

  r.pos = randomPos({-10, -5}, {+10, 5});

  return r;
}

struct Collision
{
  float depth;
  Vec2 normal;
  Vec2 contactPoint;
};

struct Interval
{
  float min, max;
};

Interval projectPolygon(const ConvexPolygon& polygon, Vec2 axis)
{
  Interval r;
  r.max = r.min = dotProduct(polygon.pos + rotate(polygon.faces[0].points[0], polygon.angle), axis);
  for(auto& face : polygon.faces)
  {
    for(auto& v : face.points)
    {
      const float projV = dotProduct(polygon.pos + rotate(v, polygon.angle), axis);
      r.min = std::min(r.min, projV);
      r.max = std::max(r.max, projV);
    }
  }
  return r;
}

Vec2 findContactPoint(const ConvexPolygon& a, const ConvexPolygon& b, Vec2 normal)
{
  const ConvexPolygon* referent = &a;
  const ConvexPolygon* incident = &b;

  float best = 0;
  float sign = 1;

  // find the face most parallel to the normal: this is the "reference face"
  for(auto& face : a.faces)
  {
    const float value = dotProduct(rotate(face.normal, a.angle), normal);
    if(value > best)
    {
      best = value;
    }
  }

  for(auto& face : b.faces)
  {
    const float value = dotProduct(rotate(face.normal, b.angle), -normal);
    if(value > best)
    {
      best = value;
      sign = -1;
      referent = &b;
      incident = &a;
    }
  }

  // TODO: clip the incident face to the referent face
  (void)referent;

  Vec2 pos;
  // find the deepest point of 'incident' which is inside 'referent'.
  float maxDepth = -1.0 / 0.0;
  for(auto& face : incident->faces)
  {
    for(auto& v : face.points)
    {
      float depth = dotProduct(-normal, incident->pos + rotate(v, incident->angle)) * sign;
      if(depth > maxDepth)
      {
        maxDepth = depth;
        pos = incident->pos + rotate(v, incident->angle);
      }
    }
  }

  return pos;
}

Collision collidePolygons(const ConvexPolygon& a, const ConvexPolygon& b)
{
  Collision r;

  r.depth = 1.0 / 0.0;

  std::vector<Vec2> axisToTest;

  for(auto& face : a.faces)
    axisToTest.push_back(rotate(face.normal, a.angle));

  for(auto& face : b.faces)
    axisToTest.push_back(rotate(face.normal, b.angle));

  for(auto axis : axisToTest)
  {
    const Interval projA = projectPolygon(a, axis);
    const Interval projB = projectPolygon(b, axis);

    Collision c{};

    const auto middleA = (projA.min + projA.max) * 0.5f;
    const auto middleB = (projB.min + projB.max) * 0.5f;

    if(middleA < middleB)
    {
      c.depth = projA.max - projB.min;
      c.normal = axis;
    }
    else
    {
      c.depth = projB.max - projA.min;
      c.normal = -axis;
    }

    if(c.depth < r.depth)
      r = c;
  }

  r.contactPoint = findContactPoint(a, b, r.normal);
  return r;
}

struct CollisionResponse2d : IApp
{
  CollisionResponse2d()
  {
    colliders[0] = createSquare();
    colliders[1] = createSquare();
    colliders[0] = randomConvexPolygon();
    colliders[1] = randomConvexPolygon();

    colliders[0].pos = {-4, 0};
    colliders[1].pos = {+4, 0};
    colliders[0].angle = 0.7;
    colliders[1].angle = -1.3;
    colliders[0].vel = {1, 1};
    colliders[1].vel = {-1, 1};
    colliders[0].omega = 0 * +0.3;
    colliders[1].omega = 0 * -0.5;
    colliders[0].invMass = 2.0;
    colliders[1].invMass = 1.0;
    colliders[0].invMoi = 1.0;
    colliders[1].invMoi = 1.0;

    compute();
  }

  static inline const float velMag = 32;

  void
  drawLinearMomentum(const ConvexPolygon& bodyA, const ConvexPolygon& bodyB, Color color, IDrawer* drawer, float offset)
  {
    {
      Vec2 totalLinearMomentum = {};
      Vec2 displayPos = {-768.0f, 0 + offset * 64};
      totalLinearMomentum += bodyA.vel / bodyA.invMass;
      totalLinearMomentum += bodyB.vel / bodyB.invMass;
      drawer->text({}, "total linear momentum", color, displayPos);
      drawer->line(Vec2{}, Vec2{}, color, displayPos, displayPos + totalLinearMomentum * velMag);
    }
    {
      float totalAngularMomentum = {};
      Vec2 displayPos = {-768.0f, -32 + offset * 64};
      totalAngularMomentum += cross(bodyA.pos, bodyA.vel / bodyA.invMass) + bodyA.omega * bodyA.invMoi;
      totalAngularMomentum += cross(bodyB.pos, bodyB.vel / bodyB.invMass) + bodyB.omega * bodyB.invMoi;
      char buf[256];
      sprintf(buf, "total angular_momentum: %.3f", totalAngularMomentum);
      drawer->text({}, buf, color, displayPos);
    }
  }

  void draw(IDrawer* drawer) override
  {
    const auto c = (collision.depth >= 0) ? Gray : White;

    const auto& bodyA = colliders[0];
    const auto& bodyB = colliders[1];

    drawBody(drawer, bodyA, c, "A");
    drawBody(drawer, bodyB, c, "B");

    drawLinearMomentum(bodyA, bodyB, White, drawer, 0);

    if(collision.depth >= 0)
    {
      const auto contactPoint = collision.contactPoint;
      drawer->line(contactPoint, contactPoint + collision.depth * collision.normal, Red);

      drawer->line(contactPoint, contactPoint, Red, {-5, 0}, {+5, 0});
      drawer->line(contactPoint, contactPoint, Red, {0, -5}, {0, +5});

      // draw velocities at contact point
      {
        const auto ra = contactPoint - colliders[0].pos;
        const auto rb = contactPoint - colliders[1].pos;
        drawer->line(contactPoint, contactPoint, Yellow, {0, 0}, (bodyA.vel + bodyA.omega * rotateLeft(ra)) * velMag);
        drawer->line(
              contactPoint, contactPoint, LightBlue, {0, 0}, (bodyB.vel + bodyB.omega * rotateLeft(rb)) * velMag);
      }

      {
        const auto& bodyA = collidersAfter[0];
        const auto& bodyB = collidersAfter[1];

        drawBody(drawer, collidersAfter[0], Green, "A");
        drawBody(drawer, collidersAfter[1], Green, "B");

        drawer->line(collidersAfter[0].pos, collidersAfter[0].pos, Yellow, {0, 0}, collidersAfter[0].vel * velMag);
        drawer->line(collidersAfter[1].pos, collidersAfter[1].pos, Yellow, {0, 0}, collidersAfter[1].vel * velMag);

        // draw velocities at contact point
        const auto ra = contactPoint - collidersAfter[0].pos;
        const auto rb = contactPoint - collidersAfter[1].pos;
        drawer->line(contactPoint, contactPoint, Green, {0, 0}, (bodyA.vel + bodyA.omega * rotateLeft(ra)) * velMag);
        drawer->line(contactPoint, contactPoint, Green, {0, 0}, (bodyB.vel + bodyB.omega * rotateLeft(rb)) * velMag);
      }

      drawLinearMomentum(collidersAfter[0], collidersAfter[1], Green, drawer, 1);
    }
  }

  void drawBody(IDrawer* drawer, const ConvexPolygon& body, Color color, const char* name)
  {
    drawCross(drawer, body.pos, color);
    drawer->text(body.pos + Vec2{.7, .7}, name, color);

    for(auto& face : body.faces)
    {
      auto v0 = rotate(face.points[0], body.angle);
      auto v1 = rotate(face.points[1], body.angle);
      drawer->line(body.pos + v0, body.pos + v1, color);
    }

    // draw velocity of center of mass
    drawer->line(body.pos, body.pos, color, {0, 0}, body.vel * velMag);

    {
      // draw velocity of two points
      for(int i = 0; i < 2; ++i)
      {
        const int N = body.faces.size();
        auto p = body.pos + rotate(body.faces[i * (N / 2)].points[0], body.angle);
        const auto ra = p - body.pos;
        drawer->line(p, p, color, {0, 0}, (body.vel + body.omega * rotateLeft(ra)) * velMag);
      }
    }
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{0.5, 0}, pos + Vec2{0.5, 0}, color);
    drawer->line(pos - Vec2{0, 0.5}, pos + Vec2{0, 0.5}, color);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    auto& collider = colliders[(int)selection];
    switch(key)
    {
    case Key::Left:
      collider.pos.x -= 0.3;
      break;
    case Key::Right:
      collider.pos.x += 0.3;
      break;
    case Key::Up:
      collider.pos.y += 0.3;
      break;
    case Key::Down:
      collider.pos.y -= 0.3;
      break;
    case Key::Space:
      selection = !selection;
      break;
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    // Apply the impulse: j * N
    //
    // (conservation of momentum)
    // vA' = vA + j/mA * N
    // vB' = vB - j/mB * N
    // wA' = wA + IA^-1 ( ra x jN )
    // wB' = wB - IB^-1 ( rb x jN )
    //
    // After the impulse has been applied, we want:
    //
    // (velocity restitution)
    // (vA' - vB').N = -e * (vA - vB).N
    //
    // Solving for j:
    //
    //        (1+e)(vB - vA).N
    // j = ----------------------------------------------------------
    //       1     1    [                                                 ]
    //      --- + --- + [ IA^-1 ((ra x N ) x ra) + IB^-1 ((rb x N ) x rb) ].N
    //       mA    mB   [                                                 ]
    //

    auto& bodyA = colliders[0];
    auto& bodyB = colliders[1];

    collidersAfter[0] = bodyA;
    collidersAfter[1] = bodyB;

    collision = collidePolygons(bodyA, bodyB);

    const auto vA = colliders[0].vel;
    const auto vB = colliders[1].vel;
    const auto wA = colliders[0].omega;
    const auto wB = colliders[1].omega;

    const auto ra = collision.contactPoint - bodyA.pos;
    const auto rb = collision.contactPoint - bodyB.pos;
    const auto va = bodyA.vel + bodyA.omega * rotateLeft(ra);
    const auto vb = bodyB.vel + bodyB.omega * rotateLeft(rb);

    const float invMassA = bodyA.invMass;
    const float invMassB = bodyB.invMass;

    const auto N = collision.normal;

    // auto correction = collision.depth * N;
    // const auto totalInvMass = invMassA + invMassB;
    // collidersAfter[0].pos -= correction * invMassA / totalInvMass;
    // collidersAfter[1].pos += correction * invMassB / totalInvMass;

    const auto e = 0.7f;
    const auto jNum = (1 + e) * dotProduct(vb - va, N);
    const auto jDenom = invMassA + invMassB + sqr(dotProduct(rotateLeft(ra), N)) / bodyA.invMoi +
          sqr(dotProduct(rotateLeft(rb), N)) / bodyB.invMoi;
    const auto j = jNum / jDenom;

    // apply the impulse
    collidersAfter[0].vel = vA + invMassA * j * N;
    collidersAfter[1].vel = vB - invMassB * j * N;

    collidersAfter[0].omega = wA + bodyA.invMoi * cross(ra, j * N);
    collidersAfter[1].omega = wB - bodyB.invMoi * cross(rb, j * N);
  }

  bool selection = false;
  Collision collision{};
  ConvexPolygon colliders[2];
  ConvexPolygon collidersAfter[2];
};

const int registered = registerApp("App.collision_response2d", []() -> IApp* { return new CollisionResponse2d; });
}
