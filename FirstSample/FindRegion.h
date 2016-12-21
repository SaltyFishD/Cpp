#pragma once
#include <ConsumerImplHelper/ToFCamera.h>
#include "HalconCpp.h"
#include "HDevThread.h"
#include "CoordinateConvert.h"

using namespace GenTLConsumerImplHelper;
using namespace HalconCpp;
using namespace std;

class FindRegion
{
public:
	/*���̱��*/
	const int saucerIndex;

	~FindRegion() {};

	bool thisHasLost()
	{
		return hasLost;
	}

	FindRegion(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval) :
		lastRow(Row), lastColumn(Column), lastArea(Area), lastGrayval(Grayval), saucerIndex(saucerCount)
	{
		saucerCount++;
	}

	friend bool operator == (FindRegion& lhs, FindRegion& rhs)
	{
		return (abs(lhs.lastRow.D() - rhs.lastRow.D() < 10) && abs(lhs.lastColumn.D() - rhs.lastColumn.D() < 10) && abs(lhs.lastGrayval.D() - rhs.lastGrayval.D()) < 100);
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
	//���ü�����Ŀ��
	const int DETECTMINLIMIT = 100;
	const int DETECTMAXLIMIT = 3000;
	size_t regionNum;

	//��region������Ҷ���
	void pushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval);
public:
	FindRegionList() {};

	std::vector<HObject> RegionsFound(HObject &Image);
	std::vector<FindRegion*> findRegionList;
	void detectRegion(HObject &Image);

	~FindRegionList() {};
};

