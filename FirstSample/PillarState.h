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

	myCoor3D PillarState::middleFilter(myCoor3D pillarCoor);

	myCoor3D getPillarCoor(const cameraParam myCamParam, const PillarIndex pillarToFind);


private:
	const myCoor3D pillarWorldCoor[7] =
	{
		myCoor3D( 7550, 4075, 1000 ),
		myCoor3D( 3525, 7075, 500 ),
		myCoor3D( 5525, 7075, 1000 ),
		myCoor3D( 7525, 7075, 1500 ),
		myCoor3D( 9525, 7075, 1000 ),
		myCoor3D( 11525, 7075, 500 ),
		myCoor3D( 7550, 10075, 1000 ),
	};

	const void *pConfidenceData;
	const void *pDepthData;
	HObject depthImage;
};

