#pragma once
#include "HalconCpp.h"
#include "HDevThread.h"
#include "CoordinateConvert.h"
#include <list>
#include <vector>
#include <fstream>
#include <iomanip>
#include <ConsumerImplHelper/ToFCamera.h>


using namespace GenTLConsumerImplHelper;
using namespace HalconCpp;
using namespace std;

void findRegion(HObject ho_GrayImage, HObject *ho_Region, HTuple hv_x, HTuple hv_y, HTuple hv_threashold);
void optical_flow(HObject ho_ThisImage, HObject ho_LastImage, HObject ho_RegionROI,
	HObject *ho_result);
void colour(HObject ho_Image, HObject *ho_Image_colour, HTuple hv_colour);
void disp_message(HTuple hv_WindowHandle, HTuple hv_String, HTuple hv_CoordSystem,
	HTuple hv_Row, HTuple hv_Column, HTuple hv_Color, HTuple hv_Box);

myCoor3D slideFilter(myCoor3D pillarCoor);

myCoor3D middleFilter(myCoor3D pillarCoor);
