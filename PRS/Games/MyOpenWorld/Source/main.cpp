#include "PRS/Core/Log.h"
#include "PRS/Engine/Application.h"
#include "PRS/Engine/Entry.h"
#include "PRS/Platform/Input/Input.h"
#include "PRS/Renderer/Renderer.h"
#include "PRS/Scene/Scene.h"

using namespace PRS;
namespace Sc = PRS::Scene;

// Demo scene for the forward Vulkan renderer: a plane, a spinning cube, a sun
// and a camera. WASD nudges the camera, ESC quits.
class MyOpenWorldApp final : public Application
{
public:
    MyOpenWorldApp() : Application(MakeConfig()) {}

protected:
    void OnInit() override
    {
        m_ground = m_scene.CreateEntity("Ground");
        m_scene.World().Emplace<Sc::MeshRenderer>(
            m_ground, Sc::MeshRenderer{"plane", "default", true, false});

        m_cube = m_scene.CreateEntity("Cube");
        Sc::Transform cubeTransform;
        cubeTransform.position = {0.0f, 1.25f, 0.0f};
        cubeTransform.scale = {2.0f, 2.0f, 2.0f};
        m_scene.World().Emplace<Sc::Transform>(m_cube, cubeTransform);
        m_scene.World().Emplace<Sc::MeshRenderer>(
            m_cube, Sc::MeshRenderer{"cube", "default", true, true});

        m_camera = m_scene.CreateEntity("MainCamera");
        Sc::Transform cameraTransform;
        cameraTransform.position = {0.0f, 5.0f, -9.0f};
        m_scene.World().Emplace<Sc::Transform>(m_camera, cameraTransform);
        m_scene.World().Emplace<Sc::Camera>(m_camera,
                                            Sc::Camera{60.0f, 0.1f, 500.0f, 16.0f / 9.0f, true});

        m_light = m_scene.CreateEntity("Sun");
        m_scene.World().Get<Sc::Transform>(m_light).rotation =
            Quat::FromEuler(Radians(-50.0f), Radians(-35.0f), 0.0f);
        m_scene.World().Emplace<Sc::Light>(
            m_light, Sc::Light{Sc::LightType::Directional, Vec3{1.0f, 0.95f, 0.85f}, 1.2f});

        if (GetRenderer())
            PRS_INFO("MyOpenWorld: %zu entities, rendering with %s", m_scene.EntityCount(),
                     GetDevice() ? GetDevice()->BackendName() : "?");
        else
            PRS_INFO("MyOpenWorld: running without rendering (no Vulkan device)");
    }

    void OnUpdate(f32 dt) override
    {
        m_time += dt;

        Window& window = GetWindow();
        if (Input::IsKeyDown(window, Key::Escape)) Quit();

        const f32 speed = 6.0f * dt;
        Sc::Transform& camera = m_scene.World().Get<Sc::Transform>(m_camera);
        if (Input::IsKeyDown(window, Key::W)) camera.position += camera.Forward() * speed;
        if (Input::IsKeyDown(window, Key::S)) camera.position -= camera.Forward() * speed;
        if (Input::IsKeyDown(window, Key::A)) camera.position -= camera.Right() * speed;
        if (Input::IsKeyDown(window, Key::D)) camera.position += camera.Right() * speed;

        Sc::Transform& cube = m_scene.World().Get<Sc::Transform>(m_cube);
        cube.rotation = Quat::FromAxisAngle(AxisY(), m_time * 0.8f) *
                        Quat::FromAxisAngle(AxisX(), Radians(28.0f));
        cube.position.y = 1.25f + 0.35f * std::sin(m_time * 1.6f);
    }

    void OnRender(f32 /*alpha*/) override
    {
        Renderer::Renderer* renderer = GetRenderer();
        if (renderer == nullptr) return;
        renderer->Render(m_scene, m_camera, m_time);
    }

private:
    static AppConfig MakeConfig()
    {
        AppConfig config;
        config.name = "MyOpenWorld (PRS)";
        config.width = 1280;
        config.height = 720;
        return config;
    }

    Sc::Scene m_scene{"MyOpenWorld"};
    ECS::Entity m_ground;
    ECS::Entity m_cube;
    ECS::Entity m_camera;
    ECS::Entity m_light;
    f32 m_time = 0.0f;
};

Scope<Application> PRS::CreateApplication() { return MakeScope<MyOpenWorldApp>(); }
