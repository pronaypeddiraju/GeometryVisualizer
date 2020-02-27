#pragma once
#include "Engine/Commons/EngineCommon.hpp"

//------------------------------------------------------------------------------------------------------------------------------

class AudioSystem;
class Clock;
class InputSystem;
class RandomNumberGenerator;
class RenderContext;

//------------------------------------------------------------------------------------------------------------------------------

constexpr float WORLD_WIDTH = 300.f;
constexpr float WORLD_HEIGHT = 150.f;
constexpr float WORLD_CENTER_X = WORLD_WIDTH / 2.f;
constexpr float WORLD_CENTER_Y = WORLD_HEIGHT / 2.f;
constexpr float SCREEN_ASPECT = 16.f/9.f;

constexpr float CAMERA_SHAKE_REDUCTION_PER_SECOND = 1.f;
constexpr float MAX_SHAKE = 2.0f;

constexpr float DEVCONSOLE_LINE_HEIGHT = 3.0f;

constexpr float BOX_MIN_WIDTH = 2.5f;
constexpr float BOX_MAX_WIDTH = 10.0f;

constexpr float DISC_MIN_RADIUS = 2.5f;
constexpr float DISC_MAX_RADIUS = 5.0f;

constexpr float MAX_ZOOM_STEPS = 10.f;

constexpr float CLIENT_ASPECT = 2.0f; // We are requesting a 1:1 aspect (square) window area

constexpr int INIT_NUM_POLYGONS = 128;
constexpr int INIT_NUM_RAYCASTS = 1;

constexpr float MIN_CONSTRUCTION_RADIUS = 5.f;
constexpr float MAX_CONSTRUCTION_RADIUS = 20.f;
constexpr float BUFFER_SPACE = 2.f;

extern AudioSystem* g_audio;
extern Clock* g_gameClock;
extern InputSystem* g_inputSystem;
extern RenderContext* g_renderContext;