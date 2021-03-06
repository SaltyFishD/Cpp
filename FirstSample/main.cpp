#include "HalconCpp.h"
#include "HDevThread.h"
#include "MyHalconFunctions.h"
#include "PillarState.h"
#include "FindRegion.h"
#include "MySerial.h"
#include "CoordinateConvert.h"

// 宏定义
//#define USESERIALPORT

using namespace HalconCpp;

//全局变量
HTuple hv_WindowHandle;
HObject depthImage, confidenceImage;
const void *depthData;
const void *intensityData;
const void *confidenceData;
CToFCamera::Coord3D farPillarCoordinate;
MySerial port;
/*陀螺仪角度*/
extern double Angle[3];

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

class Sample
{
public:
    int run();
    bool onImageGrabbed( GrabResult grabResult, BufferParts );
	void imageConfig();

private:
    CToFCamera  m_Camera;
    int         m_nBuffersGrabbed;
};

void Sample::imageConfig(void)
{
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


	//修改各项参数
	/*image quality*/

	GenApi::CIntegerPtr ptrConfidenceThreshold = m_Camera.GetParameter("ConfidenceThreshold");
	ptrConfidenceThreshold->SetValue(0);
	GenApi::CIntegerPtr ptrFilterStrength = m_Camera.GetParameter("FilterStrength");
	ptrFilterStrength->SetValue(50);
	GenApi::CIntegerPtr ptrOutlierTolerance = m_Camera.GetParameter("OutlierTolerance");
	ptrOutlierTolerance->SetValue(17088);

	/*图像格式控制*/

	//GenApi::CIntegerPtr ptrWidth = m_Camera.GetParameter("Width");
	//ptrWidth->SetValue(640);
	//GenApi::CIntegerPtr ptrHeight = m_Camera.GetParameter("Height");
	//ptrHeight->SetValue(480);
	//GenApi::CIntegerPtr ptrOffsetX = m_Camera.GetParameter("OffsetX");
	//ptrOffsetX->SetValue(0);
	//GenApi::CIntegerPtr ptrOffsetY = m_Camera.GetParameter("OffsetY");
	//ptrOffsetY->SetValue(0);

	/*深度阈值*/

	GenApi::CIntegerPtr ptrDepthMin = m_Camera.GetParameter("DepthMin");
	ptrDepthMin->SetValue(0);
	GenApi::CIntegerPtr ptrDepthMax = m_Camera.GetParameter("DepthMax");
	ptrDepthMax->SetValue(13320);

	/*曝光时间*/

	GenApi::CEnumerationPtr ptrExposureAuto = m_Camera.GetParameter("ExposureAuto");
	ptrExposureAuto->FromString("Continuous");

	//GenApi::CIntegerPtr ptrExposureTimeSelector = m_Camera.GetParameter("ExposureTimeSelector"); ptrExposureTimeSelector->SetValue(0);

	//GenApi::CFloatPtr ptrExposureTime = m_Camera.GetParameter("ExposureTime");
	//ptrExposureTime->SetValue(10000);

	/*温度（可选传感器还是LED）*/

	//GenApi::CEnumerationPtr ptrDeviceTemperatureSelector = m_Camera.GetParameter("DeviceTemperatureSelector"); ptrDeviceTemperatureSelector->FromString("SensorBoard");//"LEDBoard"
	//std::cout << "Value of DeviceTemperatureSelector is " << ptrDeviceTemperatureSelector->ToString() << std::endl;

	/*帧数*/
	GenApi::CFloatPtr ptrAcquisitionFrameRate = m_Camera.GetParameter("AcquisitionFrameRate");
	ptrAcquisitionFrameRate->SetValue(20);//最大为20帧
	//std::cout << "Value of AcquisitionFrameRate is " << ptrAcquisitionFrameRate->GetValue() << std::endl;
}

