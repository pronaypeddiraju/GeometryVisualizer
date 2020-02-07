//------------------------------------------------------------------------------------------------------------------------------
#pragma once
//Engine systems
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/XMLUtils/XMLUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Math/Ray2D.hpp"
#include "Engine/Math/Vertex_PCU.hpp"

//Game systems
#include "Game/GameCommon.hpp"
#include "Game/Geometry.hpp"
#include "Game/BitBucketBroadPhase.hpp"

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
	void					DebugEnabled();

	void					HandleKeyPressed( unsigned char keyCode );
	void					HandleKeyReleased( unsigned char keyCode );
	void					HandleCharacter( unsigned char charCode );

	bool					HandleMouseLBDown();
	bool					HandleMouseLBUp();
	bool					HandleMouseRBDown();
	bool					HandleMouseRBUp();
	//bool					HandleMouseScroll(float wheelDelta);
	
	void					Render() const;
	ConvexPoly2D			MakeConvexPoly2DFromDisc(const Vec2& center, float radius) const;

	void					PostRender();

	void					Update( float deltaTime );
	void					ClearGarbageEntities();

	bool					IsAlive();

	static Vec2				GetClientToWorldPosition2D(IntVec2 mousePosInClient, IntVec2 ClientBounds);

private:
	void					CreateConvexPolygons(int numPolygons);
	void					CreateRaycasts(int numRaycasts);
	void					CreateRenderRay();
	
	void					UpdateGeometry(float deltaTime);
	void					UpdateCamera( float deltaTime );
	void					UpdateCameraMovement(unsigned char keyCode);
	void					UpdateImGUI();
	void					UpdateVisualRay();

	//Check Rays vs ConvexHulls
	void					CheckRenderRayVsConvexHulls();
	void					CheckAllRayCastsVsConvexHulls();
	void					CheckRaycastsBroadPhase();

	void					RenderWorldBounds() const;
	void					RenderOnScreenInfo() const;
	void					RenderPersistantUI() const;
	void					RenderAllGeometry() const;
	void					RenderRaycast() const;

	void					DebugRenderTestRandomPointsOnScreen() const;
	void					DebugRenderToScreen() const;
	void					DebugRenderToCamera() const;

	void					ReRandomize();
private:

	bool					m_isGameAlive = false;
	bool					m_consoleDebugOnce = false;
	bool					m_toggleBroadPhaseMode = true;

public:

	BitmapFont*				m_squirrelFont = nullptr;
	GameCursor*				m_gameCursor = nullptr;

	Camera*					m_mainCamera = nullptr;
	Camera*					m_devConsoleCamera = nullptr;

	Shader*					m_shader = nullptr;
	std::string				m_xmlShaderPath = "default_unlit.xml";

	Vec2					m_mouseStart = Vec2::ZERO;
	Vec2					m_mouseEnd = Vec2::ZERO;

	AABB2					m_worldBounds;

	bool					m_toggleUI = false;

	bool					m_canMoveStart = false;
	bool					m_canMoveEnd = false;

	//Debug Render Variable
	bool					m_isDebugSetup = false;
	Vec2					m_debugOffset = Vec2(20.f, 20.f);
	float					m_debugFontHeight = 2.f;

	//ImGUI Widget Variables
	float ui_cameraClearColor[3] = { 1.f, 1.f, 1.f };
	float ui_polygonColor[3] = {0.5f, 0.5f, 0.5f};

	int ui_numPolygons = INIT_NUM_POLYGONS;
	int ui_minPolygons = 1;
	int ui_maxPolygons = 1000;

	int ui_numRays = INIT_NUM_RAYCASTS;
	int ui_minRays = 1;
	int ui_maxRays = 4096;

	//Convex Geometry created in Game
	std::vector<ConvexPoly2D>	m_convexPolys;
	int							m_numPolygonsLastFrame;
	//Rgba						m_polygonColor = Rgba::ORGANIC_GREY;

	std::vector<ConvexHull2D>	m_convexHulls;
	int							m_numHullsLastFrame;

	//Raycasts in the scene
	std::vector<Ray2D>			m_rays;
	std::vector<RayHit2D>		m_hits;
	int							m_numRaysLastFrame;

	bool						m_isHitting = false;
	Ray2D						m_renderedRay;
	Vec2						m_rayStart;
	Vec2						m_rayEnd;

	Vec2						m_drawRayStart;
	Vec2						m_drawRayEnd;

	Vec2						m_drawSurfanceNormal;
	float						m_surfaceNormalLength = 5.f;

	//Broad Phase Optimization
	BitFieldBroadPhase			m_broadPhaseChecker;
	float						m_cachedRaycastTime;
};
