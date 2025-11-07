#include <Game/Game.h>

#include <DFW/GameWorld/Camera/CameraSystem.h>
#include <DFW/GameWorld/Camera/CameraComponent.h>
#include <DFW/GameWorld/Graphics/RenderSystem.h>
#include <DFW/GameWorld/Graphics/DebugRenderSystem.h>
#include <DFW/GameWorld/TransformSystem.h>
#include <DFW/GameWorld/TransformComponent.h>
#include <DFW/GameWorld/Graphics/ModelComponent.h>
#include <DFW/GameWorld/Physics/PhysicsSystem.h>

#include <DFW/CoreSystems/Logging/Logger.h>

#include <DFW/Modules/ECS/Managers/SystemManager.h>
#include <DFW/Modules/Resource/Mesh/MeshLoader.h>

#include <DFW/Utility/FilePath.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>

namespace Game
{
    TennisGame::TennisGame(std::string const& a_stage_name, bool a_start_disabled)
        : DFW::StageBase(a_stage_name, a_start_disabled)
    {
    }

    void TennisGame::OnUpdate()
    {
        _ecs->UpdateECS();

    }

    void TennisGame::OnAttached()
    {
        SetupECS();

        Debug_CreateXYZAxisOrigin();

        // Camera
        {
            _camera_entity = _ecs->Registry().CreateEntity();
            _camera_entity.SetName("camera-main");
            _camera_entity.SetType<"Camera">();

            _camera_entity.AddComponent<DFW::TransformComponent>(DFW::Transform(glm::vec3(0, 0, -10.0f)));

            DFW::CameraComponent& camera_component = _camera_entity.AddComponent<DFW::CameraComponent>();
            DFW::CameraSystem* camera_system = _ecs->SystemManager().GetSystem<DFW::CameraSystem>();
            camera_system->ChangeCameraProjPerspective(camera_component, 90.f, (16.f/9.f), DFW::ClipSpace(0.1f, 1000.f));
            camera_system->RegisterCamera(camera_component, "camera-main");
            camera_system->SetActiveCamera("camera-main");
            camera_system->EnableCameraControl(camera_component);
        }

        // Playfield
        DFW::PhysicsSystem& ps = *_ecs->SystemManager().GetSystem<DFW::PhysicsSystem>();
        ps.Debug_EnableDebugDraw();
        ps.debug_draw_settings.mDrawShapeWireframe = false;
        ps.debug_draw_settings.mDrawBoundingBox = true;
        ps.JoltPhysics().SetGravity(JPH::Vec3Arg(0.0f, -10.f, 0.f));

        JPH::BodyID field = ps.CreateBoxRigidBody(DFW::Transform(glm::vec3(0.0f)), glm::vec3(110.f, 2.f, 240.f), JPH::EMotionType::Static);

        JPH::BodyID ball = ps.CreateSphereRigidBody(DFW::Transform(glm::vec3(0.0f, 64.f, -180.0f)), 2.f, JPH::EMotionType::Dynamic);
    }

    void TennisGame::OnRemoved()
    {
        _ecs->Terminate();
    }

    void TennisGame::SetupECS()
    {
        // Allocate
        _ecs = DFW::MakeUnique<DFW::DECS::ECSModule>();

        // Init Systems
        _ecs->Init();

        _ecs->SystemManager().AddSystem<DFW::TransformSystem>().ExecuteAfter<DFW::PhysicsSystem>();
        _ecs->SystemManager().AddSystem<DFW::CameraSystem>().ExecuteAfter<DFW::TransformSystem>();
        _ecs->SystemManager().AddSystem<DFW::RenderSystem>().ExecuteAfter<DFW::CameraSystem>();
        _ecs->SystemManager().AddSystem<DFW::DebugRenderSystem>().ExecuteAfter<DFW::CameraSystem>();
        
        _ecs->SystemManager().AddSystem<DFW::PhysicsSystem>();

        _ecs->SystemManager().CalculateSystemDependencies();
    }

    void TennisGame::Debug_CreateXYZAxisOrigin()
    {
        _debug_xyz = _ecs->Registry().CreateEntity();
        _debug_xyz.SetName("xyz-axis");

        _debug_xyz.AddComponent<DFW::TransformComponent>(DFW::Transform(glm::vec3(0.0f, 0.0f, 0.0f)));

        DFW::ModelComponent& xyz_model = _debug_xyz.AddComponent<DFW::ModelComponent>();
        DFW::FilePath filepath(std::string("models") + DIR_SLASH + "xyz" + DIR_SLASH + "xyz_10x10.glb");
        xyz_model.mesh = DFW::DResource::LoadMesh(filepath);
    }

} // End of namespace ~ Game.

