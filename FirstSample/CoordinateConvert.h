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

using namespace GenTLConsumerImplHelper;
bool CameraCoorToPixelCoor(CToFCamera::Coord3D CameraCoor, float* Row, float* Column);
CToFCamera::Coord3D WorldCoorToCameraCoor(cameraParam _cameraParam, CToFCamera::Coord3D WorldCoor);
CToFCamera::Coord3D CameraCoorToWorldCoor(cameraParam _cameraParam, CToFCamera::Coord3D CameraCoor);
CToFCamera::Coord3D PixelCoorToCameraCoor(float Row, float Column, float z);