int Sample::run()
{
    m_nBuffersGrabbed = 0;

    try
    {
        m_Camera.OpenFirstCamera();
        cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;

		imageConfig();

#ifdef USESERIALPORT
		port.auto_open();
		port.openListenThread();
#endif
        // Acquire images until the call-back onImageGrabbed indicates to stop acquisition. 
        // 5 buffers are used (round-robin).
        m_Camera.GrabContinuous( 5, 1000, this, &Sample::onImageGrabbed );

        // Clean-up
        m_Camera.Close();

#ifdef USESERIALPORT
		port.~MySerial();
#endif

    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

FindRegionList taskList;
PillarState myPillarState;
bool Sample::onImageGrabbed( GrabResult grabResult, BufferParts parts )
{
	bool hv_run;

	try
	{
		ofstream datafile;
		datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt",ios::app);
		/********************帧率显示**********************/
		//double frameRate;
		//frameRate = getFrameRate();
		//cout << "frameRate:  " << setw(4) << frameRate << endl;
		/*************************************************/

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
			const int width = (int)parts[0].width;
			const int height = (int)parts[0].height;

			depthData = parts[0].pData;
			intensityData = parts[1].pData;
			confidenceData = parts[2].pData;
			//float *depthImageByte = new float[width*height];
			uint16_t *depthImageByte = new uint16_t[width*height];
			//byte *depthImageByte = new byte[width*height];
			for (int i = 0; i < width*height; i++)
			{
				if (((uint16_t*)confidenceData)[i] < 4000 && (((CToFCamera::Coord3D*) depthData + i)->z) < 4000)
					depthImageByte[i] = 0;
				else           
					depthImageByte[i] = (((CToFCamera::Coord3D*) depthData + i)->z) /** 256 / 14000*/;
			}
			//GenImage1(&depthImage, "real", width, height, (Hlong)(depthImageByte));
			GenImage1(&depthImage, "uint2", width, height, (Hlong)(depthImageByte));
			//GenImage1(&depthImage, "uint2", width, height, (Hlong)(depthImageByte));
			//GenImage1(&confidenceImage, "int2", width, height, (Hlong)(uint16_t *)(confidenceData));

			if (HDevWindowStack::IsOpen())
				DispObj(depthImage, HDevWindowStack::GetActive());

			/*********************write Image************************/
			//static HTuple  hv_ImageCount = 0;
			//hv_ImageCount = hv_ImageCount + 1;
			//if (hv_ImageCount.D() < 600)
			//{
			//	WriteImage(depthImage, "tiff", 0, "C:/Users/Administrator/Desktop/2/depth" + hv_ImageCount);
			//}
			//else
			//	return 0;
			/********************************************************/

			/*********************台柱识别***************************/
			/*********************古代的方法*************************/
			//{
			//	{
			//		SetColor(hv_WindowHandle, "green");
			//		HObject ho_ROI_Rect, ho_PillarRoI, ho_Regions, ho_ConnectedRegions, ho_SelectedRegions;
			//		GenRectangle1(&ho_ROI_Rect, 200, 0, 478.5, 638.5);
			//		ReduceDomain(depthImage, ho_ROI_Rect, &ho_PillarRoI);
			//		Threshold(ho_PillarRoI, &ho_Regions, 7200, 8400);
			//		Connection(ho_Regions, &ho_ConnectedRegions);
			//		SelectShapeStd(ho_ConnectedRegions, &ho_SelectedRegions, "max_area", 90);
			//		//AreaCenter(ho_SelectedRegions, &hv_PillarArea, &hv_PillarRow, &hv_PillarColumn);

			//		HTuple  hv_Value, hv_column;
			//		RegionFeatures(ho_SelectedRegions, ((HTuple("row1").Append("column1")).Append("column2")),
			//			&hv_Value);
			//		int farPillarColumn = (hv_Value[1].D() + hv_Value[2].D()) / 2;
			//		int farPillarRow = hv_Value[0].D() + 5;
			//		if (HDevWindowStack::IsOpen())
			//			DispObj(ho_SelectedRegions, HDevWindowStack::GetActive());
			//		disp_message(hv_WindowHandle, "远", "window", farPillarRow, farPillarColumn - 15, "black", "true");


			//		CToFCamera::Coord3D *pFarPillarCoordinate;
			//		pFarPillarCoordinate = (CToFCamera::Coord3D*) depthData + farPillarRow * width + (int)farPillarColumn;
			//		farPillarCoordinate = middleFilter(*pFarPillarCoordinate);
			//		cout << farPillarCoordinate.y << endl;
			//		
			//	}
			//}
			//static bool first = true;
			//if (first)
			//{
			//	datafile << "    " << "柱子坐标" << farPillarCoordinate.x << "   " << farPillarCoordinate.y << "    " << farPillarCoordinate.z << endl;
			//	first = false;
			//}
			/***********************现代的方法****************************************/
			//myPillarState.updateCameraData(depthImage, depthData, confidenceData);
			//cameraParam tmpParam;
			//tmpParam.worldX = 5520;
			//tmpParam.worldY = 1750;
			//tmpParam.worldZ = 790;
			//tmpParam.pitch = -Angle[1] + 2;
			//tmpParam.yaw = Angle[2] + 7.5;

			//CToFCamera::Coord3D pillarPixelCoor;
			//CToFCamera::Coord3D tmp;
			////现在返回的是像素坐标
			//pillarPixelCoor = myPillarState.getPillarCoor(tmpParam, myPillarState.mostLeftPillar);
			//tmp = *((CToFCamera::Coord3D*) depthData + (int)pillarPixelCoor.x * width + (int)pillarPixelCoor.y);
			/********************************************************/

			/***************************飞盘追踪*********************/
			//SetColor(hv_WindowHandle, "red");
			//HObject ho_ROI_0, ho_ImageReduced, ho_Region, ho_ConnectedRegions, ho_SelectedRegions, ho_RegionUnion;
			//GenEmptyRegion(&ho_RegionUnion);
			//vector<HObject> regionsFound;
			//HTuple regionNum, hv_Area;

			//
			//GenRectangle1(&ho_ROI_0, 0, 0, 478.5, 637.5);
			//ReduceDomain(depthImage, ho_ROI_0, &ho_ImageReduced);

			//Threshold(ho_ImageReduced, &ho_Region, 100, 3000);
			//Connection(ho_Region, &ho_ConnectedRegions);
			//SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", 1000, 17177);
			//CountObj(ho_SelectedRegions, &regionNum);

			//int num = regionNum.D();

			//for (HTuple i = 1; i < regionNum + 1; i = i + 1)
			//{
			//	HTuple hv_Deviation;
			//	AreaCenter(ho_SelectedRegions[i], &hv_Area, &hv_Row, &hv_Column);
			//	HalconCpp::Intensity(ho_SelectedRegions[i], depthImage, &hv_Grayval,&hv_Deviation);
			//	if (hv_Grayval.D()>0&& hv_Grayval.D() < 65535)
			//	{
			//		Union2(ho_SelectedRegions[i], ho_RegionUnion, &ho_RegionUnion);
			//		taskList.PushRegionToFind(hv_Row, hv_Column, hv_Area, hv_Grayval);
			//	}
			//}

			//if (HDevWindowStack::IsOpen())
			//	DispObj(ho_RegionUnion, HDevWindowStack::GetActive());

			////存储找到的region
			//regionsFound = taskList.RegionsFound(depthImage);
			//if (regionsFound.size() > 0)
			//{
			//	for (size_t i = 0; i < regionsFound.size(); ++i)
			//	{
			//		HTuple hv_SaucerArea, hv_SaucerRow, hv_SaucerColumn;
			//		if (HDevWindowStack::IsOpen())
			//			DispObj(regionsFound[i], HDevWindowStack::GetActive());
			//		AreaCenter(regionsFound[i], &hv_SaucerArea, &hv_SaucerRow, &hv_SaucerColumn);

			//		CToFCamera::Coord3D *pSaucerCoorTwo[2];
			//		pSaucerCoorTwo[0] = (CToFCamera::Coord3D*) depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() - 1;
			//		pSaucerCoorTwo[1] = (CToFCamera::Coord3D*) depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() + 1;
			//		CToFCamera::Coord3D* pSaucerCoordinate = new CToFCamera::Coord3D();
			//		pSaucerCoordinate->x = (pSaucerCoorTwo[0]->x + pSaucerCoorTwo[1]->x) / 2;
			//		pSaucerCoordinate->y = (pSaucerCoorTwo[0]->y + pSaucerCoorTwo[1]->y) / 2;
			//		pSaucerCoordinate->z = (pSaucerCoorTwo[0]->z + pSaucerCoorTwo[1]->z) / 2;

			//		if (pSaucerCoordinate->z > 2000)
			//		{
			//			//记录坐标
			//			taskList.findRegionList[i]->recordRegionTrack(*pSaucerCoordinate);

			//			datafile << "飞盘编号: " << taskList.findRegionList[i]->saucerIndex << " X: " << setw(2) << pSaucerCoordinate->x << " Y: " << setw(2) << pSaucerCoordinate->y << " Z: " << pSaucerCoordinate->z << endl;

			//			datafile << "    " << "柱子坐标" << farPillarCoordinate.x << "   " << farPillarCoordinate.y << "    " << farPillarCoordinate.z << endl;
			//		}
			//	}
			//}
			/*****************************************************************/

			
			HTuple  hv_Row, hv_Column, hv_Button;
			HTuple  hv_Grayval, hv_Exception;

			/*按鼠标中键退出程序*/
			hv_run = 1;
			try
			{
				GetMposition(3600, &hv_Row, &hv_Column, &hv_Button);
				GetGrayval(depthImage, hv_Row, hv_Column, &hv_Grayval);
				CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) depthData + (int)hv_Row.D() * width + (int)hv_Column.D();
				cout << "----------------------------------------------------------" << endl;
				//cout << "x: " << setw(2) << p3DCoordinate->x << " y: " << setw(2) << p3DCoordinate->y << " Z:  " << setw(2) << p3DCoordinate->z << endl;
				
				CToFCamera::Coord3D computeCoor = PixelCoorToCameraCoor(hv_Row.D(), hv_Column.D(), hv_Grayval.D());
				cout << "After compute:" << endl;
				cout << "x: " << setw(2) << p3DCoordinate->x-computeCoor.x << " y: " << setw(2) << p3DCoordinate->y-computeCoor.y << " Z:  " << setw(2) << p3DCoordinate->z-computeCoor.z << endl;
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

			delete[] depthImageByte;
		}
		
		datafile.close();
		return hv_run; 
	}
	catch (...)
	{
		return 0;
	}
}

int main(int argc, char* argv[])
{
	SetSystem("width", 512);
	SetSystem("height", 512);

	SetSystem("use_window_thread", "true");
	// Local iconic variables
	HObject  ho_Image, ho_GrayImage, ho_Region;


	// Local control variables
	SetWindowAttr("background_color", "black");
	OpenWindow(0, 0, 640, 480, 0, "", "", &hv_WindowHandle);
	HDevWindowStack::Push(hv_WindowHandle);
	SetPart(hv_WindowHandle, 0, 0, 480, 640);
    int exitCode = EXIT_SUCCESS;
    
    try
    {
		ofstream datafile;
		datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt");
		datafile.clear();
		datafile.close();

        CToFCamera::InitProducer();

        Sample processing;

        exitCode = processing.run();
    }
    catch ( GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
        exitCode = EXIT_FAILURE;
    }

    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions

    return exitCode;
}
