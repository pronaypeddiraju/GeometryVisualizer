//------------------------------------------------------------------------------------------------------------------------------
#pragma once
//Engine Systems
#include "Engine/Renderer/Rgba.hpp"

//Game Systems
#include "Game/GameCommon.hpp"



class GameCursor
{
public:
	GameCursor();
	~GameCursor();

	void			StartUp();

	void			Update(float deltaTime);
	void			Render() const; 

	void			HandleKeyPressed( unsigned char keyCode );
	void			HandleKeyReleased( unsigned char keyCode );

	void			SetCursorPosition(const Vec2& position);

	const Vec2&     GetCursorPositon() const;

private:
	Vec2			m_cursorPosition = Vec2::ZERO;
	Rgba			m_cursorColor = Rgba::WHITE;
	float			m_cursorThickness = 0.25f;
	float			m_cursorRingRadius = 2.0f;

	//Movement Data
	Vec2			m_movementVector = Vec2::ZERO;
	float			m_cursorSpeed = 0.25f;
};