#include <GameApplication.h>

#include <Game/Game.h>

#include <DFW/CoreSystems/CoreServices.h>

#include <DFW/Modules/Editor/Console/MainConsole.h>
#include <DFW/Modules/Editor/MainMenuBar.h>


namespace Game
{
	void GameApplication::PreApplicationInit()
	{

	}

	void GameApplication::PostApplicationInit()
	{
		// Stages
		DFW::CoreService::GetAppStageController()->AttachStage<DFW::DEditor::MainMenuBar>("MainMenuBar", false);
		//DFW::CoreService::GetAppStageController()->AttachStage<DFW::DEditor::MainConsole>("Console", false);

		DFW::CoreService::GetAppStageController()->AttachStage<Game::TennisGame>("Game", false);

	}

} // End of namespace ~ Game.
