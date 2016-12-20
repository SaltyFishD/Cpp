#include <cMath>
#include "PillarState.h"

#include "MyHalconFunctions.h"

extern HTuple hv_WindowHandle;
CToFCamera::Coord3D PillarState::middleFilter(CToFCamera::Coord3D pillarCoor)
{
	static const int FILTERSIZE = 30;
	static CToFCamera::Coord3D sFCoor[FILTERSIZE];
	CToFCamera::Coord3D sortedCoor[FILTERSIZE];
	static int dataCount = 0;
	static bool withTenData = true;
	CToFCamera::Coord3D resultCoor;

	if (dataCount > FILTERSIZE - 1)
	{
		withTenData = false;
		dataCount = 0;
	}
	sFCoor[dataCount] = pillarCoor;
	if (withTenData)
	{
		for (int i = 0; i < dataCount + 1; i++)
		{
			sortedCoor[i] = sFCoor[i];
		}

		for (int j = 0; j < dataCount; j++)
		{
			for (int k = j + 1; k < dataCount + 1; k++)
			{
				if (sortedCoor[k].y < sortedCoor[j].y)
				{
					CToFCamera::Coord3D tmp = sortedCoor[k];
					sortedCoor[k] = sortedCoor[j];
					sortedCoor[j] = tmp;
				}
			}
		}
		resultCoor = sortedCoor[dataCount / 2];
	}

	else
	{
		for (int i = 0; i < FILTERSIZE; i++)
		{
			sortedCoor[i] = sFCoor[i];
		}

		for (int j = 0; j < FILTERSIZE - 1; j++)
		{
			for (int k = j + 1; k < FILTERSIZE; k++)
			{
				if (sortedCoor[k].y < sortedCoor[j].y)
				{
					CToFCamera::Coord3D tmp = sortedCoor[k];
					sortedCoor[k] = sortedCoor[j];
					sortedCoor[j] = tmp;
				}
			}
		}
		resultCoor = sortedCoor[FILTERSIZE / 2];
	}
	dataCount++;
	return resultCoor;
}

