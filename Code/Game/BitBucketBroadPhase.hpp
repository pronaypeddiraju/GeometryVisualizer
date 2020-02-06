#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
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

	IntVec2		GetRegionForConvexHull(const ConvexHull2D& hull);
	void		SetWorldDimensions(const Vec2& mins, const Vec2& maxs);

	void		MakeRegionsForWorld();

private:
	Vec2		m_worldMins;
	Vec2		m_worldMaxs;

	std::vector<Region>	m_regions;
};

//------------------------------------------------------------------------------------------------------------------------------
