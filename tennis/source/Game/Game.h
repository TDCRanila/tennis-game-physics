#pragma once

#include <DFW/CoreSystems/Stage/Stage.h>

#include <DFW/Modules/ECS/ECSModule.h>
#include <DFW/Modules/ECS/Entity.h>

namespace Game
{
    class TennisGame final : public DFW::StageBase
    {
    public:
        TennisGame(std::string const& a_stage_name, bool a_start_disabled = false);
        virtual ~TennisGame() = default;

        virtual void OnUpdate() override;

        virtual void OnAttached() override;
        virtual void OnRemoved() override;

    private:
        DFW::Entity _camera_entity;

        void SetupECS();
        DFW::UniquePtr<DFW::DECS::ECSModule> _ecs;

    private:
        void Debug_CreateXYZAxisOrigin();
        DFW::Entity _debug_xyz;

    };

} // End of namespace ~ Game.
