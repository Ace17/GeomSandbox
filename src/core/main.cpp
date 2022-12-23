// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Entry point

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app.h"
#include "drawer.h"
#include "font.h"
#include "geom.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL.h>
#include <SDL_opengl.h>
#undef main

struct Camera
{
  Vec2 pos{};
  float scale = 60.0f;
};

Camera g_Camera;
Camera g_TargetCamera;

Vec2 g_ScreenSize{};
bool gMustScreenShot = false;

template<typename T>
T lerp(T a, T b, float alpha)
{
  return a * (1.0f - alpha) + b * alpha;
}

Vec2 screenCoordsToViewportCoords(SDL_Point p)
{
  const auto halfScreenSize = g_ScreenSize * 0.5;
  Vec2 v;
  v.x = +(float(p.x) - halfScreenSize.x) / halfScreenSize.x;
  v.y = -(float(p.y) - halfScreenSize.y) / halfScreenSize.y;
  return v;
}

// 'screen' to 'logical' coordinates
Vec2 reverseTransform(SDL_Point p)
{
  Vec2 v = screenCoordsToViewportCoords(p);

  v.x *= g_ScreenSize.x;
  v.y *= g_ScreenSize.y;

  v.x = v.x / g_Camera.scale + g_Camera.pos.x;
  v.y = v.y / g_Camera.scale + g_Camera.pos.y;
  return v;
}

int createFontTexture()
{
  struct Pixel
  {
    uint8_t r, g, b, a;
  };

  const int glyphSize = 8;
  const int cols = 16;
  const int rows = 8;

  const int width = glyphSize * cols;
  const int height = glyphSize * rows;
  std::vector<Pixel> pixels(width * height);

  const Pixel white{0xff, 0xff, 0xff, 0xff};

  for(int c = 0; c < 128; ++c)
  {
    const int x = (c % cols) * glyphSize;
    const int y = (c / cols) * glyphSize;

    for(int row = 0; row < 8; ++row)
      for(int col = 0; col < 8; ++col)
      {
        if((font8x8_basic[c % 128][row] >> (col)) & 1)
          pixels[(x + col) + (y + row) * width] = white;
      }
  }

  GLuint texture;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

int createWhiteTexture()
{
  uint8_t data[64];
  memset(data, 0xff, sizeof data);

  GLuint texture;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

struct Vertex
{
  float x, y;
  float u, v;
  float r, g, b, a;
};

enum
{
  attrib_position,
  attrib_color,
  attrib_uv,
};

static const char* vertex_shader = R"(#version 130
uniform mat3x3 mvp;
in vec2 pos;
in vec2 uv;
in vec4 color;
out vec4 v_color;
out vec2 v_uv;
void main()
{
    v_color = color;
    v_uv = uv;
    vec3 transformedPos = mvp * vec3(pos, 1);
    gl_Position = vec4(transformedPos.xy, 0.0, 1.0 );
}
)";

static const char* fragment_shader = R"(#version 130
uniform sampler2D diffuse;
in vec4 v_color;
in vec2 v_uv;
out vec4 o_color;
void main()
{
    o_color = v_color * texture(diffuse, v_uv);
}
)";

int createShaderStage(int type, const char* code)
{
  auto vs = glCreateShader(type);

  glShaderSource(vs, 1, &code, nullptr);
  glCompileShader(vs);

  GLint status;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
  if(!status)
    throw std::runtime_error("Shader compilation error");

  return vs;
}

int createShaderProgram()
{
  auto vs = createShaderStage(GL_VERTEX_SHADER, vertex_shader);
  auto fs = createShaderStage(GL_FRAGMENT_SHADER, fragment_shader);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);

  glBindAttribLocation(program, attrib_position, "pos");
  glBindAttribLocation(program, attrib_color, "color");
  glBindAttribLocation(program, attrib_uv, "uv");
  glLinkProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}

struct OpenGlDrawer : IDrawer
{
  OpenGlDrawer()
  {
    glGenBuffers(1, &gpuVbo);

    fontTexture = createFontTexture();
    whiteTexture = createWhiteTexture();

    shaderProgram = createShaderProgram();
  }

