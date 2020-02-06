#include "Game/BitBucketBroadPhase.hpp"

//------------------------------------------------------------------------------------------------------------------------------
BitFieldBroadPhase::BitFieldBroadPhase()
{

}

//------------------------------------------------------------------------------------------------------------------------------
BitFieldBroadPhase::~BitFieldBroadPhase()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void BitFieldBroadPhase::SetWorldDimensions(const Vec2& mins, const Vec2& maxs)
{
	m_worldMins = mins;
	m_worldMaxs = maxs;
}

//------------------------------------------------------------------------------------------------------------------------------
void BitFieldBroadPhase::MakeRegionsForWorld()
{
	float xStep = (m_worldMaxs.x - m_worldMins.x) / 32.f;
	float yStep = (m_worldMaxs.y - m_worldMins.y) / 32.f;

	Vec2 yStepVec = Vec2(0.f, yStep);
	Vec2 xStepVec = Vec2(xStep, 0.f);

	Vec2 yMins = m_worldMins;
	Vec2 yMaxs = m_worldMins + yStepVec;

	for (int yIndex = 0; yIndex < 32; yIndex++)
	{
		//Make vertical splits

		Vec2 xMins = m_worldMins;
		Vec2 xMaxs = m_worldMins + xStepVec;

		for (int xIndex = 0; xIndex < 32; xIndex++)
		{
			//Make horizontal splits
			Region region;
			region.m_mins = Vec2(xMins.x, yMins.y);
			region.m_maxs = Vec2(xMaxs.x, yMaxs.y);

			region.m_RegionID.x = xIndex;
			region.m_RegionID.y = yIndex;

			m_regions.push_back(region);

			xMins += xStepVec;
			xMaxs += xStepVec;
		}

		yMins += yStepVec;
		yMaxs += yStepVec;
	}
}
