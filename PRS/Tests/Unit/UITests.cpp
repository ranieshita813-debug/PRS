#include "TestFramework.h"
#include "PRS/UI/Context.h"
#include "PRS/UI/Font.h"
#include "PRS/UI/SoftwareCanvas.h"

using namespace PRS;
using namespace PRS::UI;

namespace
{
// Mirrors the real frame order: age input, feed touch events, then build the UI.
struct Harness
{
    Context ui;
    InputState input;
    Vec2 size{400.0f, 800.0f};

    Harness()
    {
        ui.SetTheme(Theme::PhoneDark(1.0f));
        ui.BeginFrame(size, input, 1.0f / 60.0f);
        ui.EndFrame();
    }

    void BeginInput() { input.BeginFrame(1.0f / 60.0f); }
    void BeginUI() { ui.BeginFrame(size, input, 1.0f / 60.0f); }
    void EndUI() { ui.EndFrame(); }

    Rect Root() const { return Rect(0.0f, 0.0f, size.x, size.y); }
};
} // namespace

TEST(UI_RectContainsAndClip)
{
    const Rect a(10.0f, 20.0f, 100.0f, 50.0f);
    CHECK(a.Contains({10.0f, 20.0f}));
    CHECK(!a.Contains({110.0f, 70.0f}));

    const Rect overlapping = a.ClipTo(Rect(50.0f, 0.0f, 200.0f, 200.0f));
    CHECK(overlapping.x == 50.0f);
    CHECK(overlapping.w == 60.0f);

    CHECK(a.ClipTo(Rect(500.0f, 500.0f, 10.0f, 10.0f)).Empty());
}

TEST(UI_FontMeasuresText)
{
    const BitmapFont& font = BitmapFont::Default();
    CHECK(font.AtlasPixels() != nullptr);
    CHECK(font.Measure("PLAY", 14.0f) > 0.0f);
    CHECK(font.Measure("PLAY", 14.0f) > font.Measure("P", 14.0f));
    CHECK(font.LineHeight(14.0f) > 0.0f);
    CHECK(font.Find('a') != nullptr); // lowercase maps to uppercase
}

TEST(UI_DrawListBatchesAndEmitsGeometry)
{
    DrawList list;
    list.Reset();

    list.AddRect({0, 0, 10, 10}, Color(1, 1, 1, 1));
    list.AddRect({0, 10, 10, 10}, Color(1, 1, 1, 1));
    CHECK(list.Commands().size() == 1);
    CHECK(list.Indices().size() == 12);
    CHECK(list.Vertices().size() == 8);

    list.AddText("HI", {0, 0}, 14.0f, Color(1, 1, 1, 1));
    CHECK(list.Commands().size() == 2);
    CHECK(list.Commands()[1].kind == CmdKind::Text);
}

TEST(UI_ButtonActivatesOnTap)
{
    Harness h;
    bool activated = false;

    auto frame = [&](bool down, bool up) {
        h.BeginInput();
        if (down) h.input.OnPointerDown(0, {100.0f, 40.0f});
        if (up) h.input.OnPointerUp(0, {100.0f, 40.0f});
        h.BeginUI();
        h.ui.BeginView("root", h.Root());
        if (h.ui.Button("play", "PLAY")) activated = true;
        h.ui.EndView();
        h.EndUI();
    };

    frame(true, false);
    frame(false, false);
    frame(false, true);

    CHECK(activated);
}

TEST(UI_ButtonIgnoresDrag)
{
    Harness h;
    bool activated = false;

    h.BeginInput();
    h.input.OnPointerDown(0, {100.0f, 40.0f});
    h.BeginUI();
    h.ui.BeginView("root", h.Root());
    h.ui.Button("play", "PLAY");
    h.ui.EndView();
    h.EndUI();

    h.BeginInput();
    h.input.OnPointerMove(0, {100.0f, 240.0f});
    h.BeginUI();
    h.ui.BeginView("root", h.Root());
    h.ui.Button("play", "PLAY");
    h.ui.EndView();
    h.EndUI();

    h.BeginInput();
    h.input.OnPointerUp(0, {100.0f, 240.0f});
    h.BeginUI();
    h.ui.BeginView("root", h.Root());
    h.ui.Button("play", "PLAY");
    h.ui.EndView();
    h.EndUI();

    CHECK(!activated);
}

TEST(UI_SliderFollowsDrag)
{
    Harness h;
    f32 value = 0.5f;

    h.BeginInput();
    h.input.OnPointerDown(0, {40.0f, 52.0f});
    h.BeginUI();
    h.ui.BeginView("root", h.Root());
    h.ui.Slider("scale", "SCALE", value, 0.5f, 2.0f);
    h.ui.EndView();
    h.EndUI();

    h.BeginInput();
    h.input.OnPointerMove(0, {350.0f, 52.0f});
    h.BeginUI();
    h.ui.BeginView("root", h.Root());
    h.ui.Slider("scale", "SCALE", value, 0.5f, 2.0f);
    h.ui.EndView();
    h.EndUI();

    CHECK(value > 1.5f);
    CHECK(value <= 2.0f);
}

TEST(UI_ScrollViewClampsToContent)
{
    Harness h;

    auto frame = [&](bool down, bool move, Vec2 position) {
        h.BeginInput();
        if (down) h.input.OnPointerDown(0, position);
        if (move) h.input.OnPointerMove(0, position);
        h.BeginUI();
        h.ui.BeginView("list", h.Root(), true);
        for (int i = 0; i < 40; ++i) h.ui.Label("ROW");
        h.ui.EndView();
        h.EndUI();
    };

    frame(true, false, {200.0f, 700.0f});
    frame(false, true, {200.0f, 500.0f});

    const f32 scrolled = h.ui.ScrollOffset("list");
    CHECK(scrolled > 0.0f);

    // Drag far past the end: the clamp must keep it inside the content.
    frame(false, true, {200.0f, 0.0f});
    frame(false, true, {200.0f, -5000.0f});
    const f32 clamped = h.ui.ScrollOffset("list");
    CHECK(clamped > 0.0f);
    CHECK(clamped <= h.ui.ContentHeight("list") + 64.0f);
}

TEST(UI_SoftwareCanvasFillsPixels)
{
    DrawList list;
    list.Reset();
    list.AddRect({0.0f, 0.0f, 16.0f, 16.0f}, Color(1, 1, 1, 1));

    SoftwareCanvas canvas;
    canvas.Resize(32, 32);
    canvas.Clear(Color(0, 0, 0, 1));
    canvas.Render(list, BitmapFont::Default());

    const u8* pixels = canvas.Pixels();
    CHECK(pixels != nullptr);
    CHECK(pixels[0] > 200);
    CHECK(pixels[(31 * 32 + 31) * 4] < 40);
}
