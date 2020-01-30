//------------------------------------------------------------------------------------------------------------------------------
#pragma once
//Engine systems
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/XMLUtils/XMLUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vertex_PCU.hpp"

//Game systems
#include "Game/GameCommon.hpp"
#include "Game/Geometry.hpp"

//------------------------------------------------------------------------------------------------------------------------------
class Texture;
class BitmapFont;
class SpriteAnimDefenition;
class Image;
class GameCursor;
class Geometry;
class Shader;
class Trigger2D;
struct Camera;
struct IntVec2;

//------------------------------------------------------------------------------------------------------------------------------
class Game
{

public:

	Game();
	~Game();

	void					StartUp();
	void					ShutDown();
	int						GetNextValidGeometryIndex(int index);
	void					DebugEnabled();

	void					HandleKeyPressed( unsigned char keyCode );
	void					HandleKeyReleased( unsigned char keyCode );
	void					HandleCharacter( unsigned char charCode );

	bool					HandleMouseLBDown();
	bool					HandleMouseLBUp();
	bool					HandleMouseRBDown();
	bool					HandleMouseRBUp();
	bool					HandleMouseScroll(float wheelDelta);

	void					Render() const;
	void					RenderWorldBounds() const;
	void					RenderOnScreenInfo() const;
	void					RenderPersistantUI() const;
	void					RenderAllGeometry() const;
	void					RenderDebugObjectInfo() const;

	void					DebugRenderToScreen() const;
	void					DebugRenderToCamera() const;

	void					PostRender();

	void					Update( float deltaTime );
	void					UpdateGeometry( float deltaTime );
	void					UpdateCamera( float deltaTime );
	void					UpdateCameraMovement(unsigned char keyCode);

	void					ClearGarbageEntities();
	void					CheckCollisions();

	bool					IsAlive();

	static Vec2				GetClientToWorldPosition2D(IntVec2 mousePosInClient, IntVec2 ClientBounds);

	void					ToggleSimType();
	void					ChangeCurrentGeometry();

	// XML File save and load methods
	void					SaveToFile(const std::string& filePath);
	void					LoadFromFile(const std::string& filePath);

private:

	bool					m_isGameAlive = false;
	bool					m_consoleDebugOnce = false;

public:

	BitmapFont*				m_squirrelFont = nullptr;
	GameCursor*				m_gameCursor = nullptr;
	std::vector<Geometry*>  m_allGeometry;
	Geometry*				m_selectedGeometry = nullptr;
	int						m_selectedIndex;
	float					m_fontHeight = 2.5f;

	float					m_objectMass = 1.0f;
	float					m_objectRestitution = 0.5f;

	float					m_objectFriction = 0.5f;
	float					m_frictionStep = 0.1f;

	float					m_objectLinearDrag = 0.0f;
	float					m_linearDragStep = 0.1f;
	
	float					m_objectAngularDrag = 0.0f;
	float					m_angularDragStep = 0.1f;

	float					m_massStep = 0.1f;
	float					m_restitutionStep = 0.1f;

	bool					m_xFreedom = true;
	bool					m_yFreedom = true;
	bool					m_rotationFreedom = true;

	Camera*					m_mainCamera = nullptr;
	Camera*					m_devConsoleCamera = nullptr;

	Shader*					m_shader = nullptr;
	std::string				m_xmlShaderPath = "default_unlit.xml";

	eGeometryType			m_geometryType = BOX_GEOMETRY;
	bool					m_isStatic = false;

	Vec2					m_mouseStart = Vec2::ZERO;
	Vec2					m_mouseEnd = Vec2::ZERO;

	float					m_zoomLevel = 0.0f;
	float					m_zoomMultiplier = 10.f;

	AABB2					m_worldBounds;

	bool					m_toggleUI = false;

	//Debug Render Variable
	bool					m_isDebugSetup = false;
	Vec2					m_debugOffset = Vec2(20.f, 20.f);
	float					m_debugFontHeight = 2.f;

	Trigger2D*				m_boxTrigger = nullptr;
	Trigger2D*				m_capsuleTrigger = nullptr;
};
