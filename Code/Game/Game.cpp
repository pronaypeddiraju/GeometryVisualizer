//------------------------------------------------------------------------------------------------------------------------------
#include "Game/Game.hpp"
//Engine Systems
#include "Engine/Commons/ErrorWarningAssert.hpp"
#include "Engine/Commons/StringUtils.hpp"
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

//Globals
Rgba* g_clearScreenColor = nullptr;
float g_shakeAmount = 0.0f;
RandomNumberGenerator* g_randomNumGen;
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
	g_randomNumGen = new RandomNumberGenerator();

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

	g_clearScreenColor = new Rgba(0.f, 0.f, 0.f, 1.f);
	m_gameCursor = new GameCursor();

	//Create the world bounds AABB2
	Vec2 minWorldBounds = Vec2(20.f, -20.f);
	Vec2 maxWorldBounds = Vec2(WORLD_WIDTH, WORLD_HEIGHT) + Vec2(-20.f, 20.f);
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
		case A_KEY:
		case D_KEY:
		//m_gameCursor->HandleKeyPressed(keyCode);
		UpdateCameraMovement(keyCode);
		break;
		case LCTRL_KEY:
		{
			m_toggleUI = !m_toggleUI;
		}
		break;
		case SPACE_KEY:
		{
			ToggleSimType();
			break;
		}
		case DEL_KEY:
		{
			if(m_selectedGeometry == nullptr)
			{
				return;
			}

			//Destroy selected object
			delete m_selectedGeometry;
			m_selectedGeometry = nullptr;
			//Set the reference to the object in vector to be nullptr
			m_allGeometry[m_selectedIndex] = nullptr;
			break;
		}
		case G_KEY:
		{
			ChangeCurrentGeometry();
			break;
		}
		case TAB_KEY:
		{
			break;
		}
		case K_KEY:
		{
			//Friction increase
			m_objectFriction -= m_frictionStep;
			m_objectFriction = Clamp(m_objectFriction, 0.f, 1.f);
		}
		break;
		case L_KEY:
		{
			//Friction decrease
			m_objectFriction += m_frictionStep;
			m_objectFriction = Clamp(m_objectFriction, 0.f, 1.f);
		}
		break;
		case NUM_1:
		{
			//Decrease Linear Drag
			m_objectLinearDrag -= m_linearDragStep;
		}
		break;
		case NUM_2:
		{
			//Increase Linear Drag
			m_objectLinearDrag += m_linearDragStep;
		}
		break;
		case NUM_3:
		{
			//Decrease Angular Drag
			m_objectAngularDrag -= m_angularDragStep;
		}
		break;
		case NUM_4:
		{
			//Increase Angular Drag
			m_objectAngularDrag += m_angularDragStep;
		}
		break;
		case NUM_5:
		{
			//Toggle X freedom
			m_xFreedom = !m_xFreedom;
		}
		break;
		case NUM_6:
		{
			//Toggle Y freedom
			m_yFreedom = !m_yFreedom;
		}
		break;
		case NUM_7:
		{
			//Toggle rotation freedom
			m_rotationFreedom = !m_rotationFreedom;
		}
		break;
		case NUM_8:
		{
			//Save the game
			SaveToFile("Data/Gameplay/SaveGame.xml");
		}
		break;
		case NUM_9:
		{
			//Load the game
			LoadFromFile("Data/Gameplay/SaveGame.xml");
		}
		break;
		case N_KEY:
		m_objectMass -= m_massStep;
		m_objectMass = Clamp(m_objectMass, 0.1f, 10.f);
		break;
		case M_KEY:
		m_objectMass += m_massStep;
		m_objectMass = Clamp(m_objectMass, 0.1f, 10.f);
		break;
		case KEY_LESSER:
		m_objectRestitution -= m_restitutionStep;
		m_objectRestitution = Clamp(m_objectRestitution, 0.f, 1.f);
		break;
		case KEY_GREATER:
		m_objectRestitution += m_restitutionStep;
		m_objectRestitution  = Clamp(m_objectRestitution, 0.f, 1.f);
		break;
		case F1_KEY:
		{
			break;
		}
		break;
		case F2_KEY:
		{
			//F2 spawns a static disc on the cursor position
			Geometry* geometry = new Geometry(*g_physicsSystem, STATIC_SIMULATION, CAPSULE_GEOMETRY, m_gameCursor->GetCursorPositon());
			geometry->m_rigidbody->m_material.restitution = m_objectRestitution;
			geometry->m_rigidbody->m_mass = INFINITY;
			geometry->m_rigidbody->m_friction = m_objectFriction;
			geometry->m_rigidbody->m_angularDrag = m_objectLinearDrag;
			geometry->m_rigidbody->m_linearDrag = m_objectAngularDrag;
			geometry->m_collider->SetMomentForObject();

			m_allGeometry.push_back(geometry);
		}
		break;
		case F3_KEY:
		{
			//F3 spawns a dynamic box on the cursor position
			Geometry* geometry = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, BOX_GEOMETRY, m_gameCursor->GetCursorPositon(), 0.f, 3.f, m_gameCursor->GetCursorPositon() + Vec2(10.f, 10.f));
			geometry->m_rigidbody->m_mass = m_objectMass;
			geometry->m_rigidbody->m_friction = m_objectFriction;
			geometry->m_rigidbody->m_angularDrag = m_objectLinearDrag;
			geometry->m_rigidbody->m_linearDrag = m_objectAngularDrag;
			geometry->m_collider->SetMomentForObject();
			geometry->m_rigidbody->m_material.restitution = m_objectRestitution;


			m_allGeometry.push_back(geometry);
		}
		break;
		case F4_KEY:
		{
			//F4 spawns a dynamic box on the cursor position (Rotated by 90 degrees
			Geometry* geometry = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, BOX_GEOMETRY, m_gameCursor->GetCursorPositon(), 90.f, 3.f, m_gameCursor->GetCursorPositon() + Vec2(10.f, 10.f));
			geometry->m_rigidbody->m_mass = m_objectMass;
			geometry->m_rigidbody->m_friction = m_objectFriction;
			geometry->m_rigidbody->m_angularDrag = m_objectLinearDrag;
			geometry->m_rigidbody->m_linearDrag = m_objectAngularDrag;
			geometry->m_collider->SetMomentForObject();
			geometry->m_rigidbody->m_material.restitution = m_objectRestitution;

			m_allGeometry.push_back(geometry);
		}
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
int Game::GetNextValidGeometryIndex(int index)
{
	int vectorSize = static_cast<int>(m_allGeometry.size());

	int end = index;
	index = (index + 1) % vectorSize;

	while(index != end)
	{
		if(m_allGeometry[index] != nullptr)
		{
			return index;
		}
		else
		{
			index = (index + 1) % vectorSize;
		}
	}

	return end;
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
		m_gameCursor->HandleKeyReleased(keyCode);

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
	int numGeometry = static_cast<int>(m_allGeometry.size()) ;

	if(numGeometry == 0)
	{
		return true;
	}

	//Select object for possession
	float distMinSq = 200.f;
	m_selectedIndex = 0;
	for(int geometryIndex = 0; geometryIndex < numGeometry; geometryIndex++)
	{
		if(m_allGeometry[geometryIndex] == nullptr)
		{
			continue;
		}

		float distSq = GetDistanceSquared2D(m_gameCursor->GetCursorPositon(), m_allGeometry[geometryIndex]->m_transform.m_position);
		if(distMinSq > distSq)
		{
			distMinSq = distSq;
			m_selectedIndex = geometryIndex;
		}
	}

	//Now select the actual object
	m_selectedGeometry = m_allGeometry[m_selectedIndex];
	g_selectedSimType = m_selectedGeometry->m_rigidbody->GetSimulationType();

	m_selectedGeometry->m_rigidbody->SetSimulationMode(STATIC_SIMULATION);
	m_gameCursor->SetCursorPosition(m_selectedGeometry->m_transform.m_position);
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseLBUp()
{
	if(m_selectedGeometry == nullptr)
	{
		return true;
	}

	//De-select object
	m_selectedGeometry->m_rigidbody->SetSimulationMode(g_selectedSimType);
	m_selectedGeometry->m_rigidbody->m_velocity = Vec2::ZERO;
	m_selectedGeometry->m_rigidbody->m_mass = m_objectMass;
	m_selectedGeometry->m_rigidbody->m_friction = m_objectFriction;
	m_selectedGeometry->m_rigidbody->m_linearDrag = m_objectLinearDrag;
	m_selectedGeometry->m_rigidbody->m_angularDrag = m_objectAngularDrag;
	m_selectedGeometry->m_rigidbody->m_material.restitution = m_objectRestitution;
	m_selectedGeometry->m_rigidbody->SetConstraints(m_xFreedom, m_yFreedom, m_rotationFreedom);

	m_selectedGeometry = nullptr;
	m_selectedIndex = -1;

	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseRBDown()
{
	m_mouseStart = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Game::HandleMouseRBUp()
{

	m_mouseEnd = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());

	Geometry* geometry;

	//Calculate the object center, rotation and bounds
	Vec2 disp = m_mouseStart - m_mouseEnd;
	Vec2 norm = disp.GetNormalized();
	float length = disp.GetLength();

	Vec2 center = m_mouseEnd + length * norm * 0.5f;
	float rotationDegrees = disp.GetAngleDegrees() + 90.f;

	//Switch on geometry type and construct required collider
	switch( m_geometryType )
	{
	case TYPE_UNKNOWN:
	break;
	case AABB2_GEOMETRY:
	break;
	case DISC_GEOMETRY:
	break;
	case BOX_GEOMETRY:
	{
		//Calculate Centre from mouse Start and End
		

		if(m_isStatic)
		{
			geometry = new Geometry(*g_physicsSystem, STATIC_SIMULATION, BOX_GEOMETRY, center, rotationDegrees, length);
			geometry->m_rigidbody->m_mass = INFINITY;
			geometry->m_rigidbody->SetConstraints(false, false, false);
			geometry->m_rigidbody->m_collider->SetCollisionEvent("StaticCollisionEvent");
		}
		else
		{
			geometry = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, BOX_GEOMETRY, center, rotationDegrees, length);
			geometry->m_rigidbody->m_mass = m_objectMass;
			geometry->m_rigidbody->SetConstraints(m_xFreedom, m_yFreedom, m_rotationFreedom);
			geometry->m_rigidbody->m_collider->SetCollisionEvent("DynamicCollisionEvent");
		}
		geometry->m_rigidbody->m_material.restitution = m_objectRestitution;
		geometry->m_rigidbody->m_friction = m_objectFriction;
		geometry->m_rigidbody->m_angularDrag = m_objectAngularDrag;
		geometry->m_rigidbody->m_linearDrag = m_objectLinearDrag;
		geometry->m_collider->SetMomentForObject();
		m_allGeometry.push_back(geometry);

	}
	break;
	case CAPSULE_GEOMETRY:
	{
		if(m_isStatic)
		{
			geometry = new Geometry(*g_physicsSystem, STATIC_SIMULATION, CAPSULE_GEOMETRY, m_mouseStart, rotationDegrees, 0.f, m_mouseEnd);
			geometry->m_rigidbody->m_mass = INFINITY;
			geometry->m_rigidbody->SetConstraints(false, false, false);
			geometry->m_rigidbody->m_collider->SetCollisionEvent("StaticCollisionEvent");
		}
		else
		{
			geometry = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, CAPSULE_GEOMETRY, m_mouseStart, rotationDegrees, 0.f, m_mouseEnd);
			geometry->m_rigidbody->m_mass = m_objectMass;
			geometry->m_rigidbody->SetConstraints(m_xFreedom, m_yFreedom, m_rotationFreedom);
			geometry->m_rigidbody->m_collider->SetCollisionEvent("DynamicCollisionEvent");
		}
		geometry->m_rigidbody->m_friction = m_objectFriction;
		geometry->m_rigidbody->m_angularDrag = m_objectAngularDrag;
		geometry->m_rigidbody->m_linearDrag = m_objectLinearDrag;
		geometry->m_collider->SetMomentForObject();
		geometry->m_rigidbody->m_material.restitution = m_objectRestitution;

		m_allGeometry.push_back(geometry);
	}
	break;
	case NUM_GEOMETRY_TYPES:
	break;
	default:
	break;
	}

	return true;
}

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
		
	g_renderContext->ClearColorTargets(*g_clearScreenColor);

	g_renderContext->BindShader( m_shader );

	//RenderAllGeometry();

	RenderWorldBounds();

	RenderPersistantUI();

	if(m_toggleUI)
	{
		RenderOnScreenInfo();
	}

	RenderDebugObjectInfo();

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
	//Count the number of objects
	int staticVectorSize = static_cast<int>(g_physicsSystem->m_rbBucket->m_RbBucket[STATIC_SIMULATION].size());
	int dynamicVectorSize = static_cast<int>(g_physicsSystem->m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION].size());
	int staticCount = 0;
	int dynamicCount = 0;

	Vec2 camMinBounds = m_mainCamera->GetOrthoBottomLeft();
	Vec2 camMaxBounds = m_mainCamera->GetOrthoTopRight();

	for(int staticIndex = 0; staticIndex < staticVectorSize; staticIndex++)
	{
		if(g_physicsSystem->m_rbBucket->m_RbBucket[STATIC_SIMULATION][staticIndex] != nullptr)
		{
			staticCount++;
		}
	}

	for(int dynamicIndex = 0; dynamicIndex < dynamicVectorSize; dynamicIndex++)
	{
		if(g_physicsSystem->m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][dynamicIndex] != nullptr)
		{
			dynamicCount++;
		}
	}

	int lineIndex = 2;

	//Display static and dynamic object count
	std::string printStringStatic = "Number of Static Objects : ";
	printStringStatic += std::to_string(staticCount);

	std::string printStringDynamic = "Number of Dynamic Objects : ";
	printStringDynamic += std::to_string(dynamicCount);

	std::vector<Vertex_PCU> textVerts;
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringStatic, Rgba::WHITE);
	lineIndex++;

	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringDynamic, Rgba::WHITE);
	lineIndex += 3;

	//Mass Information
	std::string printStringMassClamp = "Mass Clamped between 0.1 and 10.0";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringMassClamp, Rgba::WHITE);
	lineIndex++;

	//Mass values
	std::string printStringMass = "Object Mass (Adjust with N , M) : ";
	printStringMass += std::to_string(m_objectMass);
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringMass, Rgba::WHITE);
	lineIndex++;

	//Restitution information
	std::string printStringRestitutionClamp = "Restitution Clamped between 0 and 1";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringRestitutionClamp, Rgba::WHITE);
	lineIndex++;

	//Restitution values
	std::string printStringRestitution = "Object Restitution (Adjust with < , > ) : ";
	printStringRestitution += std::to_string(m_objectRestitution);
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringRestitution, Rgba::WHITE);
	lineIndex += 3;

	//Friction
	std::string printStringFriction = "Object Friction (Adjust with K , L ) : ";
	printStringFriction += std::to_string(m_objectFriction);
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringFriction, Rgba::YELLOW);
	lineIndex++;

	//Linear Drag
	std::string printStringLDrag = "Object Linear Drag (Adjust with NUM_1 , NUM_2 ) : ";
	printStringLDrag += std::to_string(m_objectLinearDrag);
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringLDrag, Rgba::YELLOW);
	lineIndex++;

	//Angular Drag
	std::string printStringADrag = "Object Angular Drag (Adjust with NUM_3 , NUM_4 ) : ";
	printStringADrag += std::to_string(m_objectAngularDrag);
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringADrag, Rgba::YELLOW);
	lineIndex += 3;

	//Constraint X
	std::string printStringXCon = "Constraint On X (Toggle with NUM_5) : ";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringXCon, Rgba::WHITE);
	Rgba constraintColor = Rgba::WHITE;
	if (m_xFreedom)
	{
		constraintColor = Rgba::GREEN;
		printStringXCon = "FREE";
	}
	else
	{
		constraintColor = Rgba::RED;
		printStringXCon = "LOCKED";
	}
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight * 40.f, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringXCon, constraintColor);
	lineIndex++;

	//Constraint Y
	std::string printStringYCon = "Constraint On Y (Toggle with NUM_6) : ";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringYCon, Rgba::WHITE);
	constraintColor = Rgba::WHITE;
	if (m_yFreedom)
	{
		constraintColor = Rgba::GREEN;
		printStringYCon = "FREE";
	}
	else
	{
		constraintColor = Rgba::RED;
		printStringYCon = "LOCKED";
	}
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight * 40.f, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringYCon, constraintColor);
	lineIndex++;

	//Constraint rotation
	std::string printStringRotCon = "Constraint On Z (Toggle with NUM_7) : ";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringRotCon, Rgba::WHITE);
	constraintColor = Rgba::WHITE;
	if (m_rotationFreedom)
	{
		constraintColor = Rgba::GREEN;
		printStringRotCon = "FREE";
	}
	else
	{
		constraintColor = Rgba::RED;
		printStringRotCon = "LOCKED";
	}
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight * 40.f, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringRotCon, constraintColor);
	lineIndex += 3;

	//Simulation type
	std::string printStringSimType = "Simulation (Space Key) : ";
	if(m_isStatic)
	{
		printStringSimType += "Static";
	}
	else
	{
		printStringSimType += "Dynamic";
	}
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringSimType, Rgba::ORANGE);
	lineIndex++;

	std::string printStringObjType = "Geometry (G Key) : ";
	switch( m_geometryType )
	{
	case TYPE_UNKNOWN:
	break;
	case AABB2_GEOMETRY:
	printStringObjType += "AABB2";
	break;
	case DISC_GEOMETRY:
	printStringObjType += "DISC";
	break;
	case BOX_GEOMETRY:
	printStringObjType += "OBB";
	break;
	case CAPSULE_GEOMETRY:
	printStringObjType += "CAPSULE";
	break;
	case NUM_GEOMETRY_TYPES:
	break;
	default:
	break;
	}

	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringObjType, Rgba::ORANGE);
	lineIndex += 3;

	//Time
	std::string printStringTime = "Pause Game (X_KEY)";
	Rgba timeColor = Rgba::GREEN;
	if (g_gameClock->IsPaused())
	{
		timeColor = Rgba::RED;
	}
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringTime, timeColor);
	lineIndex++;

	printStringTime = "Resume Game (Z_KEY)";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringTime, Rgba::GREEN);
	lineIndex++;

	printStringTime = "Dilation 0.1 (C_KEY)";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringTime, Rgba::WHITE);
	lineIndex++;

	printStringTime = "Dilation 2.0 (V_KEY)";
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMaxBounds.y - m_fontHeight * lineIndex), m_fontHeight, printStringTime, Rgba::WHITE);
	lineIndex++;

	//Mouse Debug
	std::string printStringMousePos = "Mouse Position : ";
	printStringMousePos += std::to_string(g_windowContext->GetClientMousePosition().x);
	printStringMousePos += ", ";
	printStringMousePos += std::to_string(g_windowContext->GetClientMousePosition().y);

	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMinBounds.y + 10.f), m_fontHeight, printStringMousePos, Rgba::WHITE);

	std::string printStringClientBounds = "Client Bounds: ";
	printStringClientBounds += std::to_string(g_windowContext->GetClientBounds().x);
	printStringClientBounds += ", ";
	printStringClientBounds += std::to_string(g_windowContext->GetClientBounds().y);

	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMinBounds.x + m_fontHeight, camMinBounds.y + 10.f - m_fontHeight), m_fontHeight, printStringClientBounds, Rgba::WHITE);

	g_renderContext->BindTextureViewWithSampler(0U, m_squirrelFont->GetTexture());
	g_renderContext->DrawVertexArray(textVerts);
	g_renderContext->BindTextureViewWithSampler(0U, nullptr);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderPersistantUI() const
{
	Vec2 camMinBounds = m_mainCamera->GetOrthoBottomLeft();
	Vec2 camMaxBounds = m_mainCamera->GetOrthoTopRight();

	//Toggle UI 
	std::string printString = "Toggle Control Scheme (LCTRL Button) ";
	std::vector<Vertex_PCU> textVerts;
	m_squirrelFont->AddVertsForText2D(textVerts, Vec2(camMaxBounds.x - 90.f, camMaxBounds.y - m_fontHeight), m_fontHeight, printString, Rgba::ORANGE);

	g_renderContext->BindTextureViewWithSampler(0U, m_squirrelFont->GetTexture());
	g_renderContext->DrawVertexArray(textVerts);
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderAllGeometry() const
{
	// display debug information
	g_physicsSystem->DebugRender( g_renderContext ); 

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
void Game::PostRender()
{
	if(!m_isDebugSetup)
	{
		//SetStartupDebugRenderObjects();

		ColorTargetView* ctv = g_renderContext->GetFrameColorTarget();
		//Setup debug render client data
		g_debugRenderer->SetClientDimensions( ctv->m_height, ctv->m_width );
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
	//UpdateCamera(deltaTime);

	m_gameCursor->Update(deltaTime);

	UpdateGeometry(deltaTime);

	if(m_selectedGeometry != nullptr)
	{
		m_selectedGeometry->m_transform.m_position = m_gameCursor->GetCursorPositon();
	}

	if (g_devConsole->GetFrameCount() > 1 && !m_consoleDebugOnce)
	{
		//We have rendered the 1st frame
		m_devConsoleCamera->SetOrthoView(Vec2::ZERO, Vec2(WORLD_WIDTH, WORLD_HEIGHT));
		m_consoleDebugOnce = true;
	}

}

//------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateGeometry( float deltaTime )
{
	// let physics system play out
	g_physicsSystem->Update(deltaTime);
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
		shakeX = g_randomNumGen->GetRandomFloatInRange(-g_shakeAmount, g_shakeAmount);
		shakeY = g_randomNumGen->GetRandomFloatInRange(-g_shakeAmount, g_shakeAmount);

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
	//Kill any entity off screen
	for (int geometryIndex = 0; geometryIndex < m_allGeometry.size(); geometryIndex++)
	{
		if (!m_allGeometry[geometryIndex]->m_rigidbody->m_isAlive)
		{
			m_allGeometry[geometryIndex]->m_collider = nullptr;
			m_allGeometry[geometryIndex]->m_rigidbody = nullptr;
		}

		if(m_allGeometry[geometryIndex] == nullptr)
		{
			continue;
		}

		bool isOffRight = (m_allGeometry[geometryIndex]->m_transform.m_position.x > m_worldBounds.m_maxBounds.x);
		bool isOffLeft = (m_allGeometry[geometryIndex]->m_transform.m_position.x < m_worldBounds.m_minBounds.x);
		bool isOffBottom = (m_allGeometry[geometryIndex]->m_transform.m_position.y < m_worldBounds.m_minBounds.y);
		bool isOffTop = (m_allGeometry[geometryIndex]->m_transform.m_position.y > m_worldBounds.m_maxBounds.y);

		if(isOffRight || isOffLeft || isOffTop || isOffBottom)
		{
			if(m_allGeometry[geometryIndex] == m_selectedGeometry)
			{
				m_selectedGeometry = nullptr;
			}

			delete m_allGeometry[geometryIndex];
			m_allGeometry[geometryIndex] = nullptr;
			m_allGeometry.erase(m_allGeometry.begin() + geometryIndex);
			geometryIndex--;
		}

	}

	g_physicsSystem->PurgeDeletedObjects();

	for (int geometryIndex = 0; geometryIndex < m_allGeometry.size(); geometryIndex++)
	{
		if (m_allGeometry[geometryIndex]->m_rigidbody == nullptr)
		{
			delete m_allGeometry[geometryIndex];
			m_allGeometry[geometryIndex] = nullptr;
			m_allGeometry.erase(m_allGeometry.begin() + geometryIndex);
			geometryIndex--;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::CheckCollisions()
{
	//Look for any collisions and call required methods for collision handling
	
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
void Game::ToggleSimType()
{
	m_isStatic = !m_isStatic;
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::ChangeCurrentGeometry()
{
	if(m_geometryType == BOX_GEOMETRY)
	{
		m_geometryType = CAPSULE_GEOMETRY;
	}
	else
	{
		m_geometryType = BOX_GEOMETRY;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::SaveToFile(const std::string& filePath)
{
	//Save all scene objects to the file
	tinyxml2::XMLDocument saveDoc;

	tinyxml2::XMLNode* rootNode = saveDoc.NewElement("SavedGeometry");
	saveDoc.InsertFirstChild(rootNode);

	int numObjects = (int)m_allGeometry.size();
	for (int index = 0; index < numObjects; index++)
	{
		//Save all the object properties using XML
		XMLElement* geometry = saveDoc.NewElement("GeometryData");
		XMLElement* rbElem = saveDoc.NewElement("RigidBody");
		geometry->InsertEndChild(rbElem);

		//Rigidbody data
		rbElem->SetAttribute("SimType", m_allGeometry[index]->m_rigidbody->GetSimulationType());
		rbElem->SetAttribute("Shape", m_allGeometry[index]->m_collider->m_colliderType);
		rbElem->SetAttribute("Mass", m_allGeometry[index]->m_rigidbody->m_mass);
		rbElem->SetAttribute("Friction", m_allGeometry[index]->m_rigidbody->m_friction);
		rbElem->SetAttribute("AngularDrag", m_allGeometry[index]->m_rigidbody->m_angularDrag);
		rbElem->SetAttribute("LinearDrag", m_allGeometry[index]->m_rigidbody->m_linearDrag);
		rbElem->SetAttribute("Freedom", m_allGeometry[index]->m_rigidbody->m_constraints.GetAsString().c_str());
		rbElem->SetAttribute("Moment", m_allGeometry[index]->m_rigidbody->m_momentOfInertia);
		rbElem->SetAttribute("Restitution", m_allGeometry[index]->m_rigidbody->m_material.restitution);

		XMLElement* colElem = saveDoc.NewElement("Collider");
		geometry->InsertEndChild(colElem);

		//Collider data
		eColliderType2D type = m_allGeometry[index]->m_collider->GetType();
		Collider2D* collider = m_allGeometry[index]->m_collider;
		
		switch (type)
		{
			case COLLIDER_BOX:
			{
				BoxCollider2D* boxCollider = reinterpret_cast<BoxCollider2D*>(collider);
				colElem->SetAttribute("Center", boxCollider->GetWorldShape().GetCenter().GetAsString().c_str());
				colElem->SetAttribute("Size", (Vec2(boxCollider->GetWorldShape().GetHalfExtents()) * 2.f).GetAsString().c_str());
				colElem->SetAttribute("Rotation", boxCollider->m_rigidbody->m_rotation);
			}
			break;
			case COLLIDER_CAPSULE:
			{
				CapsuleCollider2D* col = reinterpret_cast<CapsuleCollider2D*>(collider);
				colElem->SetAttribute("Start", col->GetReferenceShape().m_start.GetAsString().c_str());
				colElem->SetAttribute("End", col->GetReferenceShape().m_end.GetAsString().c_str());
				colElem->SetAttribute("Radius", col->GetCapsuleRadius());
			}
			break;
		}

		//Transform stuff
		XMLElement* tranformElem = saveDoc.NewElement("Transform");
		geometry->InsertEndChild(tranformElem);

		tranformElem->SetAttribute("Position", m_allGeometry[index]->m_transform.m_position.GetAsString().c_str());
		tranformElem->SetAttribute("Rotation", m_allGeometry[index]->m_transform.m_rotation);
		tranformElem->SetAttribute("Scale", m_allGeometry[index]->m_transform.m_scale.GetAsString().c_str());
		
		rootNode->InsertEndChild(geometry);
	}
	
	//Save to the file
	tinyxml2::XMLError eResult = saveDoc.SaveFile(filePath.c_str());

	if (eResult != tinyxml2::XML_SUCCESS)
	{
		printf("Error: %i\n", eResult);
		ASSERT_RECOVERABLE(true, Stringf("Error: %i\n", eResult));
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::LoadFromFile(const std::string& filePath)
{
	//Delete all existing objects
	for (int index = 0; index < (int)m_allGeometry.size(); index++)
	{
		delete m_allGeometry[index];
		m_allGeometry[index] = nullptr;
	}
	 
	m_allGeometry.erase(m_allGeometry.begin(), m_allGeometry.end());

	//Open the xml file and parse it
	tinyxml2::XMLDocument saveDoc;
	saveDoc.LoadFile(filePath.c_str());

	if (saveDoc.ErrorID() != tinyxml2::XML_SUCCESS)
	{
		DebuggerPrintf("\n Error in opening file %s", filePath);
		DebuggerPrintf("\n >> Error ID : %i ", saveDoc.ErrorID());
		DebuggerPrintf("\n >> Error line number is : %i", saveDoc.ErrorLineNum());
		DebuggerPrintf("\n >> Error name : %s", saveDoc.ErrorName());
		
		ERROR_AND_DIE(">> Error loading Save Game XML file ");
		return;
	}
	else
	{
		//Load from the file and spawn required objects
		XMLElement* rootElement = saveDoc.RootElement();

		XMLElement* geometry = rootElement->FirstChildElement();

		while(geometry != nullptr)
		{
			//Read RB data first
			XMLElement* elem = geometry->FirstChildElement("RigidBody");

			int type = ParseXmlAttribute(*elem, "SimType", 0);
			int shape = ParseXmlAttribute(*elem, "Shape", 0);
			float mass = ParseXmlAttribute(*elem, "Mass", 0.1f);
			float friction = ParseXmlAttribute(*elem, "Friction", 0.f);
			float angularDrag = ParseXmlAttribute(*elem, "AngularDrag", 0.f);
			float linearDrag = ParseXmlAttribute(*elem, "LinearDrag", 0.f);
			Vec3 freedom = ParseXmlAttribute(*elem, "Freedom", Vec3::ONE);
			float moment = ParseXmlAttribute(*elem, "Moment", INFINITY);
			float restitution = ParseXmlAttribute(*elem, "Restitution", 1.f);

			//Read Collider data 
			elem = elem->NextSiblingElement("Collider");
			Geometry* entity = nullptr;

			switch (shape)
			{
			case COLLIDER_BOX:
			{
				Vec2 center = ParseXmlAttribute(*elem, "Center", Vec2::ZERO);
				Vec2 size = ParseXmlAttribute(*elem, "Size", Vec2::ZERO);
				float rotation = ParseXmlAttribute(*elem, "Rotation", 0.f);

				if (type == STATIC_SIMULATION)
				{
					if (size.x == 80.f)
					{
						entity = new Geometry(*g_physicsSystem, STATIC_SIMULATION, BOX_GEOMETRY, center, rotation, size.y, Vec2::ZERO, true);
					}
					else
					{
						entity = new Geometry(*g_physicsSystem, STATIC_SIMULATION, BOX_GEOMETRY, center, rotation, size.y);
					}
					entity->m_rigidbody->m_mass = mass;
					entity->m_rigidbody->SetConstraints(false, false, false);
				}
				else
				{
					entity = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, BOX_GEOMETRY, center, rotation, size.y);
					entity->m_rigidbody->m_mass = mass;
					entity->m_rigidbody->SetConstraints(freedom);
				}
				entity->m_rigidbody->m_material.restitution = restitution;
				entity->m_rigidbody->m_friction = friction;
				entity->m_rigidbody->m_angularDrag = angularDrag;
				entity->m_rigidbody->m_linearDrag = linearDrag;
				entity->m_rigidbody->m_momentOfInertia = moment;
			}
			break;
			case COLLIDER_CAPSULE:
			{
				Vec2 start = ParseXmlAttribute(*elem, "Start", Vec2::ZERO);
				Vec2 end = ParseXmlAttribute(*elem, "End", Vec2::ZERO);
				float radius = ParseXmlAttribute(*elem, "Radius", 0.f);
				UNUSED(radius);

				Vec2 disp = start - end;
				Vec2 norm = disp.GetNormalized();
				float length = disp.GetLength();

				Vec2 center = end + length * norm * 0.5f;
				float rotationDegrees = disp.GetAngleDegrees() + 90.f;

				if (type == STATIC_SIMULATION)
				{
					entity = new Geometry(*g_physicsSystem, STATIC_SIMULATION, CAPSULE_GEOMETRY, start, rotationDegrees, 0.f, end);
					entity->m_rigidbody->m_mass = INFINITY;
					entity->m_rigidbody->SetConstraints(false, false, false);
				}
				else
				{
					entity = new Geometry(*g_physicsSystem, DYNAMIC_SIMULATION, CAPSULE_GEOMETRY, start, rotationDegrees, 0.f, end);
					entity->m_rigidbody->m_mass = mass;
					entity->m_rigidbody->SetConstraints(freedom);
				}
				entity->m_rigidbody->m_friction = friction;
				entity->m_rigidbody->m_angularDrag = angularDrag;
				entity->m_rigidbody->m_linearDrag = linearDrag;
				entity->m_rigidbody->m_momentOfInertia = moment;
				entity->m_rigidbody->m_material.restitution = restitution;
			}
			break;
			default:
			{
				ERROR_AND_DIE("The rigidbody shape in XML file is unknown");
			}
			break;
			}

			//Read Collider data 
			elem = elem->NextSiblingElement("Transform");

			Vec2 position = ParseXmlAttribute(*elem, "Position", Vec2::ZERO);
			float rotation = ParseXmlAttribute(*elem, "Rotation", 0.f);
			Vec2 scale = ParseXmlAttribute(*elem, "Scale", Vec2::ZERO);

			entity->m_transform.m_position = position;
			entity->m_transform.m_rotation = rotation;
			entity->m_transform.m_scale = scale;

			m_allGeometry.push_back(entity);

			//Proceed to next sibling
			geometry = geometry->NextSiblingElement();
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void Game::RenderDebugObjectInfo() const
{
	Vec2 mousePos = GetClientToWorldPosition2D(g_windowContext->GetClientMousePosition(), g_windowContext->GetClientBounds());

	//Render the debug information of the object under the cursor
	int numGeometry = static_cast<int>(m_allGeometry.size());
	for(int index = 0; index < numGeometry; index++)
	{
		if (m_allGeometry[index]->m_collider == nullptr)
		{
			continue;
		}

		if(m_allGeometry[index]->m_collider->Contains(m_gameCursor->GetCursorPositon()))
		{
			//Print the debug information
			std::vector<Vertex_PCU> lineVerts;
			Vec2 offSetPos = mousePos + m_debugOffset;
			AddVertsForLine2D(lineVerts, mousePos, offSetPos, 0.5f, Rgba::WHITE);
			
			int numStrings = 0;

			std::vector<Vertex_PCU> textVerts;

			std::string printPosition = "Position : ";
			printPosition += std::to_string(m_allGeometry[index]->m_transform.m_position.x);
			printPosition += ", ";
			printPosition += std::to_string(m_allGeometry[index]->m_transform.m_position.y);

			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos, m_debugFontHeight, printPosition);

			++numStrings;

			std::string printMass = "Mass : ";
			printMass += std::to_string(m_allGeometry[index]->m_rigidbody->m_mass);

			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printMass);

			++numStrings;

			std::string printVelocity = "Velocity : ";
			printVelocity += std::to_string(m_allGeometry[index]->m_rigidbody->m_velocity.x);
			printVelocity += ", ";
			printVelocity += std::to_string(m_allGeometry[index]->m_rigidbody->m_velocity.y);

			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printVelocity);

			++numStrings;

			std::string printFriction = "Friction : ";
			printFriction += std::to_string(m_allGeometry[index]->m_rigidbody->m_friction);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printFriction, Rgba::YELLOW);
			++numStrings;

			std::string printRestitution = "Restitution : ";
			printRestitution += std::to_string(m_allGeometry[index]->m_rigidbody->m_material.restitution);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printRestitution);
			++numStrings;

			std::string printLDrag = "Linear Drag : ";
			printLDrag += std::to_string(m_allGeometry[index]->m_rigidbody->m_linearDrag);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printLDrag, Rgba::YELLOW);
			++numStrings;

			std::string printADrag = "Angular Drag : ";
			printADrag += std::to_string(m_allGeometry[index]->m_rigidbody->m_angularDrag);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printADrag, Rgba::YELLOW);
			++numStrings;

			std::string printMoment = "Moment of Inertia : ";
			printMoment += std::to_string(m_allGeometry[index]->m_rigidbody->m_momentOfInertia);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printMoment);
			++numStrings;
				
			std::string printAngular = "Angular Velocity: ";
			printAngular += std::to_string(m_allGeometry[index]->m_rigidbody->m_angularVelocity);
			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0, m_debugFontHeight * numStrings), m_debugFontHeight, printAngular);
			++numStrings;

			std::string printConstraints = "Constraints | X= ";
			if(m_xFreedom)			{				printConstraints += "FREE";			}
			else					{				printConstraints += "LOCKED";		}
			
			printConstraints += " | Y= ";
			if (m_yFreedom)			{				printConstraints += "FREE";			}
			else					{				printConstraints += "LOCKED";		}
			
			printConstraints += " | Rotation= ";
			if (m_rotationFreedom)	{				printConstraints += "FREE";			}
			else					{				printConstraints += "LOCKED";		}

			m_squirrelFont->AddVertsForText2D(textVerts, offSetPos - Vec2(0.f, m_debugFontHeight * numStrings), m_debugFontHeight, printConstraints, Rgba::ORANGE);
			++numStrings;

			g_renderContext->BindTextureViewWithSampler(0U, nullptr);
			g_renderContext->DrawVertexArray(lineVerts);
			g_renderContext->BindTextureViewWithSampler(0U, m_squirrelFont->GetTexture());
			g_renderContext->DrawVertexArray(textVerts);
		}
	}
}
