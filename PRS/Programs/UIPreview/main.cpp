#include "MobileShellUI.h"
#include "PRS/UI/Context.h"
#include "PRS/UI/Font.h"
#include "PRS/UI/SoftwareCanvas.h"
#include "PngWriter.h"

#include <cstdio>
#include <filesystem>
#include <string>

namespace
{
using namespace PRS;

struct Shot
{
    const char* name;
    u32 width;
    u32 height;
    f32 density;
    MobileShell::Tab tab;
};

void RenderShot(const Shot& shot, const std::string& directory)
{
    UI::Context ui;
    ui.SetTheme(UI::Theme::PhoneDark(shot.density));
    UI::InputState input;

    MobileShell::State state;
    state.tab = shot.tab;
    state.Notify("PREVIEW RENDER");

    UI::SoftwareCanvas canvas;
    canvas.Resize(shot.width, shot.height);

    // A few frames so the pressed/hover animations settle before capturing.
    for (int frame = 0; frame < 4; ++frame)
    {
        input.BeginFrame(1.0f / 60.0f);
        ui.BeginFrame({static_cast<f32>(shot.width), static_cast<f32>(shot.height)}, input, 1.0f / 60.0f);
        MobileShell::Draw(ui, UI::Rect(0.0f, 0.0f, static_cast<f32>(shot.width),
                                      static_cast<f32>(shot.height)),
                          state);
        ui.EndFrame();
    }

    canvas.Clear(ui.GetTheme().background);
    canvas.Render(ui.Draws(), UI::BitmapFont::Default());

    const std::string path = directory + "/" + shot.name + ".png";
    if (MobileShell::WritePng(path.c_str(), canvas.Pixels(), canvas.Width(), canvas.Height()))
    {
        std::printf("wrote %s (%ux%u, %zu draw commands)\n", path.c_str(), canvas.Width(),
                    canvas.Height(), ui.Draws().Commands().size());
    }
    else
    {
        std::printf("failed to write %s\n", path.c_str());
    }
}
} // namespace

int main(int argc, char** argv)
{
    const std::string directory = argc > 1 ? argv[1] : "Saved";
    std::error_code error;
    std::filesystem::create_directories(directory, error);

    // Layout is density-independent, so the previews render at half a phone's
    // pixel resolution and look identical to the device while staying small.
    const Shot shots[] = {
        {"ui-home", 540, 1200, 1.5f, MobileShell::Tab::Home},
        {"ui-settings", 540, 1200, 1.5f, MobileShell::Tab::Settings},
        {"ui-about", 540, 1200, 1.5f, MobileShell::Tab::About},
        {"ui-tablet", 800, 1280, 1.0f, MobileShell::Tab::Home},
    };

    for (const Shot& shot : shots) RenderShot(shot, directory);
    return 0;
}
