#include "stdafx.h"
#include <cMath>
#include "PillarState.h"

#include "MyHalconFunctions.h"

extern HTuple hv_WindowHandle;
myCoor3D PillarState::middleFilter(myCoor3D pillarCoor)
{
	static const int FILTERSIZE = 30;
	static myCoor3D sFCoor[FILTERSIZE];
	myCoor3D sortedCoor[FILTERSIZE];
	static int dataCount = 0;
	static bool withTenData = true;
	myCoor3D resultCoor;

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
					myCoor3D tmp = sortedCoor[k];
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
					myCoor3D tmp = sortedCoor[k];
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

myCoor3D PillarState::getPillarCoor(const cameraParam myCamParam, const PillarIndex pillarToFind)
{
	myCoor3D calculateCoor = WorldCoorToCameraCoor(myCamParam, pillarWorldCoor[pillarToFind]);
	cout << calculateCoor.x << " " << calculateCoor.y << " " << calculateCoor.z << endl;

	float pillarRow, pillarColumn;
	bool notBeyond = CameraCoorToPixelCoor(calculateCoor, &pillarRow, &pillarColumn);
	//pillarColumn = width - pillarColumn;
	SetColor(hv_WindowHandle, "red");
	HObject RectSB;
	GenRectangle2(&RectSB, pillarRow, pillarColumn, 0, 4, 4);
	if (HDevWindowStack::IsOpen())
		DispObj(RectSB, HDevWindowStack::GetActive());
	cout << pillarRow << "    " << pillarColumn << endl;

	if (pillarColumn - 25 < 0 && pillarColumn + 25 > 480) 
		notBeyond = false;

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
		Threshold(pillarImage, &pillarRegion, (HTuple)(calculateCoor.z - 800), (HTuple)(calculateCoor.z + 800));
		HObject ho_ConnectedRegions;
		Connection(pillarRegion, &ho_ConnectedRegions);

		HTuple hv_RegionNum;
		CountObj(ho_ConnectedRegions, &hv_RegionNum);
		if (hv_RegionNum.D() < 1)
		{
			disp_message(hv_WindowHandle, "没检测到柱子", "window", 240, 320, "black", "true");
			myCoor3D falseCoor;
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

		myCoor3D pillarCoor;
		pillarCoor.x = PillarPixelRow;
		pillarCoor.y = pillarPixelColumn;
		pillarCoor.z = 0;
		return pillarCoor;
	}
	else
	{
		disp_message(hv_WindowHandle, "柱子不在屏幕里", "window", 240, 320, "black", "true");

		myCoor3D falseCoor;
		falseCoor.x = -1;
		falseCoor.y = -1;
		falseCoor.y = -1;
		return falseCoor;
	}
}