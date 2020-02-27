#pragma once
//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"

//------------------------------------------------------------------------------------------------------------------------------
class Geometry
{
public:
	Geometry();
	explicit Geometry(const std::vector<Vec2>& constructPoints);	//To construct ConvexPoly2D using the construct points;
	explicit Geometry(const std::vector<Plane2D>& constructPlanes);	//To construct ConvexHull2D using the construct planes;
	~Geometry();

	const ConvexPoly2D&			GetConvexPoly2D() const;
	const ConvexHull2D&			GetConvexHull2D() const;

	void						SetBitFieldsForBitBucketBroadPhase(const IntVec2& bitFields);
	const IntVec2&				GetBitFields() const;

	void						MakeHullFromOwningPolygon();	//Makes m_convexHull using m_convexPoly;

public:
	
	ConvexPoly2D	m_convexPoly;
	ConvexHull2D	m_convexHull;

	//For broad-phase checks using bitBuckets
	IntVec2				m_bitFieldsXY;
};
