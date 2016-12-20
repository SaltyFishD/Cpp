/*
Access to ToF cameras is provided by a GenICam-compliant GenTL Producer. A GenTL Producer is a dynamic library
implementing a standardized software interface for accessing the camera.

The software interacting with the GentL Producer is called a GenTL Consumer. Using this terminology,
this sample is a GenTL Consumer, too.

As part of the suite of sample programs, Basler provides the ConsumerImplHelper C++ template library that serves as
an implementation helper for GenTL consumers.

The GenICam GenApi library is used to access the camera parameters.

For more details about GenICam, GenTL, and GenApi, refer to the http://www.emva.org/standards-technology/genicam/ website.
The GenICam GenApi standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_Standard_v2_0.pdf
The GenTL standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf

This sample illustrates how to grab images from a ToF camera and how to access the image and depth data.

The GenApi sample, which is part of the Basler ToF samples, illustrates in more detail how to configure a camera.
*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <fstream>
#include <iomanip> 
#include <sstream>
#include <ctime>

#include "HalconCpp.h"
#include "HDevThread.h"
using namespace HalconCpp;
using namespace std;
//HTuple hv_WindowHandle;
HObject cloudImage;
BYTE *pDepth;
HTuple hv_PointerBlueNew;
HTuple hv_Type;
HTuple hv_Height, hv_Width;
HObject depthImage;
HObject ho_ThisImage;//传递图像全局定义
HObject depthDataImage;
HTuple  hv_WindowHandle;

ofstream outfile;

int hv_i = 0;//帧数全局定义
int  hv_Value0 = 0;
int OutPutX = 0;
int OutPutY = 0;
int width = 0;
int height = 0;


float Last_worldX = 0;
float Last_worldY = 0;
float Last_worldZ = 0;

double areavalue = 0;
double ravalue = 0;
double rbvalue = 0;
double phivalue = 0;
double WorldX = 0;
double WorldY = 0;
double WorldZ = 0;
double AreaShouldBe = 0;
double AreaShouldBe1 = 0;

bool OutPutFlag = true;
bool EdgeFlag = true;
bool FrameReadyFlag = false;
bool FrameDCFlag = false;
bool FrameFFlag = false;

int frame_number = 1;
int Trail_Number = 0;
int TrailOutPutNumber = 0;
int RFrameNumber = 0;
int FirstNumber = 0;
int DiffNumber = 2;

void *replace_;

typedef struct Trail
{
	double data[3];//xyz
				   //开启过的标志位
	bool OpenFlag;
	//已关闭的标志位
	bool CloseFlag;
	//已经消失的点数，在一定次数内找到重置
	int DisappearNumber;
	//总共找到的点数，用来判断是否重置数组
	int AllPointNumber;

	int RecordFrameNumber;

	double Trailphi;
};

Trail Last_Trail[5];
Trail NowTrail[5];//一帧图像上最大region

#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif


using namespace GenTLConsumerImplHelper;
using namespace std;

class Sample
{
public:
	int run();

	bool onImageGrabbed(GrabResult grabResult, BufferParts);

private:
	CToFCamera  m_Camera;
	int         m_nBuffersGrabbed;
};

double getFrameRate()
{
	static clock_t now = clock();
	static bool firstCalled = true;
	if (!firstCalled)
	{
		clock_t newNow = clock();
		int distanceMs = newNow - now;
		double frameRate = 1000.0 / distanceMs;
		now = newNow;
		return frameRate;
	}
	else
	{
		firstCalled = false;
		return -1;
	}
}
int Sample::run()
{
	m_nBuffersGrabbed = 0;

	try
	{
		m_Camera.OpenFirstCamera();
		cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;

		// Enable 3D (point cloud) data, intensity data, and confidence data 
		GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
		GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
		GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");

		// Enable range data
		ptrComponentSelector->FromString("Range");
		ptrComponentEnable->SetValue(true);
		// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
		// Note: To change the format of an image component, the Component Selector must first be set to the component
		// you want to configure (see above).
		// To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
		ptrPixelFormat->FromString("Coord3D_ABC32f");

		ptrComponentSelector->FromString("Intensity");
		ptrComponentEnable->SetValue(true);

		ptrComponentSelector->FromString("Confidence");
		ptrComponentEnable->SetValue(true);

		GenApi::CIntegerPtr ptrDepthMin = m_Camera.GetParameter("DepthMin"); 
		ptrDepthMin->SetValue(2262);
		GenApi::CEnumerationPtr ptrExposureAuto = m_Camera.GetParameter("ExposureAuto");
		ptrExposureAuto->FromString("Off");
		GenApi::CIntegerPtr ptrExposureTimeSelector = m_Camera.GetParameter("ExposureTimeSelector");
		ptrExposureTimeSelector->SetValue(0);
		GenApi::CFloatPtr ptrExposureTime = m_Camera.GetParameter("ExposureTime");
		ptrExposureTime->SetValue(10000);
		GenApi::CFloatPtr ptrAcquisitionFrameRate = m_Camera.GetParameter("AcquisitionFrameRate");
		ptrAcquisitionFrameRate->SetValue(20);
		GenApi::CIntegerPtr ptrConfidenceThreshold = m_Camera.GetParameter("ConfidenceThreshold");
		ptrConfidenceThreshold->SetValue(0);
		GenApi::CIntegerPtr ptrFilterStrength = m_Camera.GetParameter("FilterStrength");
		ptrFilterStrength->SetValue(50);
		GenApi::CIntegerPtr ptrOutlierTolerance = m_Camera.GetParameter("OutlierTolerance");
		ptrOutlierTolerance->SetValue(17920);
		// Acquire images until the call-back onImageGrabbed indicates to stop acquisition. 
		// 5 buffers are used (round-robin).
		m_Camera.GrabContinuous(5, 1000, this, &Sample::onImageGrabbed);

		// Clean-up
		m_Camera.Close();
	}
	catch (const GenICam::GenericException& e)
	{
		cerr << "Exception occurred: " << e.GetDescription() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void Fit_Trail()
{
	int i, j, k;
	int SecondNumber = 0;


	bool NewTrailFlag = false;//是否占用了新的数组
	bool NotFillFlag = false;//不再占用flag
	double distance[5] = { 999999 };
	double MinDistance = 999999;
	double MinMinDistance = 999999;

	if (Trail_Number == 1 && frame_number - RFrameNumber < 4)//通过掉帧确定阈值,并且去除清除帧的情况
	{
		DiffNumber = frame_number - RFrameNumber;
	}

	for (k = 0; k < 5; k++)//连续掉帧关闭之前通道
	{
		if (frame_number - RFrameNumber > 3 && Last_Trail[k].OpenFlag == true)
		{
			Last_Trail[k].CloseFlag = true;
			Last_Trail[k].OpenFlag = false;
		}
	}

	//筛选是否有符合阈值的LastTrail
	for (i = 0; i < 5; i++)
	{
		if (Last_Trail[i].data[0] == 0 && Last_Trail[i].data[1] == 0 && Last_Trail[i].data[2] == 0 && NotFillFlag == false)
		{
			Last_Trail[i].data[0] = NowTrail[Trail_Number].data[0];
			Last_Trail[i].data[1] = NowTrail[Trail_Number].data[1];
			Last_Trail[i].data[2] = NowTrail[Trail_Number].data[2];
			//Last_Trail[i].AllPointNumber++;
			TrailOutPutNumber = i;
			NewTrailFlag = true;//跳过距离判断
			i = 5;
		}
		else
		{
			//对距离进行筛选（筛选的几组数据都不为0）
			if (Last_Trail[i].CloseFlag == false)//去掉其他没数据的数组以及已经关闭了的数组
			{
				if (Last_Trail[i].data[0] == 0 && Last_Trail[i].data[1] == 0)
				{
					distance[i] = 999999;
				}
				else
				{
					//加上z之后效果不佳？！阈值
					distance[i] = sqrt((Last_Trail[i].data[0] - NowTrail[Trail_Number].data[0]) * (Last_Trail[i].data[0] - NowTrail[Trail_Number].data[0])
						+ (Last_Trail[i].data[1] - NowTrail[Trail_Number].data[1]) * (Last_Trail[i].data[1] - NowTrail[Trail_Number].data[1]));
					//+ (Last_Trail[i].data[2] - NowTrail[Trail_Number].data[2]) * (Last_Trail[i].data[2] - NowTrail[Trail_Number].data[2])
				}
				if (distance[i] > 150 * (DiffNumber + Last_Trail[i].DisappearNumber))//可以根据RFrameNumber和frame_number之间的差值来判断
				{
					distance[i] = 999999;
				}
				else
				{
					NotFillFlag = true;//所有都不符合要求的话NotFillFlag为flase之后会调用新的数组
				}
			}
			else//由于之前的distance的值没有清除并且之后一直会调用
			{
				distance[i] = 999999;
			}
		}
	}

	//有的话取最小值，当最小值和次小值相差不过10cm时，判断其phi决定
	if (!NewTrailFlag)
	{
		for (j = 0; j < 5; j++)
		{
			if (distance[j] < MinDistance)
			{
				MinMinDistance = MinDistance;
				MinDistance = distance[j];
				SecondNumber = TrailOutPutNumber;
				TrailOutPutNumber = j;
			}
			else if (distance[j] < MinMinDistance)
			{
				MinMinDistance = distance[j];
				SecondNumber = j;
			}
		}
		if (abs(MinMinDistance - MinDistance) < 100)
		{
			if (abs(Last_Trail[SecondNumber].Trailphi - NowTrail[Trail_Number].Trailphi) <
				abs(Last_Trail[TrailOutPutNumber].Trailphi - NowTrail[Trail_Number].Trailphi))
			{
				TrailOutPutNumber = SecondNumber;
			}
		}
		if (MinDistance == 999999)//没有找到阈值范围内的值，而且没有剩余的存储空间
		{
			TrailOutPutNumber = 6;
		}
		else
		{
			Last_Trail[TrailOutPutNumber].data[0] = NowTrail[Trail_Number].data[0];
			Last_Trail[TrailOutPutNumber].data[1] = NowTrail[Trail_Number].data[1];
			Last_Trail[TrailOutPutNumber].data[2] = NowTrail[Trail_Number].data[2];
			//Last_Trail[j].AllPointNumber++;
			//TrailOutPutNumber = j;
		}
	}

	//各个轨迹的数据更新以及状态的改变，不影响实际TrailOutPutNumber的输出
	for (k = 0; k < 5; k++)
	{
		if (k == TrailOutPutNumber)
		{
			Last_Trail[k].AllPointNumber++;//循环利用时的参数
			Last_Trail[k].DisappearNumber = 0;
			Last_Trail[k].RecordFrameNumber = frame_number;
			Last_Trail[k].OpenFlag = true;
		}
		else
			if (Last_Trail[k].OpenFlag == true)
			{
				if (Last_Trail[k].RecordFrameNumber != frame_number)//保证丢失每帧的只加一次
				{
					Last_Trail[k].RecordFrameNumber = frame_number;
					Last_Trail[k].DisappearNumber++;
				}
				if (Last_Trail[k].DisappearNumber > 3)
				{
					Last_Trail[k].CloseFlag = true;
					Last_Trail[k].OpenFlag = false;
				}

			}

	}

	RFrameNumber = frame_number;
}



void OutPut()
{
	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) replace_ + OutPutY * width + OutPutX;
	WorldX = p3DCoordinate->x;
	WorldY = p3DCoordinate->y;
	WorldZ = p3DCoordinate->z;
	AreaShouldBe = 18617 / WorldZ / WorldZ * 1000 * 1000;
	AreaShouldBe1 = 2907 / WorldZ / WorldZ * 1000 * 1000;

	if ((areavalue < AreaShouldBe) && (areavalue > AreaShouldBe1) && (EdgeFlag == 1))
	{
		if (FirstNumber == 0)
		{
			RFrameNumber = frame_number;
			FirstNumber++;
		}

		NowTrail[Trail_Number].data[0] = WorldX;
		NowTrail[Trail_Number].data[1] = WorldY;
		NowTrail[Trail_Number].data[2] = WorldZ;
		NowTrail[Trail_Number].Trailphi = phivalue;

		Fit_Trail();//分轨迹

		OutPutFlag = 1;
		outfile.open("data.txt", ios::app);
		if (TrailOutPutNumber < 6)
		{
			if (Trail_Number == 1)
			{
				outfile << "    " << endl;
				outfile << "新帧" << endl;
			}
			outfile << "    第" << TrailOutPutNumber << "条轨迹 :" << WorldZ << "    " << -WorldX << "    " << -WorldY << "    " << areavalue << "    " << phivalue << endl;

		}
		else
		{
			outfile << "   轨迹达到上限" << endl;
		}
		outfile.close();
	}
	else
	{
		OutPutFlag = 0;
	}
}

void action(byte *depthImage)
{

	// Local iconic variables


	float RabScaling = 0;
	float Scaling = 0;
	float Scaling1 = 0;
	//double areavalue = 0;
	//double ravalue = 0;
	//double rbvalue = 0;

	HObject  ho_Image, ho_Regions, ho_ConnectedRegions;
	HObject  ho_SelectedRegions_4, ho_ObjectSelected;
	HObject  ho_Ellipse, ho_RegionIntersection;
	HObject  ho_UnionRegion, ho_User;

	HTuple  hv_ImageFiles, hv_Index, hv_Number, hv_PanData;
	HTuple  hv_Value, InterestingArea, Row, Col, AreaValue;
	HTuple  Value1, Value2, Value3;
	

	Last_worldX = 0;
	Last_worldY = 0;
	Last_worldZ = 0;

	GenEmptyRegion(&ho_UnionRegion);
	GenEmptyRegion(&ho_User);

	     
		Threshold(cloudImage, &ho_Regions, 1, 12000 * 256 / 14000);//距离阈值
		Connection(ho_Regions, &ho_ConnectedRegions);
		//初步选择区域，减少之后的运算量
		SelectShape(ho_ConnectedRegions, &ho_SelectedRegions_4, (((HTuple("area").Append("ra")).Append("rb")).Append("row")),
			"and", (((HTuple(250).Append(3)).Append(3)).Append(18)), (((HTuple(17000).Append(99999)).Append(99999)).Append(480)));
		ho_UnionRegion = ho_User; 
		CountObj(ho_SelectedRegions_4, &hv_Number);
		{
			Trail_Number = 0;
			HTuple end_val15 = hv_Number;
			HTuple step_val15 = 1;
			for (hv_PanData = 1; hv_PanData.Continue(end_val15, step_val15); hv_PanData += step_val15)
			{
				SelectObj(ho_SelectedRegions_4, &ho_ObjectSelected, hv_PanData);

				//区域参数
				RegionFeatures(ho_ObjectSelected, (HTuple("area").Append("ra").Append("rb").Append("phi").Append("row").Append("column")), &AreaValue);
				areavalue = AreaValue[0].D();
				ravalue = AreaValue[1].D();
				rbvalue = AreaValue[2].D();
				phivalue = AreaValue[3].D();

				//最小椭圆
				GenEllipse(&ho_Ellipse, AreaValue[4], AreaValue[5], AreaValue[3], AreaValue[1], AreaValue[2]);

				//重合区域
				Intersection(ho_Ellipse, ho_ObjectSelected, &ho_RegionIntersection);

				//重合区域的面积，最小椭圆的面积，原区域的面积
				RegionFeatures(ho_RegionIntersection, HTuple("area"), &Value1);
				RegionFeatures(ho_Ellipse, HTuple("area"), &Value2);
				RegionFeatures(ho_ObjectSelected, HTuple("area"), &Value3);

				//比♂例
				RabScaling = AreaValue[1].D() / AreaValue[2].D();
				Scaling = Value1[0].D() / Value3[0].D();
				Scaling1 = Value1[0].D() / Value2[0].D();

				//取区域中点作为参考点
				AreaCenter(ho_ObjectSelected, &InterestingArea, &Row, &Col);
				OutPutY = Row.D();
				OutPutX = Col.D();

				EdgeFlag = false;

				if (Scaling > 0.9)
				{
					if (Scaling1 > 0.8)
					{
						EdgeFlag = true;
						Trail_Number++;
					}
				}
				//}

				//listener.read_depth_data(OutPutX, OutPutY);
				OutPut();

				if (EdgeFlag == 1 && OutPutFlag == 1)
				{
					Union2(ho_UnionRegion, ho_ObjectSelected, &ho_UnionRegion);
				}
			}
		}
		SetPart(hv_WindowHandle, 0, 0, height, width);
			if (HDevWindowStack::IsOpen())
				DispObj(cloudImage, HDevWindowStack::GetActive());
			if (HDevWindowStack::IsOpen())
				SetColor(HDevWindowStack::GetActive(), "green");
			if (HDevWindowStack::IsOpen())
				DispObj(ho_UnionRegion, HDevWindowStack::GetActive());

			frame_number++;
		
	
}


bool Sample::onImageGrabbed(GrabResult grabResult, BufferParts parts)
{
	bool hv_run;
	HTuple  hv_Row, hv_Column, hv_Button;
	HTuple  hv_Grayval, hv_Exception;
	double frame;
	frame = getFrameRate();

	cout << frame << endl;
	replace_ = parts[0].pData;

	if (grabResult.status == GrabResult::Timeout)
	{
		cerr << "Timeout occurred. Acquisition stopped." << endl;
		return false; // Indicate to stop acquisition
	}
	m_nBuffersGrabbed++;
	if (grabResult.status != GrabResult::Ok)
	{
		cerr << "Image " << m_nBuffersGrabbed << "was not grabbed." << endl;
	}
	else
	{
		// Retrieve the values for the center pixel
		width = (int)parts[0].width;
		height = (int)parts[0].height;
		const int x = (int)(0.5 * width);
		const int y = (int)(0.5 * height);

		uint16_t *pIntensity = (uint16_t*)parts[1].pData + y * width + x;
		uint16_t *pConfidence = (uint16_t*)parts[2].pData + y * width + x;
		//float *depthImage = new float[width*height];
		//uint16_t *depthImage = new uint16_t[width*height];
		byte *depthImage = new byte[width*height];
		for (int i = 0; i < width*height; i++)
		{
			depthImage[i] = ((uint16_t)((CToFCamera::Coord3D*) parts[0].pData + i)->z) * 256 / 14000;
		}
		//GenImage1(&cloudImage, "real", width, height, (Hlong)(depthImage));
		//GenImage1(&cloudImage, "uint2", width, height, (Hlong)(depthImage));
		GenImage1(&cloudImage, "byte", width, height, (Hlong)(depthImage));
		//GenImage1(&cloudImage, "int2", width, height, (Hlong)(uint16_t *)(pConfidence));

		action(depthImage);

		SetPart(hv_WindowHandle, 0, 0, height, width);
		//if (HDevWindowStack::IsOpen())
		//	DispObj(cloudImage, HDevWindowStack::GetActive());

		hv_run = 1;
		try
		{
			GetMposition(3600, &hv_Row, &hv_Column, &hv_Button);
			GetGrayval(cloudImage, hv_Row, hv_Column, &hv_Grayval);
			CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData + (int)hv_Row.D() * width + (int)hv_Column.D();
			cout << "row: " << setw(4) << hv_Row.D() << " col: " << setw(4) << hv_Column.D() << " Z:  " << setw(4) << p3DCoordinate->z << endl;
			if (0 != (hv_Button == 2))
			{
				hv_run = 0;
			}
		}
		// catch (Exception) 
		catch (HalconCpp::HException &HDevExpDefaultException)
		{
			HDevExpDefaultException.ToHTuple(&hv_Exception);
		}

		delete[] depthImage;
		//cout << "Center pixel of image " << setw(2) << m_nBuffersGrabbed << ": ";
		//cout.setf( ios_base::fixed);
		//cout.precision(1);
		//if ( p3DCoordinate->IsValid() )
		//    cout << "x=" << setw(6) <<  p3DCoordinate->x << " y=" << setw(6) << p3DCoordinate->y << " z=" << setw(6) << p3DCoordinate->z;
		//else
		//    cout << "x=   n/a y=   n/a z=   n/a";
		//cout << " intensity="<< setw(5) << *pIntensity << " confidence=" << setw(5) << *pConfidence << endl;
	}
	return hv_run; // Indicate to stop acquisition when 10 buffers are grabbed
}

int main(int argc, char* argv[])
{
	SetSystem("width", 512);
	SetSystem("height", 512);

	for (int Count = 0; Count < 5; Count++)
	{
		Last_Trail[Count].data[0] = 0;
		Last_Trail[Count].data[1] = 0;
		Last_Trail[Count].data[2] = 0;
		Last_Trail[Count].AllPointNumber = 0;
		Last_Trail[Count].DisappearNumber = 0;
		Last_Trail[Count].OpenFlag = false;
		Last_Trail[Count].CloseFlag = false;
		Last_Trail[Count].RecordFrameNumber = 0;
		Last_Trail[Count].Trailphi = 0;
	}

	outfile.open("data.txt");
	outfile.close();

	outfile.open("data.txt", ios::app);
	outfile << "               " << "X       " << "Y          " << "Z           " << "S      " << "phi" << endl;
	outfile.close();

	SetSystem("use_window_thread", "true");
	// Local iconic variables
	HObject  ho_Image, ho_GrayImage, ho_Region;


	// Local control variables
	SetWindowAttr("background_color", "black");
	OpenWindow(0, 0, 640, 480, 0, "", "", &hv_WindowHandle);
	HDevWindowStack::Push(hv_WindowHandle);
	SetPart(hv_WindowHandle, 0, 0, 640, 480);

	int exitCode = EXIT_SUCCESS;

	try
	{
		CToFCamera::InitProducer();

		Sample processing;


		exitCode = processing.run();
	}
	catch (GenICam::GenericException& e)
	{
		cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
		exitCode = EXIT_FAILURE;
	}

	// Release the GenTL producer and all of its resources. 
	// Note: Don't call TerminateProducer() until the destructor of the CToFCamera
	// class has been called. The destructor may require resources which may not
	// be available anymore after TerminateProducer() has been called.
	if (CToFCamera::IsProducerInitialized())
		CToFCamera::TerminateProducer();  // Won't throw any exceptions

	cout << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}
