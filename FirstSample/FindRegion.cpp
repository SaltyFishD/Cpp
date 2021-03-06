#include "FindRegion.h"
#include "MyHalconFunctions.h"

extern CToFCamera::Coord3D farPillarCoordinate;

int FindRegion::saucerCount = 1;

HObject FindRegion::findNext(HObject &Image)
{
	xv = vx.D(), yv = vy.D(), zv = vz.D();
	lx = lastColumn.D(), ly = lastRow.D(), la = lastArea.D(), lg = lastGrayval.D();
	++findTimesCount;
	HObject Rectangle, ImageReduced, Region, BackgroundRegions, EmptyRegion, SelectShapeRegion;
	HTuple Number;
	GenEmptyRegion(&EmptyRegion);

	if (findAgainTimes == 0)
	{
		hasLost = true;
		return EmptyRegion;
		;//删除这个类
	}
	if (findTimesCount)
	{
		if (lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) > 32755 || lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) < -32755 ||
			lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes) > 32748 || lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes) < -32748 ||
			lastGrayval.D() < 1000)
		{
			hasLost = true;
			return EmptyRegion;
			//飞出屏幕外，干掉这个类
		}

		GenRectangle2(&Rectangle, lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes), lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes), 0, 190000.0 / lastGrayval.D(), 120000.0 / lastGrayval.D());
		if (lastArea.D() < 350)
			GenRectangle2(&Rectangle, lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes), lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes), 0, 330 / 9, 330 / 11);
		if (lastArea.D() < 50)
			GenRectangle2(&Rectangle, lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes), lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes), 0, 240 / 9, 240 / 13);
		ReduceDomain(Image, Rectangle, &ImageReduced);
		if (lastGrayval.D() - threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes) > 65535 || lastGrayval.D() + threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes) < 0)
		{
			hasLost = true;
			return EmptyRegion;
			//飞出屏幕外，干掉这个类
		}

		Threshold(ImageReduced, &Region, lastGrayval - threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes), lastGrayval + threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes));
		Connection(Region, &BackgroundRegions);
		SelectShape(BackgroundRegions, &SelectShapeRegion, "area", "and", 233.180000 / (lastGrayval.D() / 1000 * (lastGrayval.D() / 1000)), 15177.840000 / (lastGrayval.D() / 1000 * (lastGrayval.D() / 1000)));
		//SelectShape(SelectShapeRegion, &SelectShapeRegion, "row", "and", 0, 450);
		SelectShape(BackgroundRegions, &SelectShapeRegion, "row", "and", lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) - 20, lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) + 20);


		Union1(SelectShapeRegion, &Region);
		CountObj(Region, &Number);

		//HTuple hv_area1, hv_row1, hv_column1, number;
		//AreaCenter(SelectShapeRegion, &hv_area1, &hv_row1, &hv_column1);
		//HTuple topRow = 479, topColumn = 639;
		//for (HTuple i = 0;i < hv_area1.Length();i = i + 1)
		//{

		//	if (hv_row1[i].D() < topRow.D())
		//	{
		//		topRow = hv_row1[i];
		//		topColumn = hv_column1[i];
		//	}
		//}
		////SelectShape(Region, &Region, "area", "and", 9999, 9999);
		//if (topRow.D() < 479)
		//{
		//	findRegion(Image, &Region, topRow, topColumn, 500);
		//	HTuple A, R, C;
		//	CountObj(Region, &Number);
		//	AreaCenter(Region, &A, &R, &C);
		//	if (A.D() < lastArea.D() * 0.4 || A.D() > lastArea.D() * 1.5)
		//	{
		//		SelectShape(Region, &Region, "area", "and", 9999, 9999);
		//		Number = 0;
		//	}
		//}

		if (!(Number == 0))
		{
			HTuple A, R, C;
			AreaCenter(Region, &A, &R, &C);
			findRegion(Image, &Region, R, C, 1500);
			AreaCenter(Region, &A, &R, &C);
			if (A.D() < lastArea.D() * 0.4 || A.D() > lastArea.D() * 1.6)
			{
				Number = 0;
			}
		}

		if (Number == 0)
		{
			--findAgainTimes;
			return EmptyRegion;
		}

		else//更新数据
		{

			Result = Region;
			HTuple Area, Row, Column, Grayval, Deviation;
			AreaCenter(Result, &Area, &Row, &Column);
			HalconCpp::Intensity(Result, Image, &Grayval, &Deviation);

			vx = (Column - lastColumn) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
			vy = (Row - lastRow) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
			vz = (Grayval - lastGrayval) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
			lastArea = Area * 0.65 + lastArea * lastGrayval * lastGrayval / Grayval / Grayval*0.35;
			lastColumn = Column;
			lastRow = Row;
			lastGrayval = Grayval;
			findAgainTimes = MAXFINDAGAINTIMES;
			return Result;

		}
	}
}

