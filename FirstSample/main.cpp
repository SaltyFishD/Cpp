#include "stdafx.h"
#include "HalconCpp.h"
#include "HDevThread.h"
#include "MyHalconFunctions.h"
#include "PillarState.h"
#include "FindRegion.h"
#include "MySerial.h"
#include "CoordinateConvert.h"

//摄像头宏定义
#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif

//命名空间
using namespace HalconCpp;
using namespace GenTLConsumerImplHelper;
using namespace std;

//全局变量
HTuple hv_WindowHandle;
HObject depthImage, confidenceImage;
const void *depthData;//深度图像数据
const void *intensityData;
const void *confidenceData;
myCoor3D pillarCamCoor;
MySerial port;//串口类
cameraParam myCamParam;//摄像头状态参数;

FindRegionList taskList; //飞盘追踪类
PillarState myPillarState; //台柱识别类

//外部变量
extern double Angle[3];//陀螺仪角度

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

class CameraAction
{
public:
    int run();
    bool onImageGrabbed( GrabResult grabResult, BufferParts );
	void imageConfig();

private:
    CToFCamera  m_Camera;
    int         m_nBuffersGrabbed;
};

void CameraAction::imageConfig(void)
{
	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");

	// Enable range data
	ptrComponentSelector->FromString("Range");
	ptrComponentEnable->SetValue(true);
	// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this CameraAction, we want to acquire 3D coordinates.
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

int CameraAction::run()
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
        m_Camera.GrabContinuous( 5, 1000, this, &CameraAction::onImageGrabbed );

        // Clean-up
        m_Camera.Close();

    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool CameraAction::onImageGrabbed( GrabResult grabResult, BufferParts parts )
{
	bool hv_run;

	try
	{
		ofstream datafile;
		datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt",ios::app);
		/********************帧率显示**********************/
		double frameRate;
		frameRate = getFrameRate();
		cout << "frameRate:  " << setw(4) << frameRate << endl;
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

			//生成深度图像
			depthData = parts[0].pData;
			intensityData = parts[1].pData;
			confidenceData = parts[2].pData;
			//float *depthImageByte = new float[width*height];
			uint16_t *depthImageByte = new uint16_t[width*height];
			//byte *depthImageByte = new byte[width*height];
			for (int i = 0; i < width*height; i++)
			{
				if (((uint16_t*)confidenceData)[i] < 4000 && (((myCoor3D*) depthData + i)->z) < 4000)
					depthImageByte[i] = 0;
				else           
					depthImageByte[i] = (((myCoor3D*) depthData + i)->z) /** 256 / 14000*/;
			}
			//GenImage1(&depthImage, "real", width, height, (Hlong)(depthImageByte));
			GenImage1(&depthImage, "uint2", width, height, (Hlong)(depthImageByte));
			//GenImage1(&depthImage, "uint2", width, height, (Hlong)(depthImageByte));
			//GenImage1(&confidenceImage, "int2", width, height, (Hlong)(uint16_t *)(confidenceData));

			//释放图片内存
			delete[] depthImageByte;

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
			myPillarState.updateCameraData(depthImage, depthData, confidenceData);

			myCamParam.worldX = 5740;
			myCamParam.worldY = 1160;
			myCamParam.worldZ = 520;
			myCamParam.pitch = -Angle[1] + 3;
			myCamParam.yaw = Angle[2] - 16;

			myCoor3D pillarPixelCoor;

			//现在返回的是像素坐标
			pillarPixelCoor = myPillarState.getPillarCoor(myCamParam, myPillarState.nearPillar);
			pillarCamCoor = *((myCoor3D*) depthData + (int)pillarPixelCoor.x * width + (int)pillarPixelCoor.y);
			/***************************飞盘追踪*********************/
			
				//检测有无飞盘飞过检测区
				taskList.detectRegion(depthImage);

				vector<HObject> regionsFound;
try
{
				//存储找到的region
				regionsFound = taskList.RegionsFound(depthImage);
				if (regionsFound.size() > 0)
				{

					for (size_t i = 0; i < regionsFound.size(); ++i)
					{
						HTuple hv_SaucerArea, hv_SaucerRow, hv_SaucerColumn;
						if (HDevWindowStack::IsOpen())
							DispObj(regionsFound[i], HDevWindowStack::GetActive());
						AreaCenter(regionsFound[i], &hv_SaucerArea, &hv_SaucerRow, &hv_SaucerColumn);

						myCoor3D *pSaucerCoorTwo[2];
						pSaucerCoorTwo[0] = (myCoor3D*)depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() - 1;
						pSaucerCoorTwo[1] = (myCoor3D*)depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() + 1;
						myCoor3D* pSaucerCoordinate = new myCoor3D();
						pSaucerCoordinate->x = (pSaucerCoorTwo[0]->x + pSaucerCoorTwo[1]->x) / 2;
						pSaucerCoordinate->y = (pSaucerCoorTwo[0]->y + pSaucerCoorTwo[1]->y) / 2;
						pSaucerCoordinate->z = (pSaucerCoorTwo[0]->z + pSaucerCoorTwo[1]->z) / 2;
					
						if (pSaucerCoordinate->z > 2000)
						{
							//记录坐标
							taskList.findRegionList[i]->recordRegionTrack(*pSaucerCoordinate);

							//datafile << "飞盘编号: " << taskList.findRegionList[i]->saucerIndex << " X: " << setw(2) << pSaucerCoordinate->x << " Y: " << setw(2) << pSaucerCoordinate->y << " Z: " << pSaucerCoordinate->z << endl;

							//datafile << "    " << "柱子坐标" << farPillarCoordinate.x << "   " << farPillarCoordinate.y << "    " << farPillarCoordinate.z << endl;
						}

					}

				}
}
catch (...)
{
	cout << "这错了" << endl;
	throw ERROR;
}
			
			/*****************************************************************/

			
			HTuple  hv_Row, hv_Column, hv_Button;
			HTuple  hv_Grayval, hv_Exception;

			/*按鼠标中键退出程序*/
			hv_run = 1;
			try
			{
				GetMposition(3600, &hv_Row, &hv_Column, &hv_Button);
				GetGrayval(depthImage, hv_Row, hv_Column, &hv_Grayval);
				myCoor3D *p3DCoordinate = (myCoor3D*) depthData + (int)hv_Row.D() * width + (int)hv_Column.D();

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

		}
		
		datafile.close();
		return hv_run; 
	}
	catch (...)
	{
		cin.get();
		return 0;
	}
}

int main(int argc, char* argv[])
{
	SetSystem("width", 512);
	SetSystem("height", 512);

	SetSystem("use_window_thread", "true");

	// Local control variables
	SetWindowAttr("background_color", "black");
	OpenWindow(0, 0, 640, 480, 0, "", "", &hv_WindowHandle);
	HDevWindowStack::Push(hv_WindowHandle);
	SetPart(hv_WindowHandle, 0, 0, 480, 640);
    int exitCode = EXIT_SUCCESS;

	ofstream datafile;
	datafile.open("C:\\Users\\Administrator\\Desktop\\datafile.txt");
	datafile.clear();
	datafile.close();
    
    try
    {
        CToFCamera::InitProducer();

        CameraAction processing;

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
