// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Entry point

#include <csignal>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app.h"
#include "camera.h"
#include "drawer.h"
#include "font.h"
#include "geom.h"
#include "matrix4.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL.h>
#include <SDL_opengl.h>
#undef main

const float CAMERA_UPDATE_RATIO = 0.8;

OrthoCamera g_OrthoCamera;
PerspectiveCamera g_PerspectiveCamera;
ICamera* g_CurrCamera = &g_OrthoCamera;

Matrix4f g_CameraTransform = translate({});

Vec2 g_ScreenSize{};

bool gMustReset = false;
bool gMustQuit = false;
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

  const auto aspectRatio = g_ScreenSize.x / g_ScreenSize.y;
  v.x *= aspectRatio;

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
  float x, y, z;
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
uniform mat4x4 mvp;
in vec3 pos;
in vec2 uv;
in vec4 color;
out vec4 v_color;
out vec2 v_uv;
void main()
{
    v_color = color;
    v_uv = uv;
    gl_Position = mvp * vec4(pos, 1);
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

struct PrimitiveBuffer
{
  PrimitiveBuffer() { glGenBuffers(1, &gpuVbo); }
  ~PrimitiveBuffer() { glDeleteBuffers(1, &gpuVbo); }

  void write(const Vertex& v) { cpuVbo.push_back(v); }
  void draw()
  {
    glBindBuffer(GL_ARRAY_BUFFER, gpuVbo);

    glEnableVertexAttribArray(attrib_position);
    glVertexAttribPointer(attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));

    glEnableVertexAttribArray(attrib_color);
    glVertexAttribPointer(attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));

    glEnableVertexAttribArray(attrib_uv);
    glVertexAttribPointer(attrib_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * cpuVbo.size(), cpuVbo.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(type, 0, cpuVbo.size());

    cpuVbo.clear();
  }

  GLuint type;
  GLuint gpuVbo;
  std::vector<Vertex> cpuVbo;
};

struct OpenGlDrawer : IDrawer
{
  OpenGlDrawer()
  {
    fontTexture = createFontTexture();
    whiteTexture = createWhiteTexture();

    shaderProgram = createShaderProgram();
    m_bufLines.type = GL_LINES;
    m_bufTris.type = GL_TRIANGLES;
    m_bufLinesUI.type = GL_LINES;
    m_bufTrisUI.type = GL_TRIANGLES;
  }

  ~OpenGlDrawer()
  {
    glDeleteProgram(shaderProgram);

    glDeleteTextures(1, &fontTexture);
    glDeleteTextures(1, &whiteTexture);
  }

  /////////////////////////////////////////////////////////////////////////////
  // IDrawer implementation

  void line(Vec2 a, Vec2 b, Color color) override { rawLine(m_bufLines, a, b, color); }
  void line(Vec3 a, Vec3 b, Color color) override { rawLine(m_bufLines, a, b, color); }

  void rect(Vec2 a, Vec2 b, Color color) override
  {
    const auto A = a;
    const auto B = a + b;

    const auto P0 = Vec2(A.x, A.y);
    const auto P1 = Vec2(B.x, A.y);
    const auto P2 = Vec2(B.x, B.y);
    const auto P3 = Vec2(A.x, B.y);

    rawLine(m_bufLines, P0, P1, color);
    rawLine(m_bufLines, P1, P2, color);
    rawLine(m_bufLines, P2, P3, color);
    rawLine(m_bufLines, P3, P0, color);
  }

  void circle(Vec2 center, float radius, Color color) override
  {
    auto const N = 20;
    Vec2 PREV{};

    for(int i = 0; i <= N; ++i)
    {
      const auto angle = i * 2 * M_PI / N;
      auto A = center + Vec2(cos(angle), sin(angle)) * radius;

      if(i > 0)
        rawLine(m_bufLines, PREV, A, color);

      PREV = A;
    }
  }

