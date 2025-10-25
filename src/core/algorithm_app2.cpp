#include "algorithm_app2.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "fiber.h"
#include "sandbox.h"

namespace
{
Vec3 to3d(Vec2 v) { return {v.x, v.y, 0}; }

int64_t getSteadyClockMs()
{
  using namespace std::chrono;
  auto elapsedTime = steady_clock::now().time_since_epoch();
  return duration_cast<milliseconds>(elapsedTime).count();
}

int64_t getSteadyClockUs()
{
  using namespace std::chrono;
  auto elapsedTime = steady_clock::now().time_since_epoch();
  return duration_cast<microseconds>(elapsedTime).count();
}

struct Visualizer : IVisualizer
{
  struct VisualLine
  {
    Vec3 a, b;
    Color color;
    Vec2 offsetA, offsetB;
  };

  struct VisualRect
  {
    Vec2 a, size;
    Color color;
    Vec2 invariantSize;
  };

  struct VisualCircle
  {
    Vec2 center;
    float radius;
    Color color;
    float invariantRadius;
  };

  struct VisualText
  {
    Vec2 pos;
    std::string text;
    Color color;
    Vec2 offset;
  };

  struct ScreenState
  {
    std::vector<VisualLine> lines;
    std::vector<VisualRect> rects;
    std::vector<VisualCircle> circles;
    std::vector<VisualText> texts;
  };

  bool m_insideAlgorithmExecute = false;
  ScreenState m_screen; // the frame we're building
  ScreenState m_frontScreen; // the frame we're currently showing

  void rect(Vec2 a, Vec2 b, Color color, Vec2 invariantSize) { m_screen.rects.push_back({a, b, color, invariantSize}); }
  void circle(Vec2 center, float radius, Color color, float invariantRadius)
  {
    m_screen.circles.push_back({center, radius, color, invariantRadius});
  }
  void text(Vec2 pos, const char* text, Color color, Vec2 offset)
  {
    m_screen.texts.push_back({pos, text, color, offset});
  }
  void line(Vec2 a, Vec2 b, Color c, Vec2 offsetA, Vec2 offsetB) override
  {
    m_screen.lines.push_back({to3d(a), to3d(b), c, offsetA, offsetB});
  }
  void line(Vec3 a, Vec3 b, Color c, Vec2 offsetA, Vec2 offsetB) override
  {
    m_screen.lines.push_back({a, b, c, offsetA, offsetB});
  }
  void printf(const char* fmt, va_list args)
  {
    char buffer[4096]{};
    const int n = vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
    fwrite(buffer, 1, n, stdout);
    fflush(stdout);
  }

  void step() override
  {
    assert(m_insideAlgorithmExecute);
    m_frontScreen = std::move(m_screen);
    Fiber::yield();
  }

  void flush(IDrawer* drawer)
  {
    for(auto& line : m_frontScreen.lines)
      drawer->line(line.a, line.b, line.color, line.offsetA, line.offsetB);

    for(auto& rect : m_frontScreen.rects)
      drawer->rect(rect.a, rect.size, rect.color, rect.invariantSize);

    for(auto& circle : m_frontScreen.circles)
      drawer->circle(circle.center, circle.radius, circle.color, circle.invariantRadius);

    for(auto& text : m_frontScreen.texts)
      drawer->text(text.pos, text.text.c_str(), text.color, text.offset);
  }
};

std::vector<uint8_t> loadFile(const char* path)
{
  FILE* fp = fopen(path, "rb");

  if(!fp)
  {
    char buf[256];
    sprintf(buf, "Can't read '%s'", path);
    throw std::runtime_error(buf);
  }

  fseek(fp, 0, SEEK_END);
  const int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  std::vector<uint8_t> r(size);
  fread(r.data(), 1, r.size(), fp);

  fclose(fp);

  return r;
}

struct AlgorithmApp2 : IApp
{
  AlgorithmApp2(const AlgorithmDescription& algoDesc_)
      : m_algo(algoDesc_)
  {
    m_input = m_algo.generateInputFunc(m_seed);
  }

  const AlgorithmDescription& m_algo;
  std::any m_input, m_output;

