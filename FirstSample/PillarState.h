#pragma once

#include <ConsumerImplHelper/ToFCamera.h>

#include "HalconCpp.h"
#include "HDevThread.h"
#include "CoordinateConvert.h"

#define REDTEAM
using namespace GenTLConsumerImplHelper;
using namespace HalconCpp;
using namespace std;

class PillarState
{
public:
	enum PillarIndex
	{
		nearPillar = 0,
		mostLeftPillar,
		secondLeftPillar,
		middlePillar,
		secondRightPillar,
		mostRightPillar,
		farPillar
	};
	
	void updateCameraData(HObject newImage, const void* confidence, const void* depth)
	{
		depthImage = newImage;
		pConfidenceData = confidence;
		pDepthData = depth;
	}

	CToFCamera::Coord3D PillarState::middleFilter(CToFCamera::Coord3D pillarCoor);

	CToFCamera::Coord3D getPillarCoor(const cameraParam myCamParam, const PillarIndex pillarToFind);


private:
	const int pillarWorldCoor[7][3] =
	{
		{ 7550, 4075, 1000 },
		{ 3525, 7075, 500 },
		{ 5525, 7075, 1000 },
		{ 7525, 7075, 1500 },
		{ 9525, 7075, 1000 },
		{ 11525, 7075, 500 },
		{ 7550, 10075, 1000 },
	};

	const void *pConfidenceData;
	const void *pDepthData;
	HObject depthImage;
	CToFCamera::Coord3D calculatePillarCoor(const cameraParam& myCamParam, const PillarIndex pillarToFind);
};

