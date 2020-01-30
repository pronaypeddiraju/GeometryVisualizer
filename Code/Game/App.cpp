//------------------------------------------------------------------------------------------------------------------------------
#include "Game/App.hpp"
//Engine Systems
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystems.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/XMLUtils/XMLUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/PhysicsSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/RenderContext.hpp"
//Game Systems
#include "Game/Game.hpp"

//Globals
App* g_theApp = nullptr;
Clock* g_gameClock = nullptr;
Clock* g_devConsoleClock = nullptr;

//------------------------------------------------------------------------------------------------------------------------------
App::App()
{	
	g_gameClock = new Clock(nullptr);
	g_devConsoleClock = new Clock(nullptr);
}

App::~App()
{
	ShutDown();
}

STATIC bool App::Command_Quit(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return true;
}

void App::LoadGameBlackBoard()
{
	const char* xmlDocPath = "Data/Gameplay/GameConfig.xml";
	tinyxml2::XMLDocument gameconfig;
	gameconfig.LoadFile(xmlDocPath);
	
	if(gameconfig.ErrorID() != tinyxml2::XML_SUCCESS)
	{
		printf("\n >> Error loading XML file from %s ", xmlDocPath);
		printf("\n >> Error ID : %i ", gameconfig.ErrorID());
		printf("\n >> Error line number is : %i", gameconfig.ErrorLineNum());
		printf("\n >> Error name : %s", gameconfig.ErrorName());
		ERROR_AND_DIE(">> Error loading GameConfig XML file ")
		return;
	}
	else
	{
		//We read everything fine. Now just shove all that data into the black board
		XMLElement* rootElement = gameconfig.RootElement();
		g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);

	}

}

void App::StartUp()
{
	//Load initial black board here
	//LoadGameBlackBoard();

	//Create event system
	g_eventSystem = new EventSystems();

	//Create the Render Context
	//g_renderContext = new RenderContext();
	//g_renderContext->Startup();
	
	//Create the Debug Render System
	g_debugRenderer = new DebugRender();
	g_debugRenderer->Startup(g_renderContext);

	//Create the Input System
	g_inputSystem = new InputSystem();

	//Create the Audio System
	g_audio = new AudioSystem();

	//Create dev console
	g_devConsole = new DevConsole();
	g_devConsole->Startup();

	//Create physics system
	g_physicsSystem = new PhysicsSystem();
	g_physicsSystem->SetGravity(Vec2(0.f, -9.8f));

	//create the networking system
	//g_networkSystem = new NetworkSystem();

	//Create the game here
	m_game = new Game();
	m_game->StartUp();
	
	g_eventSystem->SubscribeEventCallBackFn("Quit", Command_Quit);
}

void App::ShutDown()
{
	m_game->ShutDown();
	delete m_game;

	delete g_renderContext;
	g_renderContext = nullptr;

	delete g_debugRenderer;
	g_debugRenderer = nullptr;

	delete g_inputSystem;
	g_inputSystem = nullptr;

	delete g_audio;
	g_audio = nullptr;

	delete g_devConsole;
	g_devConsole = nullptr;

	delete g_eventSystem;
	g_eventSystem = nullptr;
}

void App::RunFrame()
{
	
	BeginFrame();	
	
	Update();
	Render();	

	PostRender();

	EndFrame();
}


void App::EndFrame()
{
	g_renderContext->EndFrame();
	g_debugRenderer->EndFrame();
	g_inputSystem->EndFrame();
	g_audio->EndFrame();
	g_eventSystem->EndFrame();
	g_devConsole->EndFrame();
}

void App::BeginFrame()
{
	g_renderContext->BeginFrame();
	g_debugRenderer->BeginFrame();
	g_inputSystem->BeginFrame();
	g_audio->BeginFrame();
	g_eventSystem->BeginFrame();
	g_devConsole->BeginFrame();
}

void App::Update()
{	
	m_timeAtLastFrameBegin = m_timeAtThisFrameBegin;
	m_timeAtThisFrameBegin = GetCurrentTimeSeconds();
	float deltaTime = static_cast<float>(m_timeAtThisFrameBegin - m_timeAtLastFrameBegin);
	
	deltaTime = Clamp(deltaTime, 0.0f, 0.1f);
	g_gameClock->Step(deltaTime);
	g_devConsoleClock->Step(deltaTime);

	g_devConsole->UpdateConsole((float)g_devConsoleClock->GetFrameTime());

	deltaTime = (float)g_gameClock->GetFrameTime();
	if(deltaTime == 0.f)
	{
		return;
	}

	g_debugRenderer->Update(deltaTime);
	m_game->Update(deltaTime);

}