  void draw(IDrawer* drawer) override
  {
    m_visuForFrame = {};

    gVisualizer = &m_visuForFrame;
    m_algo.displayFunc(&m_input, &m_output);
    gVisualizer = gNullVisualizer;

    m_visuForFrame.m_frontScreen = std::move(m_visuForFrame.m_screen);

    m_visuForFrame.flush(drawer);
    m_visuForAlgo.flush(drawer);
  }

  static void staticExecute(void* userParam) { ((AlgorithmApp2*)userParam)->executeFromFiber(); }

  void executeFromFiber()
  {
    // clear visualization
    m_visuForAlgo.m_screen = {};
    m_visuForAlgo.m_insideAlgorithmExecute = true;

    m_output = m_algo.executeFunc(m_input);

    // clear last step's visualisation
    m_visuForAlgo.m_insideAlgorithmExecute = false;
    m_visuForAlgo.m_frontScreen = {};
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void finishExecutionIfNeeded()
  {
    if(!m_fiber)
      return;

    while(!m_fiber->finished())
      m_fiber->resume();
    m_fiber.reset();
  }

  void keydown(Key key)
  {
    if(key == Key::Home)
    {
      runProfiling();
    }
    else if(key == Key::F1)
    {
      finishExecutionIfNeeded();
      m_seed++;
      m_input = m_algo.generateInputFunc(m_seed);
    }
    else if(key == Key::F3)
    {
      finishExecutionIfNeeded();
      loadTestCase(m_testCaseCounter++);
    }
    else if(key == Key::F4)
    {
      finishExecutionIfNeeded();
      loadInput();
    }
    else if(key == Key::Space || key == Key::Return)
    {
      gVisualizer = &m_visuForAlgo;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticExecute, this);

      if(key == Key::Return)
      {
        while(!m_fiber->finished())
          m_fiber->resume();
      }
      else
        m_fiber->resume();

      gVisualizer = gNullVisualizer;
    }
  }

  void runProfiling()
  {
    printf("Profiling ...\n");
    fflush(stdout);

    const int N = 8000;
    const auto t0 = getSteadyClockMs();
    int64_t processingTotalUs = 0;
    for(int k = 0; k < N; ++k)
    {
      fprintf(stderr, "\r%d/%d", k + 1, N);
      srand(k);
      m_input = m_algo.generateInputFunc(k);

      const auto us0 = getSteadyClockUs();
      m_algo.executeFunc(m_input);
      const auto us1 = getSteadyClockUs();

      processingTotalUs += (us1 - us0);
    }
    fprintf(stderr, " - OK\n");
    fflush(stderr);

    const auto t1 = getSteadyClockMs();
    printf("Processed %d instances in %.2fs (%.2f ms/instance)\n", N, (t1 - t0) / 1000.0, (t1 - t0) / (N * 1.0));
    printf("Input generation: %.3f ms/instance\n", ((t1 - t0) - processingTotalUs / 1000.0) / N);
    printf("      Processing: %.3f ms/instance\n", processingTotalUs / 1000.0 / N);
  }

  void loadTestCase(int which) { m_input = m_algo.getTestCaseInput(which); }

  void selfTest() override
  {
    const int N = (int)m_algo.testCases.size();
    for(int i = 0; i < N; ++i)
    {
      fprintf(stderr, "[%s] %s\n", m_algo.name, m_algo.testCases[i]);
      std::any input = m_algo.getTestCaseInput(i);
      std::any output = m_algo.executeFunc(input);
      m_algo.checkTestCaseOutput(i, &output);
    }
  }

  void loadInput()
  {
    if(!m_algo.loadInputFunc)
    {
      fprintf(stderr, "No support for loading for '%s' algorithm\n", m_algo.name);
      return;
    }

    try
    {
      auto data = loadFile("algo.in");
      m_algo.loadInputFunc(data);
    }
    catch(const std::exception& err)
    {
      fprintf(stderr, "Error: %s\n", err.what());
    }
  }

  std::unique_ptr<Fiber> m_fiber;

  Visualizer m_visuForAlgo;
  Visualizer m_visuForFrame;
  int m_testCaseCounter = 0;
  int m_seed = 1;
};
}

IApp* createAlgorithmApp2(const AlgorithmDescription& desc)
{
  (void)desc;
  return new AlgorithmApp2(desc);
}
