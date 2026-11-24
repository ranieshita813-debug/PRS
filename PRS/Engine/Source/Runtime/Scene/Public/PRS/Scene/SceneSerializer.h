#pragma once
#include "PRS/Scene/Scene.h"
#include <string>

namespace PRS::Scene
{
// Text scene format: stable, diffable in version control and easy to inspect
// without the editor. Save/Load go through the string versions so the same code
// path is covered by tests and by the editor's undo/redo snapshots.
class SceneSerializer
{
public:
    static bool Save(const Scene& scene, const std::string& path);
    static bool Load(Scene& scene, const std::string& path);

    static std::string ToString(const Scene& scene);
    static bool FromString(Scene& scene, const std::string& text);

    static constexpr const char* Extension() { return ".prsscene"; }
    static constexpr u32 Version = 2;
};
} // namespace PRS::Scene