  ~OpenGlDrawer()
  {
    glDeleteProgram(shaderProgram);

    glDeleteTextures(1, &fontTexture);
    glDeleteTextures(1, &whiteTexture);

    glDeleteBuffers(1, &gpuVbo);
  }

  void line(Vec2 a, Vec2 b, Color color) override { rawLine(a, b, color); }

  void rect(Vec2 a, Vec2 b, Color color) override
  {
    const auto A = a;
    const auto B = a + b;

    cpuVbo_Lines.push_back({A.x, A.y, 0, 0, color.r, color.g, color.b, color.a});
    cpuVbo_Lines.push_back({B.x, A.y, 0, 0, color.r, color.g, color.b, color.a});

    cpuVbo_Lines.push_back({B.x, A.y, 0, 0, color.r, color.g, color.b, color.a});
    cpuVbo_Lines.push_back({B.x, B.y, 0, 0, color.r, color.g, color.b, color.a});

    cpuVbo_Lines.push_back({B.x, B.y, 0, 0, color.r, color.g, color.b, color.a});
    cpuVbo_Lines.push_back({A.x, B.y, 0, 0, color.r, color.g, color.b, color.a});

    cpuVbo_Lines.push_back({A.x, B.y, 0, 0, color.r, color.g, color.b, color.a});
    cpuVbo_Lines.push_back({A.x, A.y, 0, 0, color.r, color.g, color.b, color.a});
  }

  static Vec2 direction(float angle) { return Vec2(cos(angle), sin(angle)); }

  void circle(Vec2 center, float radius, Color color)
  {
    auto const N = 20;
    Vec2 PREV{};

    for(int i = 0; i <= N; ++i)
    {
      auto A = center + direction(i * 2 * M_PI / N) * radius;

      if(i > 0)
        rawLine(PREV, A, color);

      PREV = A;
    }
  }

  static constexpr float fontSize = 32;

  void text(Vec2 pos, const char* text, Color color) override
  {
    const auto W = fontSize / g_Camera.scale;

    while(*text)
    {
      rawChar(pos, *text, color);
      pos.x += W;
      ++text;
    }
  }

  void rawLine(Vec2 A, Vec2 B, Color color)
  {
    cpuVbo_Lines.push_back({A.x, A.y, 0, 0, color.r, color.g, color.b, color.a});
    cpuVbo_Lines.push_back({B.x, B.y, 0, 0, color.r, color.g, color.b, color.a});
  }

  void rawChar(Vec2 POS, char c, Color color)
  {
    const int cols = 16;
    const int rows = 8;

    const int col = c % cols;
    const int row = c / cols;

    const float u0 = (col + 0.0f) / cols;
    const float u1 = (col + 1.0f) / cols;
    const float v0 = (row + 1.0f) / rows;
    const float v1 = (row + 0.0f) / rows;

    const auto W = fontSize / g_Camera.scale;
    const auto H = fontSize / g_Camera.scale;

    POS.y -= H;

    cpuVbo_Triangles.push_back({POS.x + 0, POS.y + 0, u0, v0, color.r, color.g, color.b, color.a});
    cpuVbo_Triangles.push_back({POS.x + W, POS.y + H, u1, v1, color.r, color.g, color.b, color.a});
    cpuVbo_Triangles.push_back({POS.x + W, POS.y + 0, u1, v0, color.r, color.g, color.b, color.a});

    cpuVbo_Triangles.push_back({POS.x + 0, POS.y + 0, u0, v0, color.r, color.g, color.b, color.a});
    cpuVbo_Triangles.push_back({POS.x + 0, POS.y + H, u0, v1, color.r, color.g, color.b, color.a});
    cpuVbo_Triangles.push_back({POS.x + W, POS.y + H, u1, v1, color.r, color.g, color.b, color.a});
  }

  void flush()
  {
    const int mvpLoc = glGetUniformLocation(shaderProgram, "mvp");

    // setup transform
    {
      float M[3][3] = {};
      M[0][0] = M[1][1] = M[2][2] = 1;

      M[0][0] = g_Camera.scale / g_ScreenSize.x;
      M[1][1] = g_Camera.scale / g_ScreenSize.y;

      M[2][0] = -g_Camera.pos.x * (g_Camera.scale / g_ScreenSize.x);
      M[2][1] = -g_Camera.pos.y * (g_Camera.scale / g_ScreenSize.y);

      glUniformMatrix3fv(mvpLoc, 1, GL_FALSE, &M[0][0]);
    }

    glUseProgram(shaderProgram);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, gpuVbo);