void App::Render() const
{
	m_game->Render();

	if (g_devConsole->IsOpen())
	{
		g_renderContext->BindShader(m_game->m_shader);
		g_renderContext->BindTextureViewWithSampler(0U, m_game->m_squirrelFont->GetTexture());
		g_renderContext->SetModelMatrix(Matrix44::IDENTITY);
		g_devConsole->Render(*g_renderContext, *m_game->m_devConsoleCamera, DEVCONSOLE_LINE_HEIGHT);
	}
}

void App::PostRender()
{
	m_game->PostRender();
}

bool App::HandleKeyPressed(unsigned char keyCode)
{
	if (keyCode == TILDY_KEY)
	{
		g_devConsole->ToggleOpenFull();
	}

	if (g_devConsole->IsOpen())
	{
		g_devConsole->HandleKeyDown(keyCode);
		return true;
	}

	switch(keyCode)
	{
		case Z_KEY:
		{
			g_gameClock->ForceResume();
		}
		break;
		case  X_KEY:
		{
			g_gameClock->Pause();
		}
		break;
		case C_KEY:
		{
			g_gameClock->Dilate(0.1);
		}
		break;
		case V_KEY:
		{
			g_gameClock->Dilate(2);
		}
		break;
		case B_KEY:
		{
			g_gameClock->ForceStep(0.1f);
		}
		break;
		case UP_ARROW:
		case RIGHT_ARROW:
		case LEFT_ARROW:
		case DOWN_ARROW:	
		case W_KEY:
		case A_KEY:
		case S_KEY:
		case D_KEY:
		case F1_KEY:
		case F2_KEY:
		case F3_KEY:
		case F4_KEY:
		case F5_KEY:
		case F6_KEY:
		case F7_KEY:
		case SPACE_KEY:
		case DEL_KEY:
		case TAB_KEY:
		case KEY_LESSER:
		case ENTER_KEY:
		case BACK_SPACE:
		case KEY_GREATER:
		case N_KEY:
		case M_KEY:
		case G_KEY:
		case K_KEY:
		case L_KEY:
		case NUM_1:
		case NUM_2:
		case NUM_3:
		case NUM_4:
		case NUM_5:
		case NUM_6:
		case NUM_7:
		case NUM_8:
		case NUM_9:
		case NUM_0:
		case LCTRL_KEY:
			m_game->HandleKeyPressed(keyCode);
			return true;
		break;
		case F8_KEY:
		//Kill and restart the app
		delete m_game;
		m_game = nullptr;
		m_game = new Game();
		m_game->StartUp();
		return true;
		break;
		case KEY_ESC:
		{
			if (!g_devConsole->IsOpen())
			{
				//Shut the app
				g_theApp->HandleQuitRequested();
				return true;
			}
			else
			{
				m_game->HandleKeyPressed(keyCode);
				return true;
			}
		}
		default:
		break;
	}
	return false;
}

bool App::HandleKeyReleased(unsigned char keyCode)
{
	switch(keyCode)
	{
		case 'T':
			//Implement code to return deltaTime to original value
			m_isSlowMo = false;
			return true;
		break;
		case  'P':
			//Implement code to un-pause game
			m_isPaused = false;
			return true;
		break;
		case UP_ARROW:
		case RIGHT_ARROW:
		case LEFT_ARROW:
		case DOWN_ARROW:
		case SPACE_KEY:
			m_game->HandleKeyReleased(keyCode);
			return true;
		break;

		default:
		//Nothing to worry about
		return false;
		break;
	}


}

bool App::HandleCharacter( unsigned char charCode )
{
	UNUSED(charCode);
	m_game->HandleCharacter(charCode);
	return false;
}

bool App::HandleMouseLBDown()
{
	//Implement Mouse Left button down logic here
	return m_game->HandleMouseLBDown();
}

bool App::HandleMouseLBUp()
{
	//Implement Mouse Left button Up logic here
	return m_game->HandleMouseLBUp();
}

bool App::HandleMouseRBDown()
{
	//Implement Mouse Right Button Down logic here
	return m_game->HandleMouseRBDown();
}

bool App::HandleMouseRBUp()
{
	//Implement Mouse Right Button Up logic here
	return m_game->HandleMouseRBUp();	
}

bool App::HandleMouseScroll(float wheelDelta)
{
	return m_game->HandleMouseScroll(wheelDelta);
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return m_isQuitting;
}