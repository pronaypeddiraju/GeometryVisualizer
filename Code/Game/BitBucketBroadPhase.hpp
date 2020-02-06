#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/Ray2D.hpp"
#include <vector>

//Have all your broadphase check functions here
//Like identify region
//Mark bits for polygon
//Do regions overlap
//Get region for ray

//------------------------------------------------------------------------------------------------------------------------------
struct Region
{
	Vec2 m_mins;
	Vec2 m_maxs;
	IntVec2 m_RegionID;
};

//------------------------------------------------------------------------------------------------------------------------------
class BitFieldBroadPhase
{
public:
	BitFieldBroadPhase();
	~BitFieldBroadPhase();

	IntVec2		GetRegionForConvexPoly(const ConvexPoly2D& polygon) const;
	IntVec2		GetRegionIDForMinMaxs(const Vec2& shapeMins, const Vec2& shapeMaxs) const;
	IntVec2		GetRegionForRay(const Ray2D& ray) const;
	
	void		SetWorldDimensions(const Vec2& mins, const Vec2& maxs);

	void		MakeRegionsForWorld();

private:
	Vec2		m_worldMins;
	Vec2		m_worldMaxs;

	float		m_xDelta;
	float		m_yDelta;

	std::vector<Region>	m_regions;
};

//------------------------------------------------------------------------------------------------------------------------------