CToFCamera::Coord3D PillarState::getPillarCoor(const cameraParam myCamParam, const PillarIndex pillarToFind)
{
	CToFCamera::Coord3D calculateCoor = calculatePillarCoor(myCamParam, pillarToFind);
	float pillarRow, pillarColumn;
	bool notBeyond = CameraCoorToPixelCoor(calculateCoor, &pillarRow, &pillarColumn);
	//pillarColumn = width - pillarColumn;
	SetColor(hv_WindowHandle, "red");
	HObject RectSB;
	GenRectangle2(&RectSB, pillarRow, pillarColumn, 0, 4, 4);
	if (HDevWindowStack::IsOpen())
		DispObj(RectSB, HDevWindowStack::GetActive());
	//cout << pillarRow << "    " << pillarColumn << endl;

	if (pillarColumn - 25 < 0 && pillarColumn + 25 > 480) 
		notBeyond = false;
	//cout << calculateCoor.x << " " << calculateCoor.y << " " << calculateCoor.z << endl;
	if (notBeyond)
	{
		float rowStart, columnStart, rowEnd, columnEnd;
		rowStart = pillarRow - 10;
		rowEnd = 479;
		columnStart = pillarColumn - 25;
		columnEnd = pillarColumn + 25;

		HObject pillarRect;
		GenRectangle1(&pillarRect, (HTuple)rowStart, (HTuple)columnStart, (HTuple)rowEnd, (HTuple)columnEnd);
		HObject pillarImage;
		ReduceDomain(depthImage, pillarRect, &pillarImage);
		HObject pillarRegion;
		Threshold(pillarImage, &pillarRegion, (HTuple)(calculateCoor.y - 500), (HTuple)(calculateCoor.y + 500));
		HObject ho_ConnectedRegions;
		Connection(pillarRegion, &ho_ConnectedRegions);

		HTuple hv_RegionNum;
		CountObj(ho_ConnectedRegions, &hv_RegionNum);
		if (hv_RegionNum.D() < 1)
		{
			disp_message(hv_WindowHandle, "没检测到柱子", "window", 240, 320, "black", "true");
			CToFCamera::Coord3D falseCoor;
			falseCoor.x = -2;
			falseCoor.y = -2;
			falseCoor.y = -2;
			return falseCoor;
		}

		HObject ho_SelectedRegions;
		SelectShapeStd(ho_ConnectedRegions, &ho_SelectedRegions, "max_area", 90);
		HTuple a,r,c;
		AreaCenter(ho_ConnectedRegions, &a, &r, &c);
		HObject plRegion;
		/*****************待修改**********/
		SetColor(hv_WindowHandle, "green");
		findRegion(depthImage, &plRegion, r, c, 500);
		if (HDevWindowStack::IsOpen())
				DispObj(plRegion, HDevWindowStack::GetActive());
		/*****************************************************************/
		HTuple  hv_Value;
		RegionFeatures(ho_ConnectedRegions, ((HTuple("row1").Append("column1")).Append("column2")),
			&hv_Value);
		
		int pillarPixelColumn = (hv_Value[1].D() + hv_Value[2].D()) / 2;
		int PillarPixelRow = hv_Value[0].D() + 5;

		disp_message(hv_WindowHandle, "台", "window", PillarPixelRow - 15, pillarPixelColumn, "black", "true");

		CToFCamera::Coord3D pillarCoor;
		pillarCoor.x = PillarPixelRow;
		pillarCoor.y = pillarPixelColumn;
		pillarCoor.z = 0;
		return pillarCoor;
	}
	else
	{
		disp_message(hv_WindowHandle, "柱子不在屏幕里", "window", 240, 320, "black", "true");

		CToFCamera::Coord3D falseCoor;
		falseCoor.x = -1;
		falseCoor.y = -1;
		falseCoor.y = -1;
		return falseCoor;
	}
}

CToFCamera::Coord3D PillarState::calculatePillarCoor(const cameraParam& myCamParam, const PillarIndex pillarToFind)
{
	CToFCamera::Coord3D relativeCoor, CameraCoor;
	if (!relativeCoor.IsValid())
	{
		CameraCoor.x = 0;
		CameraCoor.y = 0;
		CameraCoor.z = 0;
		return CameraCoor;
	}
	//相对坐标
	relativeCoor.x = pillarWorldCoor[pillarToFind][0] - myCamParam.worldX;

#ifdef BLUETEAM
	relativeCoor.x = -relativeCoor.x;
#endif

#ifdef REDTEAM
	relativeCoor.x = relativeCoor.x;
#endif

	relativeCoor.y = pillarWorldCoor[pillarToFind][1] - myCamParam.worldY;
	relativeCoor.z = pillarWorldCoor[pillarToFind][2] - myCamParam.worldZ;
	//转换为相机坐标
	CameraCoor.x = relativeCoor.x * cos(myCamParam.yaw / 180 * 3.14159) + relativeCoor.y * sin(myCamParam.yaw / 180 * 3.14159);

	CameraCoor.y = relativeCoor.x * (-sin(myCamParam.yaw / 180 * 3.14159) * cos(myCamParam.pitch / 180 * 3.14159)) +
		relativeCoor.y * (cos(myCamParam.yaw / 180 * 3.14159) * cos(myCamParam.pitch / 180 * 3.14159)) + relativeCoor.z * sin(myCamParam.pitch / 180 * 3.14159);
	CameraCoor.z = relativeCoor.x * (sin(myCamParam.yaw / 180 * 3.14159) * sin(myCamParam.pitch / 180 * 3.14159)) +
		relativeCoor.y * (-cos(myCamParam.yaw / 180 * 3.14159) * sin(myCamParam.pitch / 180 * 3.14159)) + relativeCoor.z * cos(myCamParam.pitch / 180 * 3.14159);
	return CameraCoor;

}