int FindRegion::getOffset(CToFCamera::Coord3D pillarCoor, float offset[2])
{
	static const int HEIGHTOFFSET = 300;
	static const int HEIGHTFILTRE = 200;
	//最后一次在平台上的飞盘的坐标的编号
	if (!regionTrack.size())
		return false;
	size_t lastCoorIndex = regionTrack.size() - 1;

	for (; lastCoorIndex > 0; lastCoorIndex--)
	{
		if (regionTrack[lastCoorIndex].y < pillarCoor.y - HEIGHTFILTRE && regionTrack[lastCoorIndex].z < pillarCoor.z + 300 /*&& abs(regionTrack[lastCoorIndex].x - pillarCoor.x ) < 300*/)
		{
			break;
		}
	}

	//lastCoorIndex--;
	//如果追踪点的数量太少 或者追踪的region过早丢失
	if (lastCoorIndex < 2)
		return 1;
	else if (pillarCoor.z - regionTrack[lastCoorIndex].z > 2000)
		return 2;


	//交点坐标
	CToFCamera::Coord3D intersectionCoor;
	float ratioThree[3];
	ratioThree[0] = (regionTrack[lastCoorIndex - 1].y - regionTrack[lastCoorIndex].y) /
		(regionTrack[lastCoorIndex].y - (pillarCoor.y - HEIGHTOFFSET));
	ratioThree[1] = (regionTrack[lastCoorIndex - 2].y - regionTrack[lastCoorIndex - 1].y) /
		(regionTrack[lastCoorIndex - 1].y - (pillarCoor.y - HEIGHTOFFSET));
	ratioThree[2] = (regionTrack[lastCoorIndex - 2].y - regionTrack[lastCoorIndex - 0].y) /
		(regionTrack[lastCoorIndex - 0].y - (pillarCoor.y - HEIGHTOFFSET));
	float ratio = (ratioThree[0] + ratioThree[1] + ratioThree[2]) / 3;
	intersectionCoor.x = regionTrack[lastCoorIndex].x - (regionTrack[lastCoorIndex - 1].x - regionTrack[lastCoorIndex].x) / ratio;
	intersectionCoor.y = pillarCoor.y;
	intersectionCoor.z = regionTrack[lastCoorIndex].z - (regionTrack[lastCoorIndex - 1].z - regionTrack[lastCoorIndex].z) / ratio;

	offset[0] = intersectionCoor.x - pillarCoor.x;
	offset[1] = intersectionCoor.z - pillarCoor.z;


	std::ofstream datafile;
	datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt", std::ios::app);
	datafile << "使用了倒数第" << regionTrack.size() - lastCoorIndex << " 个数据" << endl;
	datafile.close();
	return 0;
}

void FindRegionList::PushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval)
{
	FindRegion* tmp = new FindRegion(Row, Column, Area, Grayval);
	findRegionList.push_back(tmp);
}

std::vector<HObject> FindRegionList::RegionsFound(HObject &Image)
{
	std::vector<HObject> result;
	HObject emptyRegion, tmpObj;
	emptyRegion.GenEmptyObj();
	if (findRegionList.empty())
		return result;

	for (size_t i = 0; i < findRegionList.size(); i++)
	{
		//删两次
		for (size_t j = i + 1; j < findRegionList.size(); j++)
		{
			if ((*findRegionList[j]) == (*findRegionList[i]))
			{
				delete (findRegionList[j]);
				findRegionList.erase(findRegionList.begin() + j);
			}
		}

		//将返回的结果加入vector容器
		if (i < findRegionList.size())
			tmpObj = findRegionList[i]->findNext(Image);

		HTuple hv_Area, hv_Row, hv_Column;
		AreaCenter(tmpObj, &hv_Area, &hv_Row, &hv_Column);
		if (hv_Area.D() > 0)
		{
			result.push_back(tmpObj);
		}

		//删除重复的region
		for (size_t j = i + 1; j < findRegionList.size(); j++)
		{
			if ((*findRegionList[j]) == (*findRegionList[i]))
			{
				delete (findRegionList[j]);
				findRegionList.erase(findRegionList.begin() + j);
			}
		}

		try
		{
			if (findRegionList[i]->thisHasLost())
			{
				int trackSuccess;
				float offset[2];

				trackSuccess = findRegionList[i]->getOffset(farPillarCoordinate, offset);

				std::ofstream datafile;
				datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt", std::ios::app);
				if (trackSuccess == 0)
					datafile << "飞盘" << findRegionList[i]->saucerIndex << " x偏差: " << setw(2) << offset[0] << " z偏差: " << setw(2) << offset[1] << endl;
				else if (trackSuccess == 1)
					datafile << "飞盘" << findRegionList[i]->saucerIndex << "采集点太少或者是杂物" << endl;
				else if (trackSuccess == 2)
					datafile << "飞盘" << findRegionList[i]->saucerIndex << "过早跟丢" << endl;
				datafile << "飞盘" << findRegionList[i]->saucerIndex << " 已丢失！" << std::endl << std::endl;

				delete (findRegionList[i]);
				findRegionList.erase(findRegionList.begin() + i);


				datafile.close();
			}
		}
		catch (...)
		{
			std::cout << "delete error" << std::endl;
			std::cin.get();
		}
	}
	//auto it = std::unique(result.begin(), result.end());
	//result.erase(it, result.end());
	regionNum = result.size();
	return result;
}