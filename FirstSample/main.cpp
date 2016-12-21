#include "stdafx.h"
#include "HalconCpp.h"
#include "HDevThread.h"
#include "MyHalconFunctions.h"
#include "PillarState.h"
#include "FindRegion.h"
#include "MySerial.h"
#include "CoordinateConvert.h"

//����ͷ�궨��
#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif

//�����ռ�
using namespace HalconCpp;
using namespace GenTLConsumerImplHelper;
using namespace std;

//ȫ�ֱ���
HTuple hv_WindowHandle;
HObject depthImage, confidenceImage;
const void *depthData;//���ͼ������
const void *intensityData;
const void *confidenceData;
myCoor3D farPillarCoordinate;
MySerial port;//������

FindRegionList taskList; //����׷����
PillarState myPillarState; //̨��ʶ����

//�ⲿ����
extern double Angle[3];//�����ǽǶ�

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


	//�޸ĸ������
	/*image quality*/

	GenApi::CIntegerPtr ptrConfidenceThreshold = m_Camera.GetParameter("ConfidenceThreshold");
	ptrConfidenceThreshold->SetValue(0);
	GenApi::CIntegerPtr ptrFilterStrength = m_Camera.GetParameter("FilterStrength");
	ptrFilterStrength->SetValue(50);
	GenApi::CIntegerPtr ptrOutlierTolerance = m_Camera.GetParameter("OutlierTolerance");
	ptrOutlierTolerance->SetValue(17088);

	/*ͼ���ʽ����*/

	//GenApi::CIntegerPtr ptrWidth = m_Camera.GetParameter("Width");
	//ptrWidth->SetValue(640);
	//GenApi::CIntegerPtr ptrHeight = m_Camera.GetParameter("Height");
	//ptrHeight->SetValue(480);
	//GenApi::CIntegerPtr ptrOffsetX = m_Camera.GetParameter("OffsetX");
	//ptrOffsetX->SetValue(0);
	//GenApi::CIntegerPtr ptrOffsetY = m_Camera.GetParameter("OffsetY");
	//ptrOffsetY->SetValue(0);

	/*�����ֵ*/

	GenApi::CIntegerPtr ptrDepthMin = m_Camera.GetParameter("DepthMin");
	ptrDepthMin->SetValue(0);
	GenApi::CIntegerPtr ptrDepthMax = m_Camera.GetParameter("DepthMax");
	ptrDepthMax->SetValue(13320);

	/*�ع�ʱ��*/

	GenApi::CEnumerationPtr ptrExposureAuto = m_Camera.GetParameter("ExposureAuto");
	ptrExposureAuto->FromString("Continuous");

	//GenApi::CIntegerPtr ptrExposureTimeSelector = m_Camera.GetParameter("ExposureTimeSelector"); ptrExposureTimeSelector->SetValue(0);

	//GenApi::CFloatPtr ptrExposureTime = m_Camera.GetParameter("ExposureTime");
	//ptrExposureTime->SetValue(10000);

	/*�¶ȣ���ѡ����������LED��*/

	//GenApi::CEnumerationPtr ptrDeviceTemperatureSelector = m_Camera.GetParameter("DeviceTemperatureSelector"); ptrDeviceTemperatureSelector->FromString("SensorBoard");//"LEDBoard"
	//std::cout << "Value of DeviceTemperatureSelector is " << ptrDeviceTemperatureSelector->ToString() << std::endl;

	/*֡��*/
	GenApi::CFloatPtr ptrAcquisitionFrameRate = m_Camera.GetParameter("AcquisitionFrameRate");
	ptrAcquisitionFrameRate->SetValue(20);//���Ϊ20֡
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
		/********************֡����ʾ**********************/
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

			//�������ͼ��
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

			//�ͷ�ͼƬ�ڴ�
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

			/*********************̨��ʶ��***************************/
			/*********************�Ŵ��ķ���*************************/
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
			//		disp_message(hv_WindowHandle, "Զ", "window", farPillarRow, farPillarColumn - 15, "black", "true");


			//		myCoor3D *pFarPillarCoordinate;
			//		pFarPillarCoordinate = (myCoor3D*) depthData + farPillarRow * width + (int)farPillarColumn;
			//		farPillarCoordinate = middleFilter(*pFarPillarCoordinate);
			//		cout << farPillarCoordinate.y << endl;
			//		
			//	}
			//}
			//static bool first = true;
			//if (first)
			//{
			//	datafile << "    " << "��������" << farPillarCoordinate.x << "   " << farPillarCoordinate.y << "    " << farPillarCoordinate.z << endl;
			//	first = false;
			//}
			/***********************�ִ��ķ���****************************************/
			myPillarState.updateCameraData(depthImage, depthData, confidenceData);
			cameraParam tmpParam;
			tmpParam.worldX = 5740;
			tmpParam.worldY = 1160;
			tmpParam.worldZ = 520;
			tmpParam.pitch = -Angle[1] + 6;
			tmpParam.yaw = Angle[2] - 20;

			myCoor3D pillarPixelCoor;
			myCoor3D pillarCamCoor;
			//���ڷ��ص�����������
			pillarPixelCoor = myPillarState.getPillarCoor(tmpParam, myPillarState.middlePillar);
			pillarCamCoor = *((myCoor3D*) depthData + (int)pillarPixelCoor.x * width + (int)pillarPixelCoor.y);
			/********************************************************/

			/***************************����׷��*********************/

			//������޷��̷ɹ������
			taskList.detectRegion(depthImage);

			vector<HObject> regionsFound;

			//�洢�ҵ���region
			regionsFound = taskList.RegionsFound(depthImage);
			//if (regionsFound.size() > 0)
			//{
			//	for (size_t i = 0; i < regionsFound.size(); ++i)
			//	{
			//		HTuple hv_SaucerArea, hv_SaucerRow, hv_SaucerColumn;
			//		if (HDevWindowStack::IsOpen())
			//			DispObj(regionsFound[i], HDevWindowStack::GetActive());
			//		AreaCenter(regionsFound[i], &hv_SaucerArea, &hv_SaucerRow, &hv_SaucerColumn);

			//		myCoor3D *pSaucerCoorTwo[2];
			//		pSaucerCoorTwo[0] = (myCoor3D*) depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() - 1;
			//		pSaucerCoorTwo[1] = (myCoor3D*) depthData + (int)hv_SaucerRow.D() * width + (int)hv_SaucerColumn.D() + 1;
			//		myCoor3D* pSaucerCoordinate = new myCoor3D();
			//		pSaucerCoordinate->x = (pSaucerCoorTwo[0]->x + pSaucerCoorTwo[1]->x) / 2;
			//		pSaucerCoordinate->y = (pSaucerCoorTwo[0]->y + pSaucerCoorTwo[1]->y) / 2;
			//		pSaucerCoordinate->z = (pSaucerCoorTwo[0]->z + pSaucerCoorTwo[1]->z) / 2;

			//		if (pSaucerCoordinate->z > 2000)
			//		{
			//			//��¼����
			//			taskList.findRegionList[i]->recordRegionTrack(*pSaucerCoordinate);

			//			datafile << "���̱��: " << taskList.findRegionList[i]->saucerIndex << " X: " << setw(2) << pSaucerCoordinate->x << " Y: " << setw(2) << pSaucerCoordinate->y << " Z: " << pSaucerCoordinate->z << endl;

			//			datafile << "    " << "��������" << farPillarCoordinate.x << "   " << farPillarCoordinate.y << "    " << farPillarCoordinate.z << endl;
			//		}
			//	}
			//}
			/*****************************************************************/

			
			HTuple  hv_Row, hv_Column, hv_Button;
			HTuple  hv_Grayval, hv_Exception;

			/*������м��˳�����*/
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
