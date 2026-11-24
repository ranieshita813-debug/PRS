#include "PRS/Scene/SceneSerializer.h"
#include "PRS/Core/Log.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace PRS::Scene
{
namespace
{
struct EntityRecord
{
    ECS::Entity handle{};
    std::string name = "Entity";
    PRS::Transform transform;
    bool hasTransform = false;
    bool hasParent = false;
    ECS::Entity parent{};
    bool hasCamera = false;
    Camera camera;
    bool hasMesh = false;
    MeshRenderer mesh;
    bool hasLight = false;
    Light light;
    bool disabled = false;
};

void WriteTransform(std::ostream& out, const PRS::Transform& t)
{
    const Quat q = t.rotation;
    out << "transform " << t.position.x << ' ' << t.position.y << ' ' << t.position.z << ' ' << q.x
        << ' ' << q.y << ' ' << q.z << ' ' << q.w << ' ' << t.scale.x << ' ' << t.scale.y << ' '
        << t.scale.z << '\n';
}

bool ReadBlock(std::istream& in, EntityRecord& record)
{
    std::string keyword;
    while (in >> keyword)
    {
        if (keyword == "end") return true;
        if (keyword == "id")
        {
            u32 index = 0, generation = 0;
            if (!(in >> index >> generation)) return false;
        }
        else if (keyword == "name")
        {
            if (!(in >> std::quoted(record.name))) return false;
        }
        else if (keyword == "transform")
        {
            PRS::Transform& t = record.transform;
            if (!(in >> t.position.x >> t.position.y >> t.position.z >> t.rotation.x >>
                  t.rotation.y >> t.rotation.z >> t.rotation.w >> t.scale.x >> t.scale.y >>
                  t.scale.z))
                return false;
            t.rotation = t.rotation.Normalized();
            record.hasTransform = true;
        }
        else if (keyword == "parent")
        {
            if (!(in >> record.parent.index >> record.parent.generation)) return false;
            record.hasParent = true;
        }
        else if (keyword == "camera")
        {
            int primary = 0;
            if (!(in >> record.camera.fovYDegrees >> record.camera.nearZ >> record.camera.farZ >>
                  record.camera.aspect >> primary))
                return false;
            record.camera.primary = primary != 0;
            record.hasCamera = true;
        }
        else if (keyword == "mesh")
        {
            int visible = 1;
            int shadows = 1;
            if (!(in >> std::quoted(record.mesh.mesh) >> std::quoted(record.mesh.material) >>
                  visible >> shadows))
                return false;
            record.mesh.visible = visible != 0;
            record.mesh.castShadows = shadows != 0;
            record.hasMesh = true;
        }
        else if (keyword == "light")
        {
            int type = 0;
            int shadows = 1;
            if (!(in >> type >> record.light.color.x >> record.light.color.y >>
                  record.light.color.z >> record.light.intensity >> record.light.range >>
                  record.light.innerConeDegrees >> record.light.outerConeDegrees >> shadows))
                return false;
            record.light.type = static_cast<LightType>(type);
            record.light.castShadows = shadows != 0;
            record.hasLight = true;
        }
        else if (keyword == "disabled")
        {
            record.disabled = true;
        }
        else
        {
            PRS_ERROR("SceneSerializer: unexpected token '%s' in entity block", keyword.c_str());
            return false;
        }
    }
    // A block that reaches end-of-file without "end" is truncated.
    return false;
}
} // namespace

std::string SceneSerializer::ToString(const Scene& scene)
{
    std::ostringstream out;
    out << std::setprecision(8);

    out << "PRSSCENE " << Version << '\n';
    out << "name " << std::quoted(scene.GetName()) << '\n';

    Array<ECS::Entity> entities;
    scene.World().Query<Name>().ForEach([&](ECS::Entity e, const Name&) { entities.push_back(e); });

    out << "count " << entities.size() << '\n';

    for (ECS::Entity entity : entities)
    {
        out << "entity " << entity.index << ' ' << entity.generation << '\n';
        out << "id " << entity.index << ' ' << entity.generation << '\n';

        if (const Name* name = scene.World().TryGet<Name>(entity))
            out << "name " << std::quoted(name->value) << '\n';

        if (const Transform* transform = scene.World().TryGet<Transform>(entity))
            WriteTransform(out, *transform);

        if (const Parent* parent = scene.World().TryGet<Parent>(entity))
            out << "parent " << parent->entity.index << ' ' << parent->entity.generation << '\n';

        if (const Camera* camera = scene.World().TryGet<Camera>(entity))
            out << "camera " << camera->fovYDegrees << ' ' << camera->nearZ << ' ' << camera->farZ
                << ' ' << camera->aspect << ' ' << (camera->primary ? 1 : 0) << '\n';

        if (const MeshRenderer* mesh = scene.World().TryGet<MeshRenderer>(entity))
            out << "mesh " << std::quoted(mesh->mesh) << ' ' << std::quoted(mesh->material) << ' '
                << (mesh->visible ? 1 : 0) << ' ' << (mesh->castShadows ? 1 : 0) << '\n';

        if (const Light* light = scene.World().TryGet<Light>(entity))
            out << "light " << static_cast<int>(light->type) << ' ' << light->color.x << ' '
                << light->color.y << ' ' << light->color.z << ' ' << light->intensity << ' '
                << light->range << ' ' << light->innerConeDegrees << ' ' << light->outerConeDegrees
                << ' ' << (light->castShadows ? 1 : 0) << '\n';

        if (scene.World().Has<Disabled>(entity)) out << "disabled\n";

        out << "end\n";
    }
    return out.str();
}

bool SceneSerializer::FromString(Scene& scene, const std::string& text)
{
    std::istringstream in(text);
    std::string keyword;
    u32 version = 0;

    if (!(in >> keyword) || keyword != "PRSSCENE") return false;
    if (!(in >> version) || version == 0 || version > Version) return false;

    Array<EntityRecord> records;
    std::string sceneName = scene.GetName();

    while (in >> keyword)
    {
        if (keyword == "name")
        {
            if (!(in >> std::quoted(sceneName))) return false;
        }
        else if (keyword == "count")
        {
            size_t count = 0;
            if (!(in >> count)) return false;
            records.reserve(count);
        }
        else if (keyword == "entity")
        {
            EntityRecord record;
            if (!(in >> record.handle.index >> record.handle.generation)) return false;
            if (!ReadBlock(in, record)) return false;
            records.push_back(std::move(record));
        }
        else
        {
            PRS_ERROR("SceneSerializer: unexpected token '%s'", keyword.c_str());
            return false;
        }
    }

    // Apply: create every entity first so parent links can reference any order.
    scene.Clear();
    scene.SetName(sceneName);

    HashMap<u64, ECS::Entity> remap;
    for (const EntityRecord& record : records)
    {
        const ECS::Entity entity = scene.CreateEntity(record.name);
        remap.emplace((static_cast<u64>(record.handle.index) << 32) | record.handle.generation,
                      entity);
        if (record.hasTransform) scene.World().Emplace<Transform>(entity, record.transform);
        if (record.hasCamera) scene.World().Emplace<Camera>(entity, record.camera);
        if (record.hasMesh) scene.World().Emplace<MeshRenderer>(entity, record.mesh);
        if (record.hasLight) scene.World().Emplace<Light>(entity, record.light);
        if (record.disabled) scene.World().Emplace<Disabled>(entity, Disabled{});
    }

    for (const EntityRecord& record : records)
    {
        if (!record.hasParent) continue;
        const auto child =
            remap.find((static_cast<u64>(record.handle.index) << 32) | record.handle.generation);
        const auto parent =
            remap.find((static_cast<u64>(record.parent.index) << 32) | record.parent.generation);
        if (child == remap.end() || parent == remap.end()) continue;
        scene.SetParent(child->second, parent->second);
    }
    return true;
}

bool SceneSerializer::Save(const Scene& scene, const std::string& path)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    const std::string text = ToString(scene);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return out.good();
}

bool SceneSerializer::Load(Scene& scene, const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return FromString(scene, buffer.str());
}
} // namespace PRS::Scene
