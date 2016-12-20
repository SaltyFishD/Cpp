#include "MyHalconFunctions.h"

//找某一点相关的颜色的region
void findRegion(HObject ho_GrayImage, HObject *ho_Region, HTuple hv_x, HTuple hv_y,
	HTuple hv_threashold)
{

	// Local iconic variables
	HObject  ho_EmptyRegion, ho_point, ho_Regions;
	HObject  ho_ConnectedRegions, ho_ObjectSelected, ho_ifEmpty;

	// Local control variables
	HTuple  hv_Grayval, hv_FloorX, hv_FloorY, hv_Number;
	HTuple  hv_Index;

	//
	GenEmptyRegion(&ho_EmptyRegion);
	//
	GetGrayval(ho_GrayImage, hv_x, hv_y, &hv_Grayval);
	//
	//
	TupleFloor(hv_x, &hv_FloorX);
	TupleFloor(hv_y, &hv_FloorY);
	GenRegionPoints(&ho_point, hv_FloorX, hv_FloorY);
	//
	Threshold(ho_GrayImage, &ho_Regions, hv_Grayval - hv_threashold, hv_Grayval + hv_threashold);
	Connection(ho_Regions, &ho_ConnectedRegions);
	CountObj(ho_ConnectedRegions, &hv_Number);
	{
		HTuple end_val13 = hv_Number;
		HTuple step_val13 = 1;
		for (hv_Index = 1; hv_Index.Continue(end_val13, step_val13); hv_Index += step_val13)
		{
			SelectObj(ho_ConnectedRegions, &ho_ObjectSelected, hv_Index);
			Intersection(ho_ObjectSelected, ho_point, &ho_ifEmpty);
			if (0 != (ho_ifEmpty != ho_EmptyRegion))
			{
				(*ho_Region) = ho_ObjectSelected;
			}
			//
		}
	}
	//
	//
	//
	return;
}

void optical_flow(HObject ho_ThisImage, HObject ho_LastImage, HObject ho_RegionROI,
	HObject *ho_result)
{

	// Local iconic variables
	HObject  ho_VectorField, ho_LengthImage, ho_RegionMovement;
	HObject  ho_ConnectedRegions, ho_ConvexHullregion;

	// Local control variables
	HTuple  hv_Min, hv_Max, hv_Range, hv_Area, hv_RCenterNew;
	HTuple  hv_CCenterNew;

	OpticalFlowMg(ho_ThisImage, ho_LastImage, &ho_VectorField, "clg", 0.8, 1, 20, 5,
		"default_parameters", "accurate");
	VectorFieldLength(ho_VectorField, &ho_LengthImage, "squared_length");
	MinMaxGray(ho_RegionROI, ho_LengthImage, 0, &hv_Min, &hv_Max, &hv_Range);
	//
	//
	if (0 != (hv_Max>2))
	{
		Threshold(ho_LengthImage, &ho_RegionMovement, 2, hv_Max);
		Connection(ho_RegionMovement, &ho_ConnectedRegions);
		SelectShapeStd(ho_ConnectedRegions, &ho_RegionMovement, "max_area", 70);
		AreaCenter(ho_RegionMovement, &hv_Area, &hv_RCenterNew, &hv_CCenterNew);
		if (0 != (hv_Area>0))
		{
			ShapeTrans(ho_RegionMovement, &ho_ConvexHullregion, "convex");
			Intersection(ho_RegionROI, ho_ConvexHullregion, &(*ho_result));
		}
	}
	else
	{
		GenEmptyRegion(&(*ho_result));
	}
	//
	return;
}