  void text(Vec2 pos, const char* text, Color color) override
  {
    Vec3 vX = {g_CameraTransform.data[0].elements[0], g_CameraTransform.data[1].elements[0],
          g_CameraTransform.data[2].elements[0]};
    const auto W = fontSize / magnitude(vX);
    const auto H = fontSize / magnitude(vX);

    while(*text)
    {
      rawChar(m_bufTris, pos, *text, color, W, H);
      pos.x += W;
      ++text;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Public API

  void uiText(Vec2 pos, const char* text, Color color)
  {
    const auto W = 16;
    const auto H = 16;

    while(*text)
    {
      rawChar(m_bufTrisUI, pos, *text, color, W, H);
      pos.x += W;
      ++text;
    }
  }

  void uiRect(Vec2 a, Vec2 b, Color color)
  {
    const auto A = a;
    const auto B = a + b;

    const auto P0 = Vec2(A.x, A.y);
    const auto P1 = Vec2(B.x, A.y);
    const auto P2 = Vec2(B.x, B.y);
    const auto P3 = Vec2(A.x, B.y);

    rawLine(m_bufLinesUI, P0, P1, color);
    rawLine(m_bufLinesUI, P1, P2, color);
    rawLine(m_bufLinesUI, P2, P3, color);
    rawLine(m_bufLinesUI, P3, P0, color);
  }

  void flush()
  {
    const int mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    const auto aspectRatio = g_ScreenSize.x / g_ScreenSize.y;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    // draw world
    {
      // setup transform
      {
        Matrix4f M = g_CurrCamera->getTransform(aspectRatio);
        g_CameraTransform = lerp(M, g_CameraTransform, CAMERA_UPDATE_RATIO);
        M = transpose(g_CameraTransform); // make the matrix column-major
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &M[0][0]);
      }

      glBindTexture(GL_TEXTURE_2D, whiteTexture);
      m_bufLines.draw();

      glBindTexture(GL_TEXTURE_2D, fontTexture);
      m_bufTris.draw();
    }

    // draw UI
    {
      // setup transform
      {
        Matrix4f M;
        M = translate({-1, -1, 0}) * scale({2.0f / g_ScreenSize.x, 2.0f / g_ScreenSize.y, 1});
        M = transpose(M); // make the matrix column-major
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &M[0][0]);
      }

      glBindTexture(GL_TEXTURE_2D, whiteTexture);
      m_bufLinesUI.draw();

      glBindTexture(GL_TEXTURE_2D, fontTexture);
      m_bufTrisUI.draw();
    }
  }

  private:
  static constexpr float fontSize = 0.032;

  void rawLine(PrimitiveBuffer& buf, Vec2 A, Vec2 B, Color color)
  {
    buf.write({A.x, A.y, 0, /* uv */ 0, 0, color.r, color.g, color.b, color.a});
    buf.write({B.x, B.y, 0, /* uv */ 0, 0, color.r, color.g, color.b, color.a});
  }

  void rawLine(PrimitiveBuffer& buf, Vec3 A, Vec3 B, Color color)
  {
    buf.write({A.x, A.y, A.z, /* uv */ 0, 0, color.r, color.g, color.b, color.a});
    buf.write({B.x, B.y, B.z, /* uv */ 0, 0, color.r, color.g, color.b, color.a});
  }

  void rawChar(PrimitiveBuffer& buf, Vec2 POS, char c, Color color, float W, float H)
  {
    const int cols = 16;
    const int rows = 8;

    const int col = c % cols;
    const int row = c / cols;

    const float u0 = (col + 0.0f) / cols;
    const float u1 = (col + 1.0f) / cols;
    const float v0 = (row + 1.0f) / rows;
    const float v1 = (row + 0.0f) / rows;

    POS.y -= H;

    buf.write({POS.x + 0, POS.y + 0, 1, /* uv */ u0, v0, color.r, color.g, color.b, color.a});
    buf.write({POS.x + W, POS.y + H, 1, /* uv */ u1, v1, color.r, color.g, color.b, color.a});
    buf.write({POS.x + W, POS.y + 0, 1, /* uv */ u1, v0, color.r, color.g, color.b, color.a});

    buf.write({POS.x + 0, POS.y + 0, 1, /* uv */ u0, v0, color.r, color.g, color.b, color.a});
    buf.write({POS.x + 0, POS.y + H, 1, /* uv */ u0, v1, color.r, color.g, color.b, color.a});
    buf.write({POS.x + W, POS.y + H, 1, /* uv */ u1, v1, color.r, color.g, color.b, color.a});
  }

  GLuint shaderProgram{};
  PrimitiveBuffer m_bufLines;
  PrimitiveBuffer m_bufTris;
  PrimitiveBuffer m_bufLinesUI;
  PrimitiveBuffer m_bufTrisUI;
  GLuint fontTexture{};
  GLuint whiteTexture{};
};

void takeScreenshot()
{
  static int counter;

  int width = g_ScreenSize.x;
  int height = g_ScreenSize.y;

  SDL_Surface* sshot = SDL_CreateRGBSurface(0, width, height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

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

void drawScreen(OpenGlDrawer& drawer, IApp* app, const char* appName)
{
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  app->draw(&drawer);

  drawer.uiRect({5, 5}, {g_ScreenSize.x - 10, g_ScreenSize.y - 10}, White);
  drawer.uiRect({32, g_ScreenSize.y - 32 + 8}, {800, -32 - 16}, White);
  drawer.uiText({32, g_ScreenSize.y - 32}, appName, White);
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
  case SDLK_END:
    return Key::End;
  case SDLK_RETURN:
    return Key::Return;
  case SDLK_F1:
    return Key::F1;
  case SDLK_F2:
    return Key::F2;
  case SDLK_F3:
    return Key::F3;
  case SDLK_F4:
    return Key::F4;
  case SDLK_KP_PLUS:
    return Key::KeyPad_Plus;
  case SDLK_KP_MINUS:
    return Key::KeyPad_Minus;
  case SDLK_KP_1:
    return Key::KeyPad_1;
  case SDLK_KP_2:
    return Key::KeyPad_2;
  case SDLK_KP_3:
    return Key::KeyPad_3;
  case SDLK_KP_4:
    return Key::KeyPad_4;
  case SDLK_KP_5:
    return Key::KeyPad_5;
  case SDLK_KP_6:
    return Key::KeyPad_6;
  case SDLK_KP_7:
    return Key::KeyPad_7;
  case SDLK_KP_8:
    return Key::KeyPad_8;
  case SDLK_KP_9:
    return Key::KeyPad_9;
  case SDLK_KP_0:
    return Key::KeyPad_0;
  }

  return Key::Unknown;
}

void processOneInputEvent(IApp* app, SDL_Event event)
{
  switch(event.type)
  {
  case SDL_QUIT:
  {
    gMustQuit = true;
    break;
  }
  case SDL_KEYDOWN:
  {
    switch(event.key.keysym.sym)
    {
    case SDLK_ESCAPE:
      gMustQuit = true;
      break;
    case SDLK_F2:
      gMustReset = true;
      break;
    case SDLK_F12:
      gMustScreenShot = true;
      break;
    case SDLK_KP_5:
    {
      // switch camera
      ICamera* a = &g_PerspectiveCamera;
      ICamera* b = &g_OrthoCamera;
      g_CurrCamera = (g_CurrCamera != a ? a : b);
      break;
    }
    }

    InputEvent inputEvent;
    inputEvent.pressed = true;
    inputEvent.key = fromSdlKey(event.key.keysym.sym);

    if(!g_CurrCamera->processEvent(inputEvent))
      app->processEvent(inputEvent);
    break;
  }
  case SDL_KEYUP:
  {
    InputEvent inputEvent;
    inputEvent.pressed = false;
    inputEvent.key = fromSdlKey(event.key.keysym.sym);
    app->processEvent(inputEvent);
    break;
  }
  case SDL_WINDOWEVENT:
  {
    if(event.window.event == SDL_WINDOWEVENT_RESIZED)
    {
      g_ScreenSize = {(float)event.window.data1, (float)event.window.data2};
      glViewport(0, 0, g_ScreenSize.x, g_ScreenSize.y);
    }
    break;
  }
  case SDL_MOUSEWHEEL:
  {
    if(event.wheel.y)
    {
      int mouseX, mouseY;
      SDL_GetMouseState(&mouseX, &mouseY);

      InputEvent inputEvent{};
      inputEvent.mousePos = screenCoordsToViewportCoords(SDL_Point{mouseX, mouseY});
      inputEvent.wheel = event.wheel.y > 0 ? 1 : -1;
      g_CurrCamera->processEvent(inputEvent);
    }
    break;
  }
  }
}

void readInput(IApp* app)
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
    processOneInputEvent(app, event);
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

    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

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

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

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

  const auto& registry = Registry();

  auto i_func = registry.find(appName);

  if(i_func == registry.end())
  {
    fprintf(stderr, "Unknown app: '%s'\n", appName.c_str());
    fprintf(stderr, "Available apps:\n");

    for(auto& app : registry)
      fprintf(stderr, "  %s\n", app.first.c_str());

    throw std::runtime_error("Unknown app");
  }

  SdlMainFrame mainFrame(("GeomSandbox: " + appName).c_str());

  OpenGlDrawer drawer;

  CreationFunc* const func = i_func->second;
  auto app = std::unique_ptr<IApp>(func());

  while(!gMustQuit)
  {
    readInput(app.get());

    if(gMustReset)
    {
      app.reset(func());
      gMustReset = false;
    }

    app->tick();
    drawScreen(drawer, app.get(), appName.c_str());

    mainFrame.flush();
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
