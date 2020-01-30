#pragma once
//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/Transform2.hpp"
#include "Engine/Math/Rigidbody2D.hpp"

class PhysicsSystem;
class Collider2D;

//------------------------------------------------------------------------------------------------------------------------------
enum eGeometryType
{
	TYPE_UNKNOWN = -1,

	AABB2_GEOMETRY,
	DISC_GEOMETRY,

	BOX_GEOMETRY,
	CAPSULE_GEOMETRY,

	NUM_GEOMETRY_TYPES
};

//------------------------------------------------------------------------------------------------------------------------------
class Geometry
{
public:
	explicit Geometry(PhysicsSystem& physicsSystem, eSimulationType simulationType, eGeometryType geometryType, const Vec2& cursorPosition, float rotationDegrees = 0.f, float length = 0.f, const Vec2& endPos = Vec2::ZERO, bool staticFloor = false);
	~Geometry();


public:
	Transform2				m_transform; 
	Rigidbody2D				*m_rigidbody;
	Collider2D				*m_collider; 
	eGeometryType			m_geometryType = TYPE_UNKNOWN;
};