void colour(HObject ho_Image, HObject *ho_Image_colour, HTuple hv_colour)
{

	// Local iconic variables
	HObject  ho_Image1, ho_Image2, ho_Image3, ho_ImageSub;
	HObject  ho_Regions, ho_Regions1;

	//
	if (0 != (hv_colour == HTuple("blue")))
	{
		Decompose3(ho_Image, &ho_Image1, &ho_Image2, &ho_Image3);
		SubImage(ho_Image3, ho_Image1, &ho_ImageSub, 1, 128);
		Threshold(ho_ImageSub, &ho_Regions, 173, 255);
		SubImage(ho_Image3, ho_Image2, &ho_ImageSub, 1, 128);
		Threshold(ho_ImageSub, &ho_Regions1, 165, 255);
		Intersection(ho_Regions, ho_Regions1, &(*ho_Image_colour));
	}
	else
	{
		Decompose3(ho_Image, &ho_Image1, &ho_Image2, &ho_Image3);
		SubImage(ho_Image1, ho_Image3, &ho_ImageSub, 1, 128);
		Threshold(ho_ImageSub, &ho_Regions, 173, 255);
		SubImage(ho_Image1, ho_Image2, &ho_ImageSub, 1, 128);
		Threshold(ho_ImageSub, &ho_Regions1, 175, 255);
		Intersection(ho_Regions, ho_Regions1, &(*ho_Image_colour));
	}
	//
	//
	return;
}

