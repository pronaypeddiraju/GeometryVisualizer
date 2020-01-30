//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Commons/EngineCommon.hpp"

//------------------------------------------------------------------------------------------------------------------------------
class Game;
//------------------------------------------------------------------------------------------------------------------------------

class App
{
public:
	App();
	~App();
	
	static bool			Command_Quit(EventArgs& args);

	void				LoadGameBlackBoard();
	void				StartUp();
	void				ShutDown();
	void				RunFrame();

	bool				IsQuitting() const { return m_isQuitting; }
	bool				HandleKeyPressed( unsigned char keyCode );
	bool				HandleKeyReleased( unsigned char keyCode );
	bool				HandleCharacter( unsigned char charCode);
	
	bool				HandleMouseLBDown();
	bool				HandleMouseLBUp();
	bool				HandleMouseRBDown();
	bool				HandleMouseRBUp();
	bool				HandleMouseScroll(float wheelDelta);

	inline float		GetMouseScroll() { return m_scrollValue; }
	bool				HandleQuitRequested();

private:
	void				BeginFrame();
	void				Update();
	void				Render() const;
	void				PostRender();
	void				EndFrame();

private:
	bool				m_isQuitting = false;
	bool				m_isPaused = false;
	bool				m_isSlowMo = false;

	Game*				m_game = nullptr;
	
	double				m_timeAtLastFrameBegin = 0;
	double				m_timeAtThisFrameBegin = 0;

	float				m_scrollValue = 0.f;

};