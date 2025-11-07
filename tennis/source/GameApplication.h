#pragma once

#include <DFW/CoreSystems/ApplicationInstance.h>

namespace Game
{

	class GameApplication final : public DFW::ApplicationInstance
	{
	public:
		GameApplication() = default;
		~GameApplication() = default;

	private:
		virtual void PreApplicationInit() override;
		virtual void PostApplicationInit() override;

	};

} // End of namespace ~ Game.