void disp_message(HTuple hv_WindowHandle, HTuple hv_String, HTuple hv_CoordSystem,
	HTuple hv_Row, HTuple hv_Column, HTuple hv_Color, HTuple hv_Box)
{

	// Local iconic variables

	// Local control variables
	HTuple  hv_Red, hv_Green, hv_Blue, hv_Row1Part;
	HTuple  hv_Column1Part, hv_Row2Part, hv_Column2Part, hv_RowWin;
	HTuple  hv_ColumnWin, hv_WidthWin, hv_HeightWin, hv_MaxAscent;
	HTuple  hv_MaxDescent, hv_MaxWidth, hv_MaxHeight, hv_R1;
	HTuple  hv_C1, hv_FactorRow, hv_FactorColumn, hv_UseShadow;
	HTuple  hv_ShadowColor, hv_Exception, hv_Width, hv_Index;
	HTuple  hv_Ascent, hv_Descent, hv_W, hv_H, hv_FrameHeight;
	HTuple  hv_FrameWidth, hv_R2, hv_C2, hv_DrawMode, hv_CurrentColor;

	//This procedure displays text in a graphics window.
	//
	//Input parameters:
	//WindowHandle: The WindowHandle of the graphics window, where
	//   the message should be displayed
	//String: A tuple of strings containing the text message to be displayed
	//CoordSystem: If set to 'window', the text position is given
	//   with respect to the window coordinate system.
	//   If set to 'image', image coordinates are used.
	//   (This may be useful in zoomed images.)
	//Row: The row coordinate of the desired text position
	//   If set to -1, a default value of 12 is used.
	//Column: The column coordinate of the desired text position
	//   If set to -1, a default value of 12 is used.
	//Color: defines the color of the text as string.
	//   If set to [], '' or 'auto' the currently set color is used.
	//   If a tuple of strings is passed, the colors are used cyclically
	//   for each new textline.
	//Box: If Box[0] is set to 'true', the text is written within an orange box.
	//     If set to' false', no box is displayed.
	//     If set to a color string (e.g. 'white', '#FF00CC', etc.),
	//       the text is written in a box of that color.
	//     An optional second value for Box (Box[1]) controls if a shadow is displayed:
	//       'true' -> display a shadow in a default color
	//       'false' -> display no shadow (same as if no second value is given)
	//       otherwise -> use given string as color string for the shadow color
	//
	//Prepare window
	GetRgb(hv_WindowHandle, &hv_Red, &hv_Green, &hv_Blue);
	GetPart(hv_WindowHandle, &hv_Row1Part, &hv_Column1Part, &hv_Row2Part, &hv_Column2Part);
	GetWindowExtents(hv_WindowHandle, &hv_RowWin, &hv_ColumnWin, &hv_WidthWin, &hv_HeightWin);
	SetPart(hv_WindowHandle, 0, 0, hv_HeightWin - 1, hv_WidthWin - 1);
	//
	//default settings
	if (0 != (hv_Row == -1))
	{
		hv_Row = 12;
	}
	if (0 != (hv_Column == -1))
	{
		hv_Column = 12;
	}
	if (0 != (hv_Color == HTuple()))
	{
		hv_Color = "";
	}
	//
	hv_String = (("" + hv_String) + "").TupleSplit("\n");
	//
	//Estimate extentions of text depending on font size.
	GetFontExtents(hv_WindowHandle, &hv_MaxAscent, &hv_MaxDescent, &hv_MaxWidth, &hv_MaxHeight);
	if (0 != (hv_CoordSystem == HTuple("window")))
	{
		hv_R1 = hv_Row;
		hv_C1 = hv_Column;
	}
	else
	{
		//Transform image to window coordinates
		hv_FactorRow = (1.*hv_HeightWin) / ((hv_Row2Part - hv_Row1Part) + 1);
		hv_FactorColumn = (1.*hv_WidthWin) / ((hv_Column2Part - hv_Column1Part) + 1);
		hv_R1 = ((hv_Row - hv_Row1Part) + 0.5)*hv_FactorRow;
		hv_C1 = ((hv_Column - hv_Column1Part) + 0.5)*hv_FactorColumn;
	}
	//
	//Display text box depending on text size
	hv_UseShadow = 1;
	hv_ShadowColor = "gray";
	if (0 != (HTuple(hv_Box[0]) == HTuple("true")))
	{
		hv_Box[0] = "#fce9d4";
		hv_ShadowColor = "#f28d26";
	}
	if (0 != ((hv_Box.TupleLength())>1))
	{
		if (0 != (HTuple(hv_Box[1]) == HTuple("true")))
		{
			//Use default ShadowColor set above
		}
		else if (0 != (HTuple(hv_Box[1]) == HTuple("false")))
		{
			hv_UseShadow = 0;
		}
		else
		{
			hv_ShadowColor = ((const HTuple&)hv_Box)[1];
			//Valid color?
			try
			{
				SetColor(hv_WindowHandle, HTuple(hv_Box[1]));
			}
			// catch (Exception) 
			catch (HalconCpp::HException &HDevExpDefaultException)
			{
				HDevExpDefaultException.ToHTuple(&hv_Exception);
				hv_Exception = "Wrong value of control parameter Box[1] (must be a 'true', 'false', or a valid color string)";
				throw HalconCpp::HException(hv_Exception);
			}
		}
	}
	if (0 != (HTuple(hv_Box[0]) != HTuple("false")))
	{
		//Valid color?
		try
		{
			SetColor(hv_WindowHandle, HTuple(hv_Box[0]));
		}
		// catch (Exception) 
		catch (HalconCpp::HException &HDevExpDefaultException)
		{
			HDevExpDefaultException.ToHTuple(&hv_Exception);
			hv_Exception = "Wrong value of control parameter Box[0] (must be a 'true', 'false', or a valid color string)";
			throw HalconCpp::HException(hv_Exception);
		}
		//Calculate box extents
		hv_String = (" " + hv_String) + " ";
		hv_Width = HTuple();
		{
			HTuple end_val93 = (hv_String.TupleLength()) - 1;
			HTuple step_val93 = 1;
			for (hv_Index = 0; hv_Index.Continue(end_val93, step_val93); hv_Index += step_val93)
			{
				GetStringExtents(hv_WindowHandle, HTuple(hv_String[hv_Index]), &hv_Ascent,
					&hv_Descent, &hv_W, &hv_H);
				hv_Width = hv_Width.TupleConcat(hv_W);
			}
		}
		hv_FrameHeight = hv_MaxHeight*(hv_String.TupleLength());
		hv_FrameWidth = (HTuple(0).TupleConcat(hv_Width)).TupleMax();
		hv_R2 = hv_R1 + hv_FrameHeight;
		hv_C2 = hv_C1 + hv_FrameWidth;
		//Display rectangles
		GetDraw(hv_WindowHandle, &hv_DrawMode);
		SetDraw(hv_WindowHandle, "fill");
		//Set shadow color
		SetColor(hv_WindowHandle, hv_ShadowColor);
		if (0 != hv_UseShadow)
		{
			DispRectangle1(hv_WindowHandle, hv_R1 + 1, hv_C1 + 1, hv_R2 + 1, hv_C2 + 1);
		}
		//Set box color
		SetColor(hv_WindowHandle, HTuple(hv_Box[0]));
		DispRectangle1(hv_WindowHandle, hv_R1, hv_C1, hv_R2, hv_C2);
		SetDraw(hv_WindowHandle, hv_DrawMode);
	}
	//Write text.
	{
		HTuple end_val115 = (hv_String.TupleLength()) - 1;
		HTuple step_val115 = 1;
		for (hv_Index = 0; hv_Index.Continue(end_val115, step_val115); hv_Index += step_val115)
		{
			hv_CurrentColor = ((const HTuple&)hv_Color)[hv_Index % (hv_Color.TupleLength())];
			if (0 != (HTuple(hv_CurrentColor != HTuple("")).TupleAnd(hv_CurrentColor != HTuple("auto"))))
			{
				SetColor(hv_WindowHandle, hv_CurrentColor);
			}
			else
			{
				SetRgb(hv_WindowHandle, hv_Red, hv_Green, hv_Blue);
			}
			hv_Row = hv_R1 + (hv_MaxHeight*hv_Index);
			SetTposition(hv_WindowHandle, hv_Row, hv_C1);
			WriteString(hv_WindowHandle, HTuple(hv_String[hv_Index]));
		}
	}
	//Reset changed window settings
	SetRgb(hv_WindowHandle, hv_Red, hv_Green, hv_Blue);
	SetPart(hv_WindowHandle, hv_Row1Part, hv_Column1Part, hv_Row2Part, hv_Column2Part);
	return;
}