    glActiveTexture(GL_TEXTURE0);

    glEnableVertexAttribArray(attrib_position);
    glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));

    glEnableVertexAttribArray(attrib_color);
    glVertexAttribPointer(attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));

    glEnableVertexAttribArray(attrib_uv);
    glVertexAttribPointer(attrib_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * cpuVbo_Lines.size(), cpuVbo_Lines.data(), GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glDrawArrays(GL_LINES, 0, cpuVbo_Lines.size());

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * cpuVbo_Triangles.size(), cpuVbo_Triangles.data(), GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, cpuVbo_Triangles.size());

    cpuVbo_Lines.clear();
    cpuVbo_Triangles.clear();
  }

  private:
  GLuint shaderProgram{};
  GLuint gpuVbo;
  std::vector<Vertex> cpuVbo_Lines;
  std::vector<Vertex> cpuVbo_Triangles;
  GLuint fontTexture{};
  GLuint whiteTexture{};
};

void takeScreenshot()
{
  static int counter;

  int width = g_ScreenSize.x;
  int height = g_ScreenSize.y;

  SDL_Surface* sshot = SDL_CreateRGBSurface(0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

  uint8_t* pixels = (uint8_t*)sshot->pixels;

  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // reverse upside down
  {
    const auto rowSize = width * 4;
    std::vector<uint8_t> rowBuf(rowSize);

    for(int row = 0; row < height / 2; ++row)
    {
      const auto rowLo = row;
      const auto rowHi = height - 1 - row;
      auto pRowLo = pixels + rowLo * rowSize;
      auto pRowHi = pixels + rowHi * rowSize;
      memcpy(rowBuf.data(), pRowLo, rowSize);
      memcpy(pRowLo, pRowHi, rowSize);
      memcpy(pRowHi, rowBuf.data(), rowSize);
    }
  }

  // save to BMP file
  {
    char filename[256];
    snprintf(filename, sizeof filename, "screenshot-%03d.bmp", counter++);
    SDL_SaveBMP(sshot, filename);
    fprintf(stderr, "Saved screenshot to: %s\n", filename);
  }

  SDL_FreeSurface(sshot);
}

void drawScreen(OpenGlDrawer& drawer, IApp* app)
{
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  app->draw(&drawer);
  drawer.flush();

  if(gMustScreenShot)
  {
    takeScreenshot();
    gMustScreenShot = false;
  }
}

Key fromSdlKey(int key)
{
  switch(key)
  {
  case SDLK_PAGEUP:
    return Key::PageUp;
  case SDLK_PAGEDOWN:
    return Key::PageDown;
  case SDLK_LEFT:
    return Key::Left;
  case SDLK_RIGHT:
    return Key::Right;
  case SDLK_UP:
    return Key::Up;
  case SDLK_DOWN:
    return Key::Down;
  case SDLK_SPACE:
    return Key::Space;
  case SDLK_HOME:
    return Key::Home;
  case SDLK_RETURN:
    return Key::Return;
  }

  return Key::Unknown;
}

bool readInput(IApp* app, bool& reset)
{
  SDL_Event event;

  static const float scaleSpeed = 1.05;
  static const float scrollSpeed = 1.0;

  while(SDL_PollEvent(&event))
  {
    if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
      return false;

    if(event.type == SDL_KEYDOWN)
    {
      app->keydown(fromSdlKey(event.key.keysym.sym));

      switch(event.key.keysym.sym)
      {
      case SDLK_KP_PLUS:
        g_TargetCamera.scale = g_Camera.scale * scaleSpeed;
        break;
      case SDLK_KP_MINUS:
        g_TargetCamera.scale = g_Camera.scale / scaleSpeed;
        break;
      case SDLK_KP_4:
        g_TargetCamera.pos = g_Camera.pos + Vec2(-scrollSpeed, 0);
        break;
      case SDLK_KP_6:
        g_TargetCamera.pos = g_Camera.pos + Vec2(+scrollSpeed, 0);
        break;
      case SDLK_KP_2:
        g_TargetCamera.pos = g_Camera.pos + Vec2(0, -scrollSpeed);
        break;
      case SDLK_KP_8:
        g_TargetCamera.pos = g_Camera.pos + Vec2(0, +scrollSpeed);
        break;
      case SDLK_F2:
        reset = true;
        break;
      case SDLK_F12:
        gMustScreenShot = true;
        break;
      }
    }
    else if(event.type == SDL_KEYUP)
    {
      app->keyup(fromSdlKey(event.key.keysym.sym));
    }
    else if(event.type == SDL_WINDOWEVENT)
    {
      if(event.window.event == SDL_WINDOWEVENT_RESIZED)
      {
        g_ScreenSize = {(float)event.window.data1, (float)event.window.data2};
        glViewport(0, 0, g_ScreenSize.x, g_ScreenSize.y);
      }
    }
    else if(event.type == SDL_MOUSEWHEEL)
    {
      int mouseX, mouseY;
      SDL_GetMouseState(&mouseX, &mouseY);
      const Vec2 mousePos = reverseTransform(SDL_Point{mouseX, mouseY});

      if(event.wheel.y)
      {
        const auto scaleFactor = event.wheel.y > 0 ? (scaleSpeed * 1.1) : 1.0 / (scaleSpeed * 1.1);
        const auto newScale = g_Camera.scale * scaleFactor;
        auto relativePos = mousePos - g_Camera.pos;
        g_TargetCamera.pos = mousePos - relativePos * (g_Camera.scale / newScale);
        g_TargetCamera.scale = newScale;
      }
    }
  }

  return true;
}

std::map<std::string, CreationFunc*>& Registry()
{
  static std::map<std::string, CreationFunc*> registry;
  return registry;
}

int registerApp(const char* name, CreationFunc* func)
{
  Registry()[name] = func;
  return 0;
}

struct SdlMainFrame
{
  SdlMainFrame(const char* windowTitle)
  {
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
      throw std::runtime_error("Can't init SDL");

    g_ScreenSize = {1280, 720};

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    window = SDL_CreateWindow("Minimal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_ScreenSize.x, g_ScreenSize.y,
          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if(!window)
    {
      SDL_Quit();
      throw std::runtime_error("Can't create window");
    }

    context = SDL_GL_CreateContext(window);
    if(!context)
    {
      SDL_Quit();
      throw std::runtime_error("Can't create OpenGL context");
    }

    // create our unique vertex array
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    SDL_SetWindowTitle(window, windowTitle);
  }

  ~SdlMainFrame()
  {
    glUseProgram(0);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  void flush() { SDL_GL_SwapWindow(window); }

  private:
  SDL_Window* window{};
  SDL_GLContext context{};
  GLuint vao{};
};

void safeMain(span<const char*> args)
{
  std::string appName = "MainMenu";

  if(args.len >= 2)
    appName = args[1];

  auto i_func = Registry().find(appName);

  if(i_func == Registry().end())
  {
    fprintf(stderr, "Unknown app: '%s'\n", appName.c_str());
    fprintf(stderr, "Available apps:\n");

    for(auto& app : Registry())
      fprintf(stderr, "  %s\n", app.first.c_str());

    throw std::runtime_error("Unknown app");
  }

  SdlMainFrame mainFrame(("GeomSandbox: " + appName).c_str());

  OpenGlDrawer drawer;

  CreationFunc* func = Registry()[appName];
  auto app = std::unique_ptr<IApp>(func());

  bool reset = false;

  while(readInput(app.get(), reset))
  {
    if(reset)
    {
      app.reset(func());
      reset = false;
    }

    const float alpha = 0.8;
    g_Camera.pos = lerp(g_TargetCamera.pos, g_Camera.pos, alpha);
    g_Camera.scale = lerp(g_TargetCamera.scale, g_Camera.scale, alpha);

    app->tick();
    drawScreen(drawer, app.get());

    mainFrame.flush();

    SDL_Delay(10);
  }
}

int main(int argc, const char* argv[])
{
  try
  {
    safeMain({(size_t)argc, argv});
    return 0;
  }
  catch(const std::exception& e)
  {
    fprintf(stderr, "Fatal: %s\n", e.what());
    fflush(stderr);
    return 1;
  }
}
