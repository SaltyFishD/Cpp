#pragma once

#include <ConsumerImplHelper/ToFCamera.h>

const int height = 480, width = 640;
const float hFov = 57, vFov = 42;

struct cameraParam
{
	float worldX;
	float worldY;
	float worldZ;
	float pitch;
	float yaw;
};

struct myCoor3D
{
	float x;
	float y;
	float z;
	bool IsValid() const { return z == z; }  // check for NAN

	myCoor3D(float _x = 0, float _y = 0, float _z = 0)
	{
		x = _x;
		y = _y;
		z = _z;
	}
};

using namespace GenTLConsumerImplHelper;
bool CameraCoorToPixelCoor(myCoor3D CameraCoor, float* Row, float* Column);
myCoor3D WorldCoorToCameraCoor(cameraParam _cameraParam, myCoor3D WorldCoor);
myCoor3D CameraCoorToWorldCoor(cameraParam _cameraParam, myCoor3D CameraCoor);
myCoor3D PixelCoorToCameraCoor(float Row, float Column, float z);
