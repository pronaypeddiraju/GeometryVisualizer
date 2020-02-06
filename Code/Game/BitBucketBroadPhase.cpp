#include "Game/BitBucketBroadPhase.hpp"
#include "Engine/Math/MathUtils.hpp"

//------------------------------------------------------------------------------------------------------------------------------
BitFieldBroadPhase::BitFieldBroadPhase()
{

}

//------------------------------------------------------------------------------------------------------------------------------
BitFieldBroadPhase::~BitFieldBroadPhase()
{

}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 BitFieldBroadPhase::GetRegionForConvexPoly(const ConvexPoly2D& polygon) const
{
	//Get the mins and maxes on both axis for all points in the convexPoly2D

	const std::vector<Vec2>& points = polygon.GetConvexPoly2DPoints();

	Vec2 shapeMins = points[0];
	Vec2 shapeMaxs = points[0];

	for (int pointIndex = 1; pointIndex < points.size(); pointIndex++)
	{
		//Update Mins
		if (points[pointIndex].x < shapeMins.x)
		{
			shapeMins.x = points[pointIndex].x;
		}

		if (points[pointIndex].y < shapeMins.y)
		{
			shapeMins.y = points[pointIndex].y;
		}

		//Update Maxs
		if (points[pointIndex].x > shapeMaxs.x)
		{
			shapeMaxs.x = points[pointIndex].x;
		}

		if (points[pointIndex].y > shapeMaxs.y)
		{
			shapeMaxs.y = points[pointIndex].y;
		}
	}

	IntVec2 regionID = GetRegionIDForMinMaxs(shapeMins, shapeMaxs);
	return regionID;
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 BitFieldBroadPhase::GetRegionIDForMinMaxs(const Vec2& shapeMins, const Vec2& shapeMaxs) const
{
	//Check what regions encompass the shape and return those as bit fields in the IntVec2
	int minXCell = (shapeMins.x) / m_xDelta;
	int minYCell = (shapeMins.y) / m_yDelta;

	int maxXCell = (shapeMaxs.x) / m_xDelta;
	int maxYCell = (shapeMaxs.y) / m_yDelta;

	//We now need to get all the bit flags for the regions between minX and maxX and add OR them to define the region of the shape
	//Same for Y which will be the y component in the IntVec2

	IntVec2 bitFlags = IntVec2::ZERO;
	for (int i = minXCell; i <= maxXCell; i++)
	{
		bitFlags.x += BIT_FLAG(i);
	}

	for (int j = minYCell; j <= maxYCell; j++)
	{
		bitFlags.y += BIT_FLAG(j);
	}

	return bitFlags;
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 BitFieldBroadPhase::GetRegionForRay(const Ray2D& ray) const
{
	//This is tricky as our rays are technically infinite
	//Take the world to be a convex hull we are trying to solve collisions for, then we can get a hit point on 1 of the 4 planes of the world
	//This hit point would allow us to easily identify the region we are solving for

	ConvexHull2D hull;
	hull.PushPlane(Plane2D(Vec2::UP, -m_worldMaxs.y));
	hull.PushPlane(Plane2D(Vec2::DOWN, 0));
	hull.PushPlane(Plane2D(Vec2::LEFT, 0));
	hull.PushPlane(Plane2D(Vec2::RIGHT, -m_worldMaxs.x));

	RayHit2D hit;
	uint hitCount = Raycast(&hit, ray, hull);

	ASSERT_OR_DIE(hitCount > 0, "The ray cast to world did not recieve a result");

	Vec2 endPos = hit.m_hitPoint;
	Vec2 startPos = ray.m_start;
	
	Vec2 minBounds;
	minBounds.x = GetLowerValue(endPos.x, startPos.x);
	minBounds.y = GetLowerValue(endPos.y, startPos.y);

	Vec2 maxBounds;
	maxBounds.x = GetHigherValue(endPos.x, startPos.x);
	maxBounds.y = GetHigherValue(endPos.y, startPos.y);

	//Now pass the region identified for the ray and get the bitFields
	IntVec2 regionID = GetRegionIDForMinMaxs(minBounds, maxBounds);
	return regionID;
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
	m_xDelta = (m_worldMaxs.x - m_worldMins.x) / 32.f;
	m_yDelta = (m_worldMaxs.y - m_worldMins.y) / 32.f;

	Vec2 yStepVec = Vec2(0.f, m_yDelta);
	Vec2 xStepVec = Vec2(m_xDelta, 0.f);

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

			region.m_RegionID.x = BIT_FLAG(xIndex);
			region.m_RegionID.y = BIT_FLAG(yIndex);

			m_regions.push_back(region);

			xMins += xStepVec;
			xMaxs += xStepVec;
		}

		yMins += yStepVec;
		yMaxs += yStepVec;
	}
}
