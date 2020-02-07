//------------------------------------------------------------------------------------------------------------------------------
#include "Game/Game.hpp"
//Engine Systems
#include "Engine/Commons/ErrorWarningAssert.hpp"
#include "Engine/Commons/StringUtils.hpp"
#include "Engine/Commons/UnitTest.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystems.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/WindowContext.hpp"
#include "Engine/Math/Collider2D.hpp"
#include "Engine/Math/Disc2D.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/PhysicsSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RigidBodyBucket.hpp"
#include "Engine/Math/Trigger2D.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/ColorTargetView.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Shader.hpp"
#include <ThirdParty/TinyXML2/tinyxml2.h>

//Game systems
#include "Game/GameCursor.hpp"
#include "Engine/Math/Ray2D.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Commons/Profiler/ProfileLogScope.hpp"
#include "Engine/Commons/Profiler/Profiler.hpp"
#include "Engine/Commons/EngineCommon.hpp"

//Globals
float g_shakeAmount = 0.0f;
bool g_debugMode = false;

eSimulationType g_selectedSimType = STATIC_SIMULATION;

//Extern 
extern RenderContext* g_renderContext;
extern AudioSystem* g_audio;

//------------------------------------------------------------------------------------------------------------------------------
Game::Game()
{
	m_isGameAlive = true;
	m_squirrelFont = g_renderContext->CreateOrGetBitmapFontFromFile("SquirrelFixedFont");

	g_devConsole->SetBitmapFont(*m_squirrelFont);
	g_debugRenderer->SetDebugFont(m_squirrelFont);

	g_devConsole->PrintString(Rgba::BLUE, "this is a test string");
	//g_devConsole->PrintString(Rgba::RED, "this is also a test string");
	//g_devConsole->PrintString(Rgba::GREEN, "damn this dev console lit!");
	//g_devConsole->PrintString(Rgba::WHITE, "Last thing I printed");
}

//------------------------------------------------------------------------------------------------------------------------------
Game::~Game()
{
	m_isGameAlive = false;
	delete m_mainCamera;
	m_mainCamera = nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::StartUp()
{
	//Setup mouse startup values
	IntVec2 clientCenter = g_windowContext->GetClientCenter();
	g_windowContext->SetClientMousePosition(clientCenter);

	g_windowContext->SetMouseMode(MOUSE_MODE_ABSOLUTE);
	g_windowContext->HideMouse();

	m_gameCursor = new GameCursor();

	//Create the world bounds AABB2
	Vec2 minWorldBounds = Vec2(0.f, 0.f);
	Vec2 maxWorldBounds = Vec2(WORLD_WIDTH, WORLD_HEIGHT);
	m_worldBounds = AABB2(minWorldBounds, maxWorldBounds);

	//Create the Camera and setOrthoView
	m_mainCamera = new Camera();
	m_mainCamera->SetColorTarget(nullptr);

	//Create a devConsole Cam
	m_devConsoleCamera = new Camera();
	m_devConsoleCamera->SetColorTarget(nullptr);

	Vec2 orthoBottomLeft = Vec2(0.f,0.f);
	Vec2 orthoTopRight = Vec2(WORLD_WIDTH, WORLD_HEIGHT);
	m_mainCamera->SetOrthoView(orthoBottomLeft, orthoTopRight);

	//Get the Shader
	m_shader = g_renderContext->CreateOrGetShaderFromFile(m_xmlShaderPath);
	m_shader->SetDepth(eCompareOp::COMPARE_LEQUAL, true);

	//Setup the colors for UI
	ui_polygonColor[0] = Rgba::ORGANIC_GREEN.r;
	ui_polygonColor[1] = Rgba::ORGANIC_GREEN.g;
	ui_polygonColor[2] = Rgba::ORGANIC_GREEN.b;

	m_broadPhaseChecker.SetWorldDimensions(minWorldBounds, maxWorldBounds);
	m_broadPhaseChecker.MakeRegionsForWorld();

	UnitTestRunAllCategories(10);

	//Generate Random Convex Polygons to render on screen
	CreateConvexPolygons(INIT_NUM_POLYGONS);
	CreateRaycasts(INIT_NUM_RAYCASTS);

	//Setup the render ray
	CreateRenderRay();
}

//------------------------------------------------------------------------------------------------------------------------------
UNITTEST("RaycastTests", "MathUtils", 1)
{
	Ray2D ray(Vec2(0.f, 0.f), Vec2(1.f, 0.f));
	Plane2D plane(Vec2(1.f, 0.f), 10.f);
	float out[2];
	uint numHits = Raycast(out, ray, plane);

	if (numHits > 0)
	{
		DebuggerPrintf("\n Num hits plane: %d", numHits);
		DebuggerPrintf("\n First Hit: %f", out[0]);
	}

	Capsule2D capsule(Vec2(20.f, 0.f), 1.f);
	numHits = Raycast(out, ray, capsule);

	if (numHits > 0)
	{
		DebuggerPrintf("\n Num hits capsule: %d", numHits);
		DebuggerPrintf("\n First Hit: %f", out[0]);
	}

	std::vector<Vec2> points = { Vec2(60.f, 40.f), Vec2(40.f, 60.f), Vec2(20.f, 40.f), Vec2(40.f, 20.f)};
	ConvexPoly2D polygon(points);
	ConvexHull2D hull;
	hull.MakeConvexHullFromConvexPolyon(polygon);
	
	int numPlanes = hull.GetNumPlanes();
	RayHit2D *hullOut = new RayHit2D[numPlanes];
	
	ray.m_direction = Vec2(1.f, 1.f);
	ray.m_direction.Normalize();
	
	numHits = Raycast(hullOut, ray, hull);

	if (numHits > 0)
	{
		DebuggerPrintf("\n Num hits Hull: %d", numHits);
		DebuggerPrintf("\n First Hit: %f", hullOut->m_timeAtHit);
	}

	delete hullOut;

	g_LogSystem->Logf("\n PrintFilter", "I am a Logf call");
	g_LogSystem->Logf("\n FlushFilter", "I am now calling flush");
	g_LogSystem->LogFlush();
	return true;
}


//------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateImGUI()
{
	//Use this function to create and use an ImGUI widget
	ImGui::Begin("Geometry Visualizer");

	ImGui::ColorEdit3("Scene Background Color", (float*)&ui_cameraClearColor); // Edit 3 floats representing a color
	ImGui::ColorEdit3("Polygon Color", (float*)ui_polygonColor);

	ImGui::Text("Polygons :");
	ImGui::SameLine();
	if (ImGui::Button("Reduce by half"))
	{
		ui_numPolygons /= 2;
		CreateConvexPolygons(ui_numPolygons);
	}

	ImGui::SameLine();
	if (ImGui::Button("Double"))
	{
		ui_numPolygons *= 2;
		CreateConvexPolygons(ui_numPolygons);
	}

	ImGui::SliderInt("Number of Polygons", &ui_numPolygons, ui_minPolygons, ui_maxPolygons);
	
	ImGui::Text("Rays :");
	ImGui::SameLine();
	if (ImGui::Button("Half"))
	{
		ui_numRays /= 2;
		CreateRaycasts(ui_numRays);
	}

	ImGui::SameLine();
	if (ImGui::Button("2X"))
	{
		ui_numRays *= 2;
		CreateRaycasts(ui_numRays);
	}

	ImGui::SliderInt("Number of Rays", &ui_numRays, ui_minRays, ui_maxRays);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::Checkbox("Enable Bit Bucket Broad-phase Check", &m_toggleBroadPhaseMode);
	ImGui::Text("Total Raycast Time last frame in ms: %f", m_cachedRaycastTime * 1000.f);

	ImGui::End();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateVisualRay()
{
	if (m_canMoveStart)
	{
		m_rayStart = m_gameCursor->GetCursorPositon();
		m_renderedRay.m_start = m_gameCursor->GetCursorPositon();
	}

	if (m_canMoveEnd)
	{
		m_rayEnd = m_gameCursor->GetCursorPositon();
	}

	Vec2 rayDir = m_rayEnd - m_rayStart;
	rayDir.Normalize();

	m_renderedRay.m_direction = rayDir;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::ShutDown()
{
	delete m_mainCamera;
	m_mainCamera = nullptr;

	delete m_devConsoleCamera;
	m_devConsoleCamera = nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::HandleKeyPressed(unsigned char keyCode)
{
	if (g_devConsole->IsOpen())
	{
		g_devConsole->HandleKeyDown(keyCode);
		return;
	}

	switch( keyCode )
	{
		case W_KEY:
		case S_KEY:
		{
			m_canMoveStart = true;
			m_mouseStart = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
			m_renderedRay.m_start = m_mouseStart;
			break;
		}
		case A_KEY:
		case D_KEY:
		break;
		case E_KEY:
		{
			m_canMoveEnd = true;
			m_mouseEnd = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
			m_rayEnd = m_mouseEnd;
			break;
		}
		case LCTRL_KEY:
		{
			m_toggleUI = !m_toggleUI;
		}
		break;
		case DEL_KEY:
		case G_KEY:
		case TAB_KEY:
		case K_KEY:
		break;
		case L_KEY:
		break;
		case NUM_1:
		{
			ui_numRays /= 2;
			CreateRaycasts(ui_numRays);
		}
		break;
		case NUM_2:
		{
			ui_numRays *= 2;
			
			if (ui_numRays == 0)
				ui_numRays = 1;

			CreateRaycasts(ui_numRays);
		}
		break;
		case NUM_3:
		{
			ui_numPolygons /= 2;
			
			if (ui_numPolygons == 0)
				ui_numPolygons = 1;

			CreateConvexPolygons(ui_numPolygons);
		}
		break;
		case NUM_4:
		{
			ui_numPolygons *= 2;
			CreateConvexPolygons(ui_numPolygons);
		}
		break;
		case NUM_5:
		{
			m_toggleBroadPhaseMode = !m_toggleBroadPhaseMode;
		}
		break;
		case NUM_6:
		{
			ReRandomize();
		}
		break;
		case NUM_7:
		break;
		case NUM_8:
		break;
		case NUM_9:
		break;
		case N_KEY:
		break;
		case M_KEY:
		break;
		case KEY_LESSER:
		break;
		case KEY_GREATER:
		break;
		case F1_KEY:
		break;
		case F2_KEY:
		break;
		case F3_KEY:
		break;
		case F4_KEY:
		break;
		case F5_KEY:
		break;
		case F6_KEY:
		break;
		case F7_KEY:
		break;
		default:
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::DebugEnabled()
{
	g_debugMode = !g_debugMode;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::HandleKeyReleased(unsigned char keyCode)
{

	switch( keyCode )
	{
		case UP_ARROW:
		case DOWN_ARROW:
		case RIGHT_ARROW:
		case LEFT_ARROW:
		break;
		default:
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::HandleCharacter(unsigned char charCode)
{
	if (g_devConsole->IsOpen())
	{
		g_devConsole->HandleCharacter(charCode);
		return;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseLBDown()
{
	m_canMoveStart = true;
	m_mouseStart = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
	m_renderedRay.m_start = m_mouseStart;
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseLBUp()
{
	m_canMoveStart = false;
	m_mouseStart = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
	m_renderedRay.m_start = m_mouseStart;
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseRBDown()
{
	m_canMoveEnd = true;
	m_mouseEnd = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
	m_rayEnd = m_mouseEnd;
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseRBUp()
{
	m_canMoveEnd = false;
	m_mouseEnd = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
	m_rayEnd = m_mouseEnd;
	return true;
}

/*
//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseScroll(float wheelDelta)
{
	m_zoomLevel += wheelDelta;
	m_zoomLevel = Clamp(m_zoomLevel, 0.00001f, MAX_ZOOM_STEPS);

	//Recalculate mins and maxes based on the zoom level
	float newMinX = 0.f + m_zoomLevel * SCREEN_ASPECT;
	float newMinY = 0.f + m_zoomLevel;

	float newMaxX = WORLD_WIDTH - m_zoomLevel * SCREEN_ASPECT;
	float newMaxY = WORLD_HEIGHT - m_zoomLevel;

	m_mainCamera->SetOrthoView(Vec2(newMinX, newMinY), Vec2(newMaxX, newMaxY));

	return true;
}
*/

//------------------------------------------------------------------------------------------------------------------------------
void Game::Render() const
{
	//Get the ColorTargetView from rendercontext
	ColorTargetView *colorTargetView = g_renderContext->GetFrameColorTarget();

	//Setup what we are rendering to
	m_mainCamera->SetColorTarget(colorTargetView);
	m_devConsoleCamera->SetColorTarget(colorTargetView);

	g_renderContext->BeginCamera(*m_mainCamera);

	g_renderContext->BindTextureViewWithSampler(0U, nullptr);
		
	g_renderContext->ClearColorTargets(Rgba(ui_cameraClearColor[0], ui_cameraClearColor[1], ui_cameraClearColor[2], 1.f));

	g_renderContext->BindShader( m_shader );

	RenderAllGeometry();
	RenderRaycast();

	RenderWorldBounds();

	//RenderPersistantUI();

	if(m_toggleUI)
	{
		//RenderOnScreenInfo();
	}

	g_ImGUI->Render();

	m_gameCursor->Render();

	g_renderContext->BindTextureViewWithSampler(0U, nullptr);

	g_renderContext->EndCamera();

}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderWorldBounds() const
{
	std::vector<Vertex_PCU> boxVerts;
	AddVertsForBoundingBox(boxVerts, m_worldBounds, Rgba::DARK_GREY, 2.f);
	
	g_renderContext->DrawVertexArray(boxVerts);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderOnScreenInfo() const
{
	
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderPersistantUI() const
{
	
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderAllGeometry() const
{
	//Render all the geometry in the scene
	std::vector<Vertex_PCU> convexPolyVerts;

	for (int polygonIndex = 0; polygonIndex < m_convexPolys.size(); polygonIndex++)
	{
		AddVertsForSolidConvexPoly2D(convexPolyVerts, m_convexPolys[polygonIndex], Rgba(ui_polygonColor[0], ui_polygonColor[1], ui_polygonColor[2], 1.f));
	}

	g_renderContext->BindTextureViewWithSampler(0U, nullptr);
	g_renderContext->DrawVertexArray(convexPolyVerts);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::DebugRenderTestRandomPointsOnScreen() const
{
	//Debugging if the RandomPointsOnCircleAreCorrect
	float angleUsed;
	Vec2 randomPoint = GetRandomPointOnDisc2D(Vec2(50.f, 50.f), 50.f, 0.f, 360.f, angleUsed);
	g_debugRenderer->DebugRenderPoint2D(randomPoint, 1.f, 5.f);

	g_debugRenderer->DebugRenderDisc2D(Disc2D(Vec2(50.f, 50.f), 10.f), 0.f);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::DebugRenderToScreen() const
{
	Camera& debugCamera = g_debugRenderer->Get2DCamera();
	debugCamera.m_colorTargetView = g_renderContext->GetFrameColorTarget();

	g_renderContext->BindShader(m_shader);
	g_renderContext->BeginCamera(debugCamera);

	g_debugRenderer->DebugRenderToScreen();

	g_renderContext->EndCamera();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::DebugRenderToCamera() const
{
	Camera& debugCamera3D = *m_mainCamera;
	debugCamera3D.m_colorTargetView = g_renderContext->GetFrameColorTarget();

	g_renderContext->BindShader(m_shader);
	g_renderContext->BeginCamera(debugCamera3D);

	g_debugRenderer->Setup3DCamera(&debugCamera3D);
	g_debugRenderer->DebugRenderToCamera();

	g_renderContext->EndCamera();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::ReRandomize()
{
	m_rays.clear();
	m_convexHulls.clear();
	m_convexPolys.clear();

	CreateConvexPolygons(ui_numPolygons);
	CreateRaycasts(ui_numRays);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CheckRaycastsBroadPhase()
{
	double totalStartTime = GetCurrentTimeSeconds();

	for (int rayIndex = 0; rayIndex < m_rays.size(); rayIndex++)
	{
		for (int hullIndex = 0; hullIndex < m_convexHulls.size(); hullIndex++)
		{
			bool xOverlapCondition = (m_rays[rayIndex].m_bitFieldsXY.x & m_convexHulls[hullIndex].GetBitFields().x) != 0;
			bool yOverlapCondition = (m_rays[rayIndex].m_bitFieldsXY.y & m_convexHulls[hullIndex].GetBitFields().y) != 0;

			if (xOverlapCondition && yOverlapCondition)
			{
				double startTime = GetCurrentTimeSeconds();
				//Run the regular collision check for ray vs convexHull here
				uint hits = 0;
				hits = Raycast(&m_hits[rayIndex], m_renderedRay, m_convexHulls[hullIndex], 0.f);
				double endTime = GetCurrentTimeSeconds();

				if (hits > 0 && IsPointOnLineSegment2D(m_hits[rayIndex].m_hitPoint, m_rayStart, m_rayEnd))
				{
					//DebuggerPrintf("\n Ray Number %d hit convexHull Number %d at position: ( %f, %f ) Time Taken: %.3f", rayIndex, hullIndex, m_hits[rayIndex].m_hitPoint.x, m_hits[rayIndex].m_hitPoint.y, (float)(endTime - startTime));
				}
				else
				{
					//DebuggerPrintf("\n Ray Number %d had no hit Time taken: %.3f", rayIndex, (float)(endTime - startTime));
				}
			}
		}
	}

	double totalEndTime = GetCurrentTimeSeconds();
	m_cachedRaycastTime = totalEndTime - totalStartTime;
	DebuggerPrintf("\n Total Time for Raycasts this frame: %f", m_cachedRaycastTime);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CheckRenderRayVsConvexHulls()
{
	float bestHit = 9999;

	//Check the render ray vs all the convex hulls in the scene
	for (int hullIndex = 0; hullIndex < m_convexHulls.size(); hullIndex++)
	{
		RayHit2D* out = new RayHit2D[m_convexHulls[hullIndex].GetNumPlanes()];
		uint hits = Raycast(out, m_renderedRay, m_convexHulls[hullIndex], 0.f);

		if (hits > 0 && IsPointOnLineSegment2D(out->m_hitPoint, m_rayStart, m_rayEnd) && out->m_timeAtHit < bestHit)
		{
			m_isHitting = true;
			DebuggerPrintf("\n Ray hit convexHull");
			bestHit = out->m_timeAtHit;
			m_drawRayStart = m_rayStart;
			m_drawRayEnd = out->m_hitPoint;
			m_drawSurfanceNormal = out->m_impactNormal;
		}
		else if(hits == 0 && bestHit == 9999)
		{
			m_isHitting = false;
			m_drawRayStart = m_rayStart;
			m_drawRayEnd = Vec2::ZERO;
			m_drawSurfanceNormal = Vec2::ZERO;
		}

		delete out;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CheckAllRayCastsVsConvexHulls()
{
	double totalStartTime = GetCurrentTimeSeconds();

	gProfiler->ProfilerPush("Ray vs Convex");

	for (int rayIndex = 0; rayIndex < m_rays.size(); rayIndex++)
	{
		for (int hullIndex = 0; hullIndex < m_convexHulls.size(); hullIndex++)
		{
			uint hits = 0;
			hits = Raycast(&m_hits[rayIndex], m_renderedRay, m_convexHulls[hullIndex], 0.f);

			if (hits > 0 && IsPointOnLineSegment2D(m_hits[rayIndex].m_hitPoint, m_rayStart, m_rayEnd))
			{
				//DebuggerPrintf("\n Ray Number %d hit convexHull Number %d at position: ( %f, %f )", rayIndex, hullIndex, m_hits[rayIndex].m_hitPoint.x, m_hits[rayIndex].m_hitPoint.y);
			}
			else
			{
				//DebuggerPrintf("\n Ray Number %d had no hit", rayIndex);
			}
		}
	}

	double totalEndTime = GetCurrentTimeSeconds();
	m_cachedRaycastTime = totalEndTime - totalStartTime;
	DebuggerPrintf("\n Total Time for Raycasts this frame: %f", m_cachedRaycastTime);

	gProfiler->ProfilerPop();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderRaycast() const
{
	std::vector<Vertex_PCU> rayVerts;

	AddVertsForArrow2D(rayVerts, m_rayStart, m_rayEnd, 0.5f, Rgba::DIM_TRANSLUCENT_GREY);
	//Draw the surface normal
	if (m_isHitting)
	{
		Vec2 endAlongNormal = m_drawRayEnd + m_surfaceNormalLength * m_drawSurfanceNormal;
		AddVertsForArrow2D(rayVerts, m_drawRayEnd, endAlongNormal, 0.5f, Rgba::ORGANIC_BLUE);

		AddVertsForArrow2D(rayVerts, m_drawRayStart, m_drawRayEnd, 0.5f, Rgba::ORGANIC_ORANGE);
	}

	g_renderContext->DrawVertexArray(rayVerts);
}

//------------------------------------------------------------------------------------------------------------------------------
ConvexPoly2D Game::MakeConvexPoly2DFromDisc(const Vec2& center, float radius) const
{
	std::vector<Vec2> convexPolyPoints;

	float startAngle;
	Vec2 point = GetRandomPointOnDisc2D(center, radius, 10.f, 170.f, startAngle);
	convexPolyPoints.push_back(point);

	float currentAngle = startAngle;
	bool completedCircle = false;

	while (!completedCircle)
	{
		point = GetRandomPointOnDisc2D(center, radius, currentAngle, currentAngle + 170.f, currentAngle);
		
		if (currentAngle > 360.f)
		{
			completedCircle = true;
			break;
		}

		convexPolyPoints.push_back(point);
	}

	ConvexPoly2D polygon(convexPolyPoints);
	return polygon;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::PostRender()
{
	if(!m_isDebugSetup)
	{
		//SetStartupDebugRenderObjects();

		ColorTargetView* ctv = g_renderContext->GetFrameColorTarget();
		//Setup debug render client data
		g_debugRenderer->SetClientDimensions( (int)WORLD_HEIGHT, (int)WORLD_WIDTH);
		g_debugRenderer->SetWorldSize2D(m_mainCamera->GetOrthoBottomLeft(), m_mainCamera->GetOrthoTopRight());

		m_isDebugSetup = true;
	}

	//All screen Debug information
	DebugRenderToScreen();

	ClearGarbageEntities();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::Update( float deltaTime )
{
	gProfiler->ProfilerPush("Game::Update");
	//UpdateCamera(deltaTime);

	m_gameCursor->Update(deltaTime);

	if (g_devConsole->GetFrameCount() > 1 && !m_consoleDebugOnce)
	{
		//We have rendered the 1st frame
		m_devConsoleCamera->SetOrthoView(Vec2::ZERO, Vec2(WORLD_WIDTH, WORLD_HEIGHT));
		m_consoleDebugOnce = true;
	}

	if (m_numPolygonsLastFrame != ui_numPolygons)
	{
		CreateConvexPolygons(ui_numPolygons);
		m_numPolygonsLastFrame = ui_numPolygons;
	}

	if (m_numRaysLastFrame != ui_numRays)
	{
		CreateRaycasts(ui_numRays);
		m_numRaysLastFrame = ui_numRays;
	}

	UpdateImGUI();

	UpdateVisualRay();
	CheckRenderRayVsConvexHulls();

	if (m_toggleBroadPhaseMode)
	{
		CheckRaycastsBroadPhase();
	}
	else
	{
		CheckAllRayCastsVsConvexHulls();
	}

	gProfiler->ProfilerPop();
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateCamera(float deltaTime)
{
	m_mainCamera = new Camera();
	Vec2 orthoBottomLeft = Vec2(0.f,0.f);
	Vec2 orthoTopRight = Vec2(WORLD_WIDTH, WORLD_HEIGHT);
	m_mainCamera->SetOrthoView(orthoBottomLeft, orthoTopRight);

	float shakeX = 0.f;
	float shakeY = 0.f;

	if(g_shakeAmount > 0)
	{
		shakeX = g_RNG->GetRandomFloatInRange(-g_shakeAmount, g_shakeAmount);
		shakeY = g_RNG->GetRandomFloatInRange(-g_shakeAmount, g_shakeAmount);

		g_shakeAmount -= deltaTime * CAMERA_SHAKE_REDUCTION_PER_SECOND;
	}
	else
	{
		g_shakeAmount = 0;
	}

	Vec2 translate2D = Vec2(shakeX, shakeY);
	translate2D.ClampLength(MAX_SHAKE);
	m_mainCamera->Translate2D(translate2D);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateCameraMovement( unsigned char keyCode )
{
	switch( keyCode )
	{
	case W_KEY:
	{
		if(m_mainCamera->GetOrthoTopRight().y < m_worldBounds.m_maxBounds.y)
		{
			//Move the camera up
			m_mainCamera->Translate2D(Vec2(0.f, 1.0f));
		}
	}
	break;
	case S_KEY:
	{
		if(m_mainCamera->GetOrthoBottomLeft().y > m_worldBounds.m_minBounds.y)
		{
			//Move the camera down
			m_mainCamera->Translate2D(Vec2(0.f, -1.0f));
		}
	}
	break;
	case A_KEY:
	{
		//For when we are over world bounds
		if(m_mainCamera->GetOrthoTopRight().x > m_worldBounds.m_maxBounds.x)
		{
			//Move the camera left
			m_mainCamera->Translate2D(Vec2(-1.f, 0.0f));
		}

		//For when we are inside the world bounds
		if(m_mainCamera->GetOrthoBottomLeft().x > m_worldBounds.m_minBounds.x)
		{
			//Move the camera left
			m_mainCamera->Translate2D(Vec2(-1.f, 0.0f));
		}
	}
	break;
	case D_KEY:
	{
		//When we are bigger than the world bounds
		if(m_mainCamera->GetOrthoBottomLeft().x < m_worldBounds.m_minBounds.x)
		{
			//Move the camera right
			m_mainCamera->Translate2D(Vec2(1.f, 0.0f));
		}

		//For when we are inside the world bounds
		if(m_mainCamera->GetOrthoTopRight().x < m_worldBounds.m_maxBounds.x)
		{
			//Move the camera right
			m_mainCamera->Translate2D(Vec2(1.f, 0.0f));
		}
	}
	break;
	default:
	break;
	}

}

//------------------------------------------------------------------------------------------------------------------------------
void Game::ClearGarbageEntities()
{
	//If needed, kill everything here
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::IsAlive()
{
	//Check if alive
	return m_isGameAlive;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC Vec2 Game::GetClientToWorldPosition2D( IntVec2 mousePosInClient, IntVec2 ClientBounds )
{
	Clamp(static_cast<float>(mousePosInClient.x), 0.f, static_cast<float>(ClientBounds.x));
	Clamp(static_cast<float>(mousePosInClient.y), 0.f, static_cast<float>(ClientBounds.y));

	float posOnX = RangeMapFloat(static_cast<float>(mousePosInClient.x), 0.0f, static_cast<float>(ClientBounds.x), 0.f, WORLD_WIDTH);
	float posOnY = RangeMapFloat(static_cast<float>(mousePosInClient.y), static_cast<float>(ClientBounds.y), 0.f, 0.f, WORLD_HEIGHT);

	return Vec2(posOnX, posOnY);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CreateConvexPolygons(int numPolygons)
{
	//early out if we have the correct number of polygons
	if (numPolygons == m_convexPolys.size())
		return;

	//If we have lesser than what we need, let's make some
	if (numPolygons > m_convexPolys.size())
	{
		for (int polygonIndex = 0; polygonIndex < numPolygons; polygonIndex++)
		{
			//Make polygons here and push them into the vector
			float randomRadius = g_RNG->GetRandomFloatInRange(MIN_CONSTRUCTION_RADIUS, MAX_CONSTRUCTION_RADIUS);

			Vec2 randomPosition;
			randomPosition.x = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.x + randomRadius + BUFFER_SPACE, m_worldBounds.m_maxBounds.x - randomRadius - BUFFER_SPACE);
			randomPosition.y = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.y + randomRadius + BUFFER_SPACE, m_worldBounds.m_maxBounds.y - randomRadius - BUFFER_SPACE);

			ConvexPoly2D polygon = MakeConvexPoly2DFromDisc(randomPosition, randomRadius);
			IntVec2 bitField;
			
			bitField = m_broadPhaseChecker.GetRegionForConvexPoly(polygon);
			polygon.SetBitFieldsForBitBucketBroadPhase(bitField);

			ConvexHull2D hull;
			hull.MakeConvexHullFromConvexPolyon(polygon);
			hull.SetBitFieldsForBitBucketBroadPhase(bitField);

			m_convexPolys.push_back(polygon);
			m_convexHulls.push_back(hull);
		}
	}
	else
	{
		//We have more polygons than we need do just discard some of them
		while (m_convexPolys.size() > numPolygons)
		{
			m_convexPolys.pop_back();
			m_convexHulls.pop_back();
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CreateRaycasts(int numRaycasts)
{
	//early out if we have the correct number of polygons
	if (numRaycasts == m_rays.size())
		return;

	//If we have lesser than what we need, let's make some
	if (numRaycasts > m_rays.size())
	{
		for (int rayIndex = 0; rayIndex < numRaycasts; rayIndex++)
		{
			//Make rays here and push them into the vector
			Vec2 randomPosition;
			randomPosition.x = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.x + BUFFER_SPACE, m_worldBounds.m_maxBounds.x - BUFFER_SPACE);
			randomPosition.y = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.y + BUFFER_SPACE, m_worldBounds.m_maxBounds.y - BUFFER_SPACE);

			Vec2 randomDirection;
			randomDirection.x = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.x + BUFFER_SPACE, m_worldBounds.m_maxBounds.x - BUFFER_SPACE);
			randomDirection.y = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.y + BUFFER_SPACE, m_worldBounds.m_maxBounds.y - BUFFER_SPACE);
			randomDirection.Normalize();

			Ray2D ray(randomPosition, randomDirection);
			IntVec2 regionBitFields = m_broadPhaseChecker.GetRegionForRay(ray);
			ray.m_bitFieldsXY = regionBitFields;

			RayHit2D hit;

			m_rays.push_back(ray);
			m_hits.push_back(hit);
		}
	}
	else
	{
		//We have more rays than we need so just discard some of them
		while (m_rays.size() > numRaycasts)
		{
			m_rays.pop_back();
			m_hits.pop_back();
		}
	}

}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CreateRenderRay()
{
	//Setup some random start and end positions
	m_rayStart.x = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.x + BUFFER_SPACE, m_worldBounds.m_maxBounds.x - BUFFER_SPACE);
	m_rayStart.y = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.y + BUFFER_SPACE, m_worldBounds.m_maxBounds.y - BUFFER_SPACE);

	m_rayEnd.x = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.x + BUFFER_SPACE, m_worldBounds.m_maxBounds.x - BUFFER_SPACE);
	m_rayEnd.y = g_RNG->GetRandomFloatInRange(m_worldBounds.m_minBounds.y + BUFFER_SPACE, m_worldBounds.m_maxBounds.y - BUFFER_SPACE);

	m_renderedRay.m_start = m_rayStart;
	m_renderedRay.m_direction = m_rayEnd - m_rayStart;
	m_renderedRay.m_direction.Normalize();
}
