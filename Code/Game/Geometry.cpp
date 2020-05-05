//------------------------------------------------------------------------------------------------------------------------------
#include "Game/Geometry.hpp"
//Engine Systems

//Game Systems

//------------------------------------------------------------------------------------------------------------------------------
Geometry::Geometry()
{
	//Does nothing, just creates an empty Geometry object
}

//------------------------------------------------------------------------------------------------------------------------------
Geometry::Geometry(const std::vector<Vec2>& constructPoints)
{
	//Construct the ConvexPoly2D using the constructPoints
	m_convexPoly = ConvexPoly2D(constructPoints);

	//Construct the ConvexHull2D using the ConvexPoly2D
	m_convexHull.MakeConvexHullFromConvexPolyon(m_convexPoly);
}

//------------------------------------------------------------------------------------------------------------------------------
Geometry::Geometry(const std::vector<Plane2D>& constructPlanes)
{
	//Construct the ConvexHull2D using the constructPlanes
	m_convexHull = ConvexHull2D(constructPlanes);
}

//------------------------------------------------------------------------------------------------------------------------------
Geometry::Geometry(const ConvexPoly2D& poly, const ConvexHull2D& hull)
{
	m_convexHull = hull;
	m_convexPoly = poly;
}

//------------------------------------------------------------------------------------------------------------------------------
Geometry::~Geometry()
{
	
}

//------------------------------------------------------------------------------------------------------------------------------
const ConvexPoly2D& Geometry::GetConvexPoly2D() const
{
	return m_convexPoly;
}

//------------------------------------------------------------------------------------------------------------------------------
const ConvexHull2D& Geometry::GetConvexHull2D() const
{
	return m_convexHull;
}

//------------------------------------------------------------------------------------------------------------------------------
void Geometry::SetBitFieldsForBitBucketBroadPhase(const IntVec2& bitFields)
{
	m_bitFieldsXY = bitFields;
}

//------------------------------------------------------------------------------------------------------------------------------
const IntVec2& Geometry::GetBitFields() const
{
	return m_bitFieldsXY;
}

//------------------------------------------------------------------------------------------------------------------------------
void Geometry::MakeHullFromOwningPolygon()
{
	m_convexHull.MakeConvexHullFromConvexPolyon(m_convexPoly);
}

