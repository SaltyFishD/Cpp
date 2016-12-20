#include "CoordinateConvert.h"
#define REDTEAM

const double M_PI = 3.14159265358979323846;

bool CameraCoorToPixelCoor(CToFCamera::Coord3D CameraCoor, float* Row, float* Column)
{
	float thetaRow, thetaColumn;
	thetaRow = atan(CameraCoor.y / CameraCoor.z) / M_PI * 180;
	thetaColumn = atan(CameraCoor.x / CameraCoor.z) / M_PI * 180;
	*Row = thetaRow * height / vFov + height / 2;
	*Column = thetaColumn * width / hFov + width / 2;

	//�ж������ǲ�������Ļ��
	if (*Row < 0 || *Row>height || *Column<0 || *Column>width)
		return false;
	return true;
}

CToFCamera::Coord3D WorldCoorToCameraCoor(cameraParam _cameraParam, CToFCamera::Coord3D WorldCoor)
{
	CToFCamera::Coord3D relativeCoor, CameraCoor;
	if (!relativeCoor.IsValid())
	{
		CameraCoor.x = 0;
		CameraCoor.y = 0;
		CameraCoor.z = 0;
		return CameraCoor;
	}
	//�������
#ifdef REDTEAM
	relativeCoor.x = _cameraParam.worldX - WorldCoor.x;
#endif
#ifdef BLUETEAM
	relativeCoor.x = WorldCoor.x - _cameraParam.worldX;
#endif
	relativeCoor.y = WorldCoor.y - _cameraParam.worldY;
	relativeCoor.z = WorldCoor.z - _cameraParam.worldZ;
	CameraCoor.x = relativeCoor.x * cos(_cameraParam.yaw / 180 * M_PI) + relativeCoor.y * sin(_cameraParam.yaw / 180 * M_PI);

	CameraCoor.z = relativeCoor.x * (-sin(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) +
		relativeCoor.y * (cos(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + relativeCoor.z * sin(_cameraParam.pitch / 180 * M_PI);
	CameraCoor.y = -(relativeCoor.x * (sin(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI)) +
		relativeCoor.y * (-cos(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI)) + relativeCoor.z * cos(_cameraParam.pitch / 180 * M_PI));


	////ת��Ϊ�������
	//CameraCoor.x = relativeCoor.x * cos(_cameraParam.yaw / 180 * M_PI) + relativeCoor.y * sin(_cameraParam.yaw / 180 * M_PI);

	//CameraCoor.y = relativeCoor.x * (-sin(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) +
	//	relativeCoor.y * (cos(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + relativeCoor.z * sin(_cameraParam.pitch / 180 * M_PI);
	//CameraCoor.z = relativeCoor.x * (sin(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI)) +
	//	relativeCoor.y * (-cos(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI)) + relativeCoor.z * cos(_cameraParam.pitch / 180 * M_PI);

	return CameraCoor;
}

CToFCamera::Coord3D CameraCoorToWorldCoor(cameraParam _cameraParam, CToFCamera::Coord3D CameraCoor)
{
	CToFCamera::Coord3D relativeCoor, WorldCoor;
	if (!relativeCoor.IsValid())
	{
		WorldCoor.x = 0;
		WorldCoor.y = 0;
		WorldCoor.z = 0;
		return WorldCoor;
	}
	float tmp;
	tmp = CameraCoor.y;
	CameraCoor.y = CameraCoor.z;
	CameraCoor.z = -tmp;
	relativeCoor.x = CameraCoor.x * cos(_cameraParam.yaw / 180 * M_PI) + CameraCoor.y * (-sin(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + CameraCoor.z * (sin(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI));
	relativeCoor.y = CameraCoor.x * sin(_cameraParam.yaw / 180 * M_PI) + CameraCoor.y * (cos(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + CameraCoor.z * (-cos(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI));
	relativeCoor.z = CameraCoor.y * sin(_cameraParam.pitch / 180 * M_PI) + CameraCoor.z * cos(_cameraParam.pitch / 180 * M_PI);
#ifdef REDTEAM
	WorldCoor.x = -relativeCoor.x + _cameraParam.worldX;
#endif
#ifdef BLUETEAM
	WorldCoor.x = relativeCoor.x + _cameraParam.worldX;
#endif
	WorldCoor.y = relativeCoor.y + _cameraParam.worldY;
	WorldCoor.z = relativeCoor.z + _cameraParam.worldZ;
	//relativeCoor.x = CameraCoor.x * cos(_cameraParam.yaw / 180 * M_PI) + CameraCoor.y * (-sin(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + CameraCoor.z * (sin(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI));
	//relativeCoor.y = CameraCoor.x * sin(_cameraParam.yaw / 180 * M_PI) + CameraCoor.y * (cos(_cameraParam.yaw / 180 * M_PI) * cos(_cameraParam.pitch / 180 * M_PI)) + CameraCoor.z * (-cos(_cameraParam.yaw / 180 * M_PI) * sin(_cameraParam.pitch / 180 * M_PI));
	//relativeCoor.z = CameraCoor.y * sin(_cameraParam.pitch / 180 * M_PI) + CameraCoor.z * cos(_cameraParam.pitch / 180 * M_PI);
	//WorldCoor.x = relativeCoor.x + _cameraParam.worldX;
	//WorldCoor.y = relativeCoor.y + _cameraParam.worldY;
	//WorldCoor.z = relativeCoor.z + _cameraParam.worldZ;
	return WorldCoor;
}

CToFCamera::Coord3D PixelCoorToCameraCoor(float Row, float Column, float z)
{
	CToFCamera::Coord3D CameraCoor;
	float thetaRow, thetaColumn;
	thetaRow = (Row - height / 2 + 3)*vFov / height;
	thetaColumn = (Column - width / 2 + 14)*hFov / width;
	CameraCoor.x = tan(thetaColumn / 180 * M_PI) * z;
	CameraCoor.y = tan(thetaRow / 180 * M_PI) * z;
	CameraCoor.z = z;
	return CameraCoor;
}
