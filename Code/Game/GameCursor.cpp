//------------------------------------------------------------------------------------------------------------------------------
#include "Game/GameCursor.hpp"
//Engine Systems
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/WindowContext.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/RenderContext.hpp"
//Game Systems
#include "Game/Game.hpp"

//------------------------------------------------------------------------------------------------------------------------------
GameCursor::GameCursor()
{
	StartUp();
}

//------------------------------------------------------------------------------------------------------------------------------
GameCursor::~GameCursor()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::StartUp()
{
	m_cursorPosition = Vec2(WORLD_CENTER_X, WORLD_CENTER_Y);
	m_squirrelFont = g_renderContext->CreateOrGetBitmapFontFromFile("SquirrelFixedFont");
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::Update( float deltaTime )
{
	UNUSED(deltaTime);

	//New code to implement cursor at mouse position;
	IntVec2 intVecPos = g_windowContext->GetClientMousePosition();
	IntVec2 clientBounds = g_windowContext->GetClientBounds();

	Vec2 worldPos = Game::GetClientToWorldPosition2D(intVecPos, clientBounds);

	m_cursorPosition = worldPos;
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::Render() const
{
	std::vector<Vertex_PCU> ringVerts;
	AddVertsForRing2D(ringVerts, m_cursorPosition, m_cursorRingRadius, m_cursorThickness, m_cursorColor);

	std::vector<Vertex_PCU>	lineVerts;
	Vec2 vertLineOffset = Vec2(0.f, m_cursorRingRadius);
	Vec2 horLineOffset = Vec2(m_cursorRingRadius, 0.f);

	AddVertsForLine2D(lineVerts, m_cursorPosition - vertLineOffset - Vec2(0.f, 0.5f), m_cursorPosition + vertLineOffset + Vec2(0.f, 0.5f), m_cursorThickness, m_cursorColor);
	AddVertsForLine2D(lineVerts, m_cursorPosition - horLineOffset - Vec2(0.5f, 0.f), m_cursorPosition + horLineOffset + Vec2(0.5f, 0.f), m_cursorThickness, m_cursorColor);

	//g_renderContext->BindTexture(nullptr);
	g_renderContext->BindTextureViewWithSampler(0U, nullptr);
	g_renderContext->DrawVertexArray(lineVerts);
	g_renderContext->DrawVertexArray(ringVerts);

	if (m_enableDebug)
	{
		DebugRenderCursor();
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::DebugRenderCursor() const
{
	//Draw any debug information you want here
	std::vector<Vertex_PCU> fontVerts;
	AABB2 textBox = AABB2(m_cursorPosition + Vec2(-50.f, 0.f), m_cursorPosition + Vec2(50.f, 10.f));
	std::string displayText = Stringf("X: %f , Y: %f", m_cursorPosition.x, m_cursorPosition.y);
	m_squirrelFont->AddVertsForTextInBox2D(fontVerts, textBox, 10.f, displayText);

	g_renderContext->BindTextureViewWithSampler(0U, m_squirrelFont->GetTexture());
	g_renderContext->DrawVertexArray(fontVerts);
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::HandleKeyPressed( unsigned char keyCode )
{
	switch( keyCode )
	{
		case UP_ARROW:
		m_movementVector.y += 1.f;
		break;
		case DOWN_ARROW:
		m_movementVector.y -= 1.f;
		break;
		case RIGHT_ARROW:
		m_movementVector.x += 1.f;
		break;
		case LEFT_ARROW:
		m_movementVector.x -= 1.f;
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::HandleKeyReleased( unsigned char keyCode )
{
	switch( keyCode )
	{
		case UP_ARROW:
		m_movementVector.y = 0.f;
		break;
		case DOWN_ARROW:
		m_movementVector.y = 0.f;
		break;
		case RIGHT_ARROW:
		m_movementVector.x = 0.f;
		break;
		case LEFT_ARROW:
		m_movementVector.x = 0.f;
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::SetCursorPosition( const Vec2& position )
{
	m_cursorPosition = position;
}

//------------------------------------------------------------------------------------------------------------------------------
const Vec2& GameCursor::GetCursorPositon() const
{
	return m_cursorPosition;
}

//------------------------------------------------------------------------------------------------------------------------------
void GameCursor::SetDebugMode(bool debugMode)
{
	m_enableDebug = debugMode;
}

