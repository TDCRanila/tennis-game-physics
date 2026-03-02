#include <DFW/Utility/ColourUtility.h>
#include <DFW/Utility/JoltUtility.h>
#include <DFW/Defines/Defines.h>

#include <Game/Game.h>

#include <DFW/GameWorld/Camera/CameraSystem.h>
#include <DFW/GameWorld/Camera/CameraComponent.h>
#include <DFW/GameWorld/Graphics/RenderSystem.h>
#include <DFW/GameWorld/Graphics/DebugRenderSystem.h>
#include <DFW/GameWorld/TransformSystem.h>
#include <DFW/GameWorld/TransformComponent.h>
#include <DFW/GameWorld/Graphics/ModelComponent.h>
#include <DFW/GameWorld/Physics/PhysicsSystem.h>

#include <DFW/GameWorld/Editor/GameViewport.h>
#include <DFW/Modules/Editor/EditorElementFiller.h>

#include <DFW/Modules/ECS/Managers/SystemManager.h>
#include <DFW/Modules/Resource/Mesh/MeshLoader.h>

#include <DFW/CoreSystems/Logging/Logger.h>
#include <DFW/CoreSystems/TimeTracker.h>
#include <DFW/CoreSystems/GameClock.h>

#include <DFW/Utility/FilePath.h>

#include <asm-generic/errno.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>

namespace Game
{
    enum class GameState
    {
        Init,
        SetupStart,
        SetupUpdate,
        SimulateStart,
        SimulateUpdate,
        SimulateExit,
        None,
    };

    struct ProjectileLaunchData
    {
        glm::vec3 position_start = { 0.0f, 0.0f, 0.0f };
        glm::vec3 position_end = { 0.0f, 0.0f, 0.0f };
        glm::vec3 gravity = { 0.0f, 0.0f, 0.0f };
        float32 launch_angle = 0.0f;
        // float32 travel_time = 0.0f;
        // float32 magnus_effect
        // float32 spin_angle;
    };

    GameState current_state = GameState::Init;
    ProjectileLaunchData projectile_launch_data;
    glm::vec3 final_launch_velocity;
    JPH::BodyID field;
    JPH::BodyID ball;
    JPH::BodyID box;

    std::vector<glm::vec3> recorded_projectile_trajectory_positions;

    glm::vec3 CalculateLaunchVelocity(ProjectileLaunchData const& a_projectile_launch_data)
    {
        ProjectileLaunchData const& data = a_projectile_launch_data;

        float32 const distance = glm::distance(data.position_end, data.position_start);
        float32 const launch_velocity_total_a = distance * std::abs(data.gravity.y);
        float32 const launch_velocity_total_b = std::sin(glm::radians(2 * data.launch_angle));
        float32 const launch_velocity_total  = std::sqrt(std::abs(launch_velocity_total_a / launch_velocity_total_b));
        //float32 const launch_velocity_total = std::sqrt((std::abs(distance * std::abs(data.gravity.y)) / std::sin(glm::radians(2 * data.launch_angle))));

        glm::vec3 launch_velocity(0.0f);

        // Vertical Velocity
        launch_velocity.y = sin(glm::radians(data.launch_angle)) * launch_velocity_total;

        // Horizontal Velocity
        glm::vec3 const horizontal_direction = glm::normalize(glm::vec3(data.position_end.x, 0, data.position_end.z) - glm::vec3(data.position_start.x, 0, data.position_start.z));
        float32 const velocity_horizontal_total = cos(glm::radians(data.launch_angle)) * launch_velocity_total;
        launch_velocity.x = horizontal_direction.x * velocity_horizontal_total;
        launch_velocity.z = horizontal_direction.z * velocity_horizontal_total;

        return launch_velocity;
    }

    TennisGame::TennisGame(std::string const& a_stage_name, bool a_start_disabled)
        : DFW::StageBase(a_stage_name, a_start_disabled)
        , _has_dockspace_been_created(false)
    {
    }

    void TennisGame::OnUpdate()
    {
        DFW::PhysicsSystem& ps = *_ecs->SystemManager().GetSystem<DFW::PhysicsSystem>();
        DFW::DebugRenderSystem& drs = *_ecs->SystemManager().GetSystem<DFW::DebugRenderSystem>();
        float32 const dt = DFW::CoreService::GetGameClock()->GetLastFrameDeltaTime();
        
        bool has_processed_state_this_frame(false);
        if (!has_processed_state_this_frame && current_state == GameState::Init)
        {
            // Pause Simulation.
            ps.PausePhysics();
            
            // Playfield
            field = ps.CreateBoxRigidBody(DFW::Transform(glm::vec3(0.0f, -2.f, 0.0f)), glm::vec3(600.f, 2.f, 1200.f), JPH::EMotionType::Static);
            
            // Spawn ball
            ball = ps.CreateSphereRigidBody(DFW::Transform(glm::vec3(0.0f, 1.0f, 0.0f)), 1.f, JPH::EMotionType::Dynamic);
            
            JPH::BodyLockWrite lock(ps.JoltPhysics().GetBodyLockInterface(), ball);
            if (lock.Succeeded())
            {
                JPH::MotionProperties *mp = lock.GetBody().GetMotionProperties();
                mp->SetLinearDamping(0.0f);
                mp->SetAngularDamping(0.0f);
            }
            
            current_state = GameState::SetupStart;
            has_processed_state_this_frame = true;
        }
        
        if (!has_processed_state_this_frame && current_state == GameState::SetupStart)
        {
            // Set Gravitiy
            ps.JoltPhysics().SetGravity(JPH::Vec3Arg(0.0f, -10.f, 0.f));
            
            // Calculate the velocity based on input parameters
            projectile_launch_data = ProjectileLaunchData();
            projectile_launch_data.gravity = DFW::DUtility::JPHToGLM(ps.JoltPhysics().GetGravity());
            projectile_launch_data.launch_angle = 35.f;
            projectile_launch_data.position_start = DFW::DUtility::JPHToGLM(ps.JoltBodyInterface().GetPosition(ball));
            projectile_launch_data.position_end = projectile_launch_data.position_start + glm::vec3(50.0f, 0.0f, 100.f);
            
            final_launch_velocity = CalculateLaunchVelocity(projectile_launch_data);
            
            // Set Velocities
            ps.JoltBodyInterface().SetFriction(ball, 0.f);
            ps.JoltBodyInterface().SetRestitution(ball, 0.5f);
            ps.JoltBodyInterface().SetLinearVelocity(ball, DFW::DUtility::GLMToJPH(final_launch_velocity));
                        
            current_state = GameState::SetupUpdate;
            has_processed_state_this_frame = true;
        }

        if (!has_processed_state_this_frame && current_state == GameState::SetupUpdate)
        {
            // imgui stuff with ui to change parameters
            // pre-visualize projectile trajectory

            static DFW::TimeTracker timer(true);
            if (timer.FetchElapsedTime() > DFW::TimeUnit(3.0f))
            {
                current_state = GameState::SimulateStart;
            }

            has_processed_state_this_frame = true;
        }

        if (!has_processed_state_this_frame && current_state == GameState::SimulateStart)
        {
            // Unpause Simulation.
            ps.UnpausePhysics();

            current_state = GameState::SimulateUpdate;
            has_processed_state_this_frame = true;
        }

        if (!has_processed_state_this_frame && current_state == GameState::SimulateUpdate)
        {
            // Draw Projectile Positions.
            drs.DrawSphere(DFW::Transform(projectile_launch_data.position_start), 2.0f, DFW::DebugDrawSettings(DFW::ColourRGBA::Green));
            drs.DrawSphere(DFW::Transform(projectile_launch_data.position_end), 2.0f, DFW::DebugDrawSettings(DFW::ColourRGBA::Red));
            
            // Record & Draw Projectile Trajector.
            static DFW::TimeTracker record_timer(true);
            if (record_timer.FetchElapsedTime() > DFW::TimeUnit(0.125f))
            {
                JPH::RVec3 ball_pos = ps.JoltBodyInterface().GetPosition(ball);
                recorded_projectile_trajectory_positions.emplace_back(DFW::DUtility::JPHToGLM(ball_pos));
                record_timer.ResetTimer(true);
            }
            
            for (auto& record_pos : recorded_projectile_trajectory_positions)
            {
                drs.DrawSphere(DFW::Transform(record_pos), 0.25f, DFW::DebugDrawSettings(DFW::ColourRGBA::White));
            }
            
            // Draw Ground.
            drs.DrawGrid(glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 128, 1, DFW::ColourRGBA::Grey);

            // DFW_INFOLOG("Pos: {}, Velocity: {} - Time: {}", 
            //           DFW::DUtility::JPHToGLM(ps.JoltBodyInterface().GetWorldTransform(ball).GetTranslation())
            //         , DFW::DUtility::JPHToGLM(ps.JoltBodyInterface().GetLinearVelocity(ball))
            //         , DFW::CoreService::GetGameClock()->GetElapsedTimeInSeconds()
            // );
            
            current_state = GameState::SimulateUpdate;
            has_processed_state_this_frame = true;
        }

        DFW::CoreService::GetGameClock()->Debug_LogInfo();

        _ecs->UpdateECS();

        _element_container.UpdateElements();
    }

    void TennisGame::OnRenderImGui()
    {
            DFW_ASSERT(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable);

            ImGuiViewport const* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGuiWindowFlags const dockspace_window_flags =
                ImGuiWindowFlags_MenuBar
                | ImGuiWindowFlags_NoDocking
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoBringToFrontOnFocus
                | ImGuiWindowFlags_NoNavFocus
                | ImGuiWindowFlags_NoBackground;

            { // Dockspace Window.
                ImGui::Begin("Window-DockSpace", nullptr, dockspace_window_flags);
                ImGui::PopStyleVar();
                ImGui::PopStyleVar(2);

                SetupDockingSpace();

                ImGui::End();
            }

            // Sub Elements
            _element_container.DisplayElements();

            _ecs->UpdateECSImGui();
    }

    void TennisGame::OnAttached()
    {
        SetupECS();

        //Debug_CreateXYZAxisOrigin();

        // Camera
        {
            _camera_entity = _ecs->Registry().CreateEntity();
            _camera_entity.SetName("camera-main");
            _camera_entity.SetType<"Camera">();

            DFW::Transform& camera_transform =_camera_entity.AddComponent<DFW::TransformComponent>();     
            camera_transform.SetTranslation(glm::vec3(30.f, 40.0f, -100.f));

            DFW::CameraComponent& camera_component = _camera_entity.AddComponent<DFW::CameraComponent>();
            DFW::CameraSystem* camera_system = _ecs->SystemManager().GetSystem<DFW::CameraSystem>();
            camera_system->ChangeCameraProjPerspective(camera_component, 90.f, (16.f/9.f), DFW::ClipSpace(0.1f, 10000.f));
            camera_system->RegisterCamera(camera_component, "camera-main");
            camera_system->SetActiveCamera("camera-main");
            camera_system->EnableSimpleCameraControlMode();
            camera_component.angles.x = -40.f;
            camera_component.angles.y = -45.f;
        }

        // Add Editor Elements.
        _element_container.AddEditorElement<DFW::DEditor::EditorElementFiller>("Toolbar");
        DFW::DEditor::GameViewport& game_viewport = _element_container.AddEditorElement<DFW::DEditor::GameViewport>("Viewport");

        // Set Render Targets of render systems.
        DFW::SharedPtr<DFW::DRender::RenderTarget const> viewport_render_target = game_viewport.GetViewportRenderTarget();
        auto rs = _ecs->SystemManager().GetSystem<DFW::RenderSystem>();
        rs->RenderToRenderTarget(viewport_render_target);
        auto drs = _ecs->SystemManager().GetSystem<DFW::DebugRenderSystem>();
        drs->RenderToRenderTarget(viewport_render_target);

    }

    void TennisGame::OnRemoved()
    {
        _ecs->Terminate();

        _element_container.ReleaseEditorElements();
    }

    void TennisGame::SetupECS()
    {   
        // Init Systems
        _ecs = DFW::MakeUnique<DFW::DECS::ECSModule>();
        _ecs->Init();

        _ecs->SystemManager().AddSystem<DFW::TransformSystem>().ExecuteAfter<DFW::PhysicsSystem>();
        _ecs->SystemManager().AddSystem<DFW::CameraSystem>().ExecuteAfter<DFW::TransformSystem>();
        _ecs->SystemManager().AddSystem<DFW::RenderSystem>().ExecuteAfter<DFW::CameraSystem>();
        _ecs->SystemManager().AddSystem<DFW::DebugRenderSystem>().ExecuteAfter<DFW::CameraSystem>();
        _ecs->SystemManager().AddSystem<DFW::PhysicsSystem>();

        _ecs->SystemManager().CalculateSystemDependencies();

        // Physics Debug Draw Settings
        DFW::PhysicsSystem& ps = *_ecs->SystemManager().GetSystem<DFW::PhysicsSystem>();
        ps.Debug_EnableDebugDraw();
        ps.debug_draw_settings.mDrawShapeWireframe = true;
        ps.debug_draw_settings.mDrawShape = true;
        ps.debug_draw_settings.mDrawBoundingBox = true;
        ps.debug_draw_settings.mDrawVelocity = true;
        ps.debug_draw_settings.mDrawCenterOfMassTransform = true;
    }

    void TennisGame::SetupDockingSpace()
    {
                    // DockSpace
            ImGuiID dockspace_id = ImGui::GetID("Main-DockSpace");
            ImGuiDockNodeFlags const dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            if (!_has_dockspace_been_created)
            {
                _has_dockspace_been_created = true;

                ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

                // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
                //  window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
                // out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
                // splitting these nodes should be/are done in correct order
                ImGuiID const dock_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.05f, nullptr, &dockspace_id);
                ImGuiID const dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.15f, nullptr, &dockspace_id);
                ImGuiID const dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
                ImGuiID const dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, nullptr, &dockspace_id);

                ImGuiID const dock_id_up = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.05f, nullptr, &dockspace_id);
                ImGuiID const dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

                ImGuiID const dock_id_center = ImGui::DockBuilderGetCentralNode(dockspace_id)->ID;

                // we now dock our windows into the docking node we made above
                ImGui::DockBuilderDockWindow(/*"Top"*/"Toolbar", dock_id_top);
                ImGui::DockBuilderDockWindow(/*"Bottom"*/"Console", dock_id_bottom);

                ImGui::DockBuilderDockWindow("Left", dock_id_left);
                ImGui::DockBuilderDockWindow(/*"Right"*/"Viewer", dock_id_right);

                ImGui::DockBuilderDockWindow("Up", dock_id_up);
                ImGui::DockBuilderDockWindow("Down", dock_id_down);

                ImGui::DockBuilderDockWindow("Viewport", dock_id_center);

                ImGui::DockBuilderFinish(dockspace_id);
            }
    }

    void TennisGame::Debug_CreateXYZAxisOrigin()
    {
        _debug_xyz = _ecs->Registry().CreateEntity();
        _debug_xyz.SetName("xyz-axis");

        _debug_xyz.AddComponent<DFW::TransformComponent>(DFW::Transform(glm::vec3(0.0f, 0.0f, 0.0f)));

        DFW::ModelComponent& xyz_model = _debug_xyz.AddComponent<DFW::ModelComponent>();
        DFW::FilePath filepath(std::string("models") + DIR_SLASH + "xyz" + DIR_SLASH + "xyz_1x1.glb");
        xyz_model.mesh = DFW::DResource::LoadMesh(filepath);
    }

} // End of namespace ~ Game.

