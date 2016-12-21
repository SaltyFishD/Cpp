#pragma once
#include <ConsumerImplHelper/ToFCamera.h>
#include "HalconCpp.h"
#include "HDevThread.h"
#include "CoordinateConvert.h"
#include "stdafx.h"

using namespace GenTLConsumerImplHelper;
using namespace HalconCpp;
using namespace std;

class FindRegion
{
public:
	FindRegion(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval) 
		:lastRow(Row), lastColumn(Column), lastArea(Area), lastGrayval(Grayval), saucerIndex(saucerCount)
	{
		saucerCount++;
	}

	FindRegion(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval, cameraParam _cameraParam) 
		:lastABSWorldCoor(CameraCoorToWorldCoor(_cameraParam, PixelCoorToCameraCoor(Row.D(), Column.D(), Grayval.D()))), lastArea(Area), saucerIndex(saucerCount)
	{
		saucerCount++;
	}

	/*飞盘编号*/
	const int saucerIndex;

	bool thisHasLost()
	{
		return hasLost;
	}

	friend bool operator == (FindRegion& lhs, FindRegion& rhs)
	{
#ifdef NEWMETHOD 
		return (abs(lhs.lastABSWorldCoor.x - rhs.lastABSWorldCoor.x < 100) && abs(lhs.lastABSWorldCoor.y - rhs.lastABSWorldCoor.y < 100) && abs(lhs.lastABSWorldCoor.z - rhs.lastABSWorldCoor.z) < 100);
#else
		return (abs(lhs.lastRow.D() - rhs.lastRow.D() < 10) && abs(lhs.lastColumn.D() - rhs.lastColumn.D() < 10) && abs(lhs.lastGrayval.D() - rhs.lastGrayval.D()) < 100);
#endif
	}

	void getSpeed(int speed[3])
	{
		speed[0] = xv;
		speed[1] = yv;
		speed[2] = zv;
	}

	HObject findNext(HObject &Image);

	HObject findNext(HObject &Image, cameraParam _cameraParam);

	void recordRegionTrack(myCoor3D regionCoor)
	{
		regionTrack.push_back(regionCoor);
	}

	int getOffset(myCoor3D pillarCoor, float offset[2]);

	~FindRegion() {};
private:
	HTuple lastRow, lastColumn, lastArea, lastGrayval = 0, vx = 0, vy = 0, vz = 0;

	HObject Result;

	const int MAXFINDAGAINTIMES = 5;

	int findAgainTimes = MAXFINDAGAINTIMES;

	const int threahold = 700;

	int xv, yv, zv, lx, ly, lg, la;

	static int saucerCount;

	bool hasLost = false;

	int findTimesCount = 0;

	myCoor3D lastABSWorldCoor;

	vector<myCoor3D> regionTrack;
};

class FindRegionList
{
private:
	//设置检测区的宽度
	const int DETECTMINLIMIT = 100;
	const int DETECTMAXLIMIT = 2500;
	size_t regionNum;

	//将region加入查找队列
	void pushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval);
	//加入世界坐标的版本
	void pushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval, cameraParam mycamParam);
public:
	FindRegionList() {};

	std::vector<HObject> RegionsFound(HObject &Image);
	std::vector<FindRegion*> findRegionList;
	void detectRegion(HObject &Image);

	~FindRegionList() {};
};