CToFCamera::Coord3D slideFilter(CToFCamera::Coord3D pillarCoor)
{
	static const int filterSize = 30;
	static CToFCamera::Coord3D sFCoor[filterSize];
	static bool withTenData = true;
	static int dataCount = 0;
	static float sumX = 0;
	static float sumY = 0;
	static float sumZ = 0;
	CToFCamera::Coord3D resultCoor;
	if (pillarCoor.IsValid())
	{
		if (dataCount > filterSize - 1)
		{
			withTenData = false;
			dataCount = 0;
		}
		if (withTenData)
		{
			sumX += pillarCoor.x;
			sumY += pillarCoor.y;
			sumZ += pillarCoor.z;

			resultCoor.x = sumX / (dataCount + 1);
			resultCoor.y = sumY / (dataCount + 1);
			resultCoor.z = sumZ / (dataCount + 1);
		}
		else
		{
			sumX = sumX + pillarCoor.x - sFCoor[dataCount].x;
			sumY = sumY + pillarCoor.y - sFCoor[dataCount].y;
			sumZ = sumZ + pillarCoor.z - sFCoor[dataCount].z;
			resultCoor.x = sumX / filterSize;
			resultCoor.y = sumY / filterSize;
			resultCoor.z = sumZ / filterSize;
		}
		sFCoor[dataCount] = pillarCoor;
		dataCount++;
	}
	else
	{
		resultCoor.x = sumX / filterSize;
		resultCoor.y = sumY / filterSize;
		resultCoor.z = sumZ / filterSize;
	}
	return resultCoor;
}

CToFCamera::Coord3D middleFilter(CToFCamera::Coord3D pillarCoor)
{
	static const int FILTERSIZE = 30;
	static CToFCamera::Coord3D sFCoor[FILTERSIZE];
	CToFCamera::Coord3D sortedCoor[FILTERSIZE];
	static int dataCount = 0;
	static bool withTenData = true;
	CToFCamera::Coord3D resultCoor;

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
					CToFCamera::Coord3D tmp = sortedCoor[k];
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
					CToFCamera::Coord3D tmp = sortedCoor[k];
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