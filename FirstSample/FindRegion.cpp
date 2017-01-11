#include "stdafx.h"
#include "FindRegion.h"
#include "MyHalconFunctions.h"

//�ⲿ����
extern myCoor3D pillarCamCoor;
extern HTuple hv_WindowHandle;
extern cameraParam myCamParam;

//��̬��Ա����
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
		;//ɾ�������
	}
	if (findTimesCount)
	{
		if (lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) > 32755 || lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) < -32755 ||
			lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes) > 32748 || lastColumn + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes) < -32748 ||
			lastGrayval.D() < 1000)
		{
			hasLost = true;
			return EmptyRegion;
			//�ɳ���Ļ�⣬�ɵ������
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
			//�ɳ���Ļ�⣬�ɵ������
		}

		Threshold(ImageReduced, &Region, lastGrayval - threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes), lastGrayval + threahold + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes));
		Connection(Region, &BackgroundRegions);
		SelectShape(BackgroundRegions, &SelectShapeRegion, "area", "and", 233.180000 / (lastGrayval.D() / 1000 * (lastGrayval.D() / 1000)), 15177.840000 / (lastGrayval.D() / 1000 * (lastGrayval.D() / 1000)));
		//SelectShape(SelectShapeRegion, &SelectShapeRegion, "row", "and", 0, 450);
		SelectShape(BackgroundRegions, &SelectShapeRegion, "row", "and", lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) - 20, lastRow + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes) + 20);


		Union1(SelectShapeRegion, &Region);
		CountObj(Region, &Number);

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

		else//��������
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

HObject FindRegion::findNext(HObject &Image, cameraParam _cameraParam)
{
	//xv = vx.D(), yv = vy.D(), zv = vz.D();
	//lx = lastColumn.D(), ly = lastRow.D(), la = lastArea.D(), lg = lastGrayval.D();
	try
	{
		++findTimesCount;
		HObject Rectangle, ImageReduced, Region, BackgroundRegions, EmptyRegion, SelectShapeRegion;
		HTuple Number;
		GenEmptyRegion(&EmptyRegion);

		myCoor3D whereThisFunctionWillFind;
		whereThisFunctionWillFind.x = lastABSWorldCoor.x + vx * (MAXFINDAGAINTIMES + 1 - findAgainTimes);
		whereThisFunctionWillFind.y = lastABSWorldCoor.y + vy * (MAXFINDAGAINTIMES + 1 - findAgainTimes);
		whereThisFunctionWillFind.z = lastABSWorldCoor.z + vz * (MAXFINDAGAINTIMES + 1 - findAgainTimes);
		myCoor3D cameraCoor;
		cameraCoor = WorldCoorToCameraCoor(_cameraParam, whereThisFunctionWillFind);
		float Row, Column;
		if (!CameraCoorToPixelCoor(cameraCoor, &Row, &Column))
		{	//�ƶ�̫���Ƴ���Ļ��
			--findAgainTimes;
			return EmptyRegion;
		}

		if (findAgainTimes == 0)
		{
			hasLost = true;
			return EmptyRegion;
			;//ɾ�������
		}
		if (findTimesCount)
		{
			if (Row > 32755 || Row < -32755 || Column > 32748 || Column < -32748 ||
				cameraCoor.z < 1000)
			{
				hasLost = true;
				return EmptyRegion;
				//�ɳ���Ļ�⣬�ɵ������
			}

			GenRectangle2(&Rectangle, Row, Column, 0, 190000.0 / cameraCoor.z, 120000.0 / cameraCoor.z);
			if (lastArea.D() < 350)
				GenRectangle2(&Rectangle, Row, Column, 0, 330 / 9, 330 / 11);
			if (lastArea.D() < 50)
				GenRectangle2(&Rectangle, Row, Column, 0, 240 / 9, 240 / 13);
			ReduceDomain(Image, Rectangle, &ImageReduced);
			if (cameraCoor.z - threahold > 65535 || cameraCoor.z + threahold < 0)
			{
				hasLost = true;
				return EmptyRegion;
				//�ɳ���Ļ�⣬�ɵ������
			}

			Threshold(ImageReduced, &Region, cameraCoor.z - threahold, cameraCoor.z + threahold);
			Connection(Region, &BackgroundRegions);
			SelectShape(BackgroundRegions, &SelectShapeRegion, "area", "and", 233.180000 / (cameraCoor.z / 1000 * (cameraCoor.z / 1000)), 15177.840000 / (cameraCoor.z / 1000 * (cameraCoor.z / 1000)));
			//SelectShape(SelectShapeRegion, &SelectShapeRegion, "row", "and", 0, 450);
			SelectShape(SelectShapeRegion, &SelectShapeRegion, "row", "and", Row - 20, Row + 20);


			Union1(SelectShapeRegion, &Region);
			CountObj(Region, &Number);


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

			else//��������
			{

				Result = Region;
				HTuple Area, Row, Column, Grayval, Deviation;
				AreaCenter(Result, &Area, &Row, &Column);
				HalconCpp::Intensity(Result, Image, &Grayval, &Deviation);

				myCoor3D curABSWorldCoor = CameraCoorToWorldCoor(_cameraParam, PixelCoorToCameraCoor(Row.D(), Column.D(), Grayval.D()));
				vx = 0.3 * vx + 0.7 * (curABSWorldCoor.x - lastABSWorldCoor.x) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
				vy = 0.3 * vy + 0.7 * (curABSWorldCoor.y - lastABSWorldCoor.y) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
				vz = 0.3 * vz + 0.7 * (curABSWorldCoor.z - lastABSWorldCoor.z) / (MAXFINDAGAINTIMES + 1 - findAgainTimes);
				lastArea = Area * 0.65 + lastArea * lastABSWorldCoor.z * lastABSWorldCoor.z / curABSWorldCoor.z / curABSWorldCoor.z * 0.35;

				lastABSWorldCoor = curABSWorldCoor;
				//lastColumn = Column;
				//lastRow = Row;
				//lastGrayval = Grayval;
				findAgainTimes = MAXFINDAGAINTIMES;
				return Result;

			}
		}
	}
	catch(...)
	{
		cout << "�������" << endl;
		throw 1;
	}
}


int FindRegion::getOffset(myCoor3D pillarCoor, float offset[2])
{
	static const int HEIGHTOFFSET = 300;
	static const int HEIGHTFILTRE = 200;
	//���һ����ƽ̨�ϵķ��̵�����ı��
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
	//���׷�ٵ������̫�� ����׷�ٵ�region���綪ʧ
	if (lastCoorIndex < 2)
		return 1;
	else if (pillarCoor.z - regionTrack[lastCoorIndex].z > 2000)
		return 2;


	//��������
	myCoor3D intersectionCoor;
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
	datafile << "ʹ���˵�����" << regionTrack.size() - lastCoorIndex << " ������" << endl;
	datafile.close();
	return 0;
}

void FindRegionList::pushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval)
{
	FindRegion* tmp = new FindRegion(Row, Column, Area, Grayval);
	findRegionList.push_back(tmp);
}

void FindRegionList::pushRegionToFind(HTuple Row, HTuple Column, HTuple Area, HTuple Grayval, cameraParam mycamParam)
{
	FindRegion* tmp = new FindRegion(Row, Column, Area, Grayval, mycamParam);
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
	{try{
		//ɾ����
		for (size_t j = i + 1; j < findRegionList.size(); j++)
		{
			if ((*findRegionList[j]) == (*findRegionList[i]))
			{
				delete (findRegionList[j]);
				findRegionList.erase(findRegionList.begin() + j);
			}
		}
		}
		catch (...)
		{
			cout << "ɾ�����ε�ʱ�����" << endl;
			throw ERROR;
		}
		try
		{
		//�����صĽ������vector����
		if (i < findRegionList.size())
		{
#ifdef NEWMETHOD
			tmpObj = findRegionList[i]->findNext(Image,myCamParam);
#else
			tmpObj = findRegionList[i]->findNext(Image);
#endif
		}

		HTuple hv_Area, hv_Row, hv_Column;
		AreaCenter(tmpObj, &hv_Area, &hv_Row, &hv_Column);
		if (hv_Area.D() > 0)
		{
			result.push_back(tmpObj);
		}
		}
		catch (...)
		{
			cout << "�����صĽ������vector��������" << endl;
			throw ERROR;
		}
		try
		{
		//ɾ���ظ���region
		for (size_t j = i + 1; j < findRegionList.size(); j++)
		{
			if ((*findRegionList[j]) == (*findRegionList[i]))
			{
				delete (findRegionList[j]);
				findRegionList.erase(findRegionList.begin() + j);
			}
		}
		}
		catch (...)
		{
			cout << "ɾ���ظ���region����" << endl;
			throw ERROR;
		}
		if (findRegionList[i]->thisHasLost())
		{
			int trackSuccess;
			float offset[2];

			trackSuccess = findRegionList[i]->getOffset(pillarCamCoor, offset);

			std::ofstream datafile;
			datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt", std::ios::app);
			if (trackSuccess == 0)
				datafile << "����" << findRegionList[i]->saucerIndex << " xƫ��: " << setw(2) << offset[0] << " zƫ��: " << setw(2) << offset[1] << endl;
			else if (trackSuccess == 1)
				datafile << "����" << findRegionList[i]->saucerIndex << "�ɼ���̫�ٻ���������" << endl;
			else if (trackSuccess == 2)
				datafile << "����" << findRegionList[i]->saucerIndex << "�������" << endl;
			datafile << "����" << findRegionList[i]->saucerIndex << " �Ѷ�ʧ��" << std::endl << std::endl;

			delete (findRegionList[i]);
			findRegionList.erase(findRegionList.begin() + i);


			datafile.close();
		}
	}
	//auto it = std::unique(result.begin(), result.end());
	//result.erase(it, result.end());
	regionNum = result.size();
	return result;
}

void FindRegionList::detectRegion(HObject& image)
{
	SetColor(hv_WindowHandle, "red");
	HObject ho_ROI_0, ho_ImageReduced, ho_Region, ho_ConnectedRegions, ho_SelectedRegions, ho_RegionUnion;
	GenEmptyRegion(&ho_RegionUnion);
	HTuple regionNum, hv_Area, hv_Row, hv_Column, hv_Grayval;


	GenRectangle1(&ho_ROI_0, 0, 0, 478.5, 637.5);
	ReduceDomain(image, ho_ROI_0, &ho_ImageReduced);

	Threshold(ho_ImageReduced, &ho_Region, DETECTMINLIMIT, DETECTMAXLIMIT);
	Connection(ho_Region, &ho_ConnectedRegions);
	SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", 1000, 17177);
	CountObj(ho_SelectedRegions, &regionNum);

	for (HTuple i = 1; i < regionNum + 1; i = i + 1)
	{
		HTuple hv_Deviation;
		AreaCenter(ho_SelectedRegions[i], &hv_Area, &hv_Row, &hv_Column);
		HalconCpp::Intensity(ho_SelectedRegions[i], image, &hv_Grayval, &hv_Deviation);
		if (hv_Grayval.D()>0 && hv_Grayval.D() < 65535)
		{
			Union2(ho_SelectedRegions[i], ho_RegionUnion, &ho_RegionUnion);
#ifdef NEWMETHOD
			pushRegionToFind(hv_Row, hv_Column, hv_Area, hv_Grayval, myCamParam);
#else
			pushRegionToFind(hv_Row, hv_Column, hv_Area, hv_Grayval);
#endif
		}

		if (HDevWindowStack::IsOpen())
			DispObj(ho_RegionUnion, HDevWindowStack::GetActive());
	}
}