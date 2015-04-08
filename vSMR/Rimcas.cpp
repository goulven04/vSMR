#include "stdafx.h"
#include "Rimcas.hpp"

CRimcas::CRimcas()
{
}

CRimcas::~CRimcas()
{
	Reset();
}

void CRimcas::Reset() {
	RunwayAreas.clear();
	AcColor.clear();
	AcOnRunway.clear();
	RunwayAreas.clear();
	TimeTable.clear();
	MonitoredRunwayArr.clear();
	MonitoredRunwayDep.clear();
}

void CRimcas::OnRefreshBegin() {
	AcColor.clear();
	AcOnRunway.clear();
	TimeTable.clear();
}

void CRimcas::OnRefresh(CRadarTarget Rt, CRadarScreen *instance) {
	GetAcInRunwayArea(Rt, instance);
	GetAcInRunwayAreaSoon(Rt, instance);
}

void CRimcas::OnRefreshEnd() {
	ProcessAircrafts();
}

void CRimcas::AddRunwayArea(CRadarScreen *instance, string runway_name1, string runway_name2, CPosition Left, CPosition Right, float hwidth, float hlenght) {
	RunwayAreas[runway_name1] = GetRunwayArea(instance, Left, Right, hwidth, hlenght);
	RunwayAreas[runway_name2] = GetRunwayArea(instance, Left, Right, hwidth, hlenght);
}

string CRimcas::GetAcInRunwayArea(CRadarTarget Ac, CRadarScreen *instance) {

	POINT AcPosPix = instance->ConvertCoordFromPositionToPixel(Ac.GetPosition().GetPosition());

	for (std::map<string, RunwayAreaType>::iterator it = RunwayAreas.begin(); it != RunwayAreas.end(); ++it)
	{
		if (!MonitoredRunwayDep[string(it->first)])
			continue;

		POINT tTopLeft = instance->ConvertCoordFromPositionToPixel(it->second.topLeft);
		POINT tTopRight = instance->ConvertCoordFromPositionToPixel(it->second.topRight);
		POINT tBottomLeft = instance->ConvertCoordFromPositionToPixel(it->second.bottomLeft);
		POINT tBottomRight = instance->ConvertCoordFromPositionToPixel(it->second.bottomRight);

		vector<POINT> RwyPolygon = { tTopLeft, tTopRight, tBottomRight, tBottomLeft };

		if (Is_Inside(AcPosPix, RwyPolygon)) {
			AcOnRunway.insert(std::pair<string, string>(it->first, Ac.GetCallsign()));
			return string(it->first);
		}
	}

	return string_false;
}

string CRimcas::GetAcInRunwayAreaSoon(CRadarTarget Ac, CRadarScreen *instance) {
	if (Ac.GetGS() < 25)
		return CRimcas::string_false;

	POINT AcPosPix = instance->ConvertCoordFromPositionToPixel(Ac.GetPosition().GetPosition());

	for (std::map<string, RunwayAreaType>::iterator it = RunwayAreas.begin(); it != RunwayAreas.end(); ++it)
	{
		if (!MonitoredRunwayArr[it->first])
			continue;

		if (RunwayTimerShort) {
			for (int i = 15; i <= 60; i = i + 15) {
				POINT tTopLeft = instance->ConvertCoordFromPositionToPixel(it->second.topLeft);
				POINT tTopRight = instance->ConvertCoordFromPositionToPixel(it->second.topRight);
				POINT tBottomLeft = instance->ConvertCoordFromPositionToPixel(it->second.bottomLeft);
				POINT tBottomRight = instance->ConvertCoordFromPositionToPixel(it->second.bottomRight);

				vector<POINT> RwyPolygon = { tTopLeft, tTopRight, tBottomRight, tBottomLeft };

				float Distance = float(Ac.GetGS())*0.514444f;
				Distance = Distance * float(i);
				CPosition TempPosition = Haversine(Ac.GetPosition().GetPosition(), float(Ac.GetTrackHeading()), Distance);
				POINT TempPoint = instance->ConvertCoordFromPositionToPixel(TempPosition);

				if (Is_Inside(TempPoint, RwyPolygon) && !Is_Inside(AcPosPix, RwyPolygon)) {
					TimeTable[it->first][i] = string(Ac.GetCallsign());

					// If aircraft is 30 seconds from landing, then it's considered on the runway

					if (i == 30 || i == 15)
						AcOnRunway.insert(std::pair<string, string>(it->first, string(Ac.GetCallsign())));

					return string(it->first);
					break;
				}
			}
		}
		else {
			for (int i = 30; i <= 120; i = i + 30) {
				POINT tTopLeft = instance->ConvertCoordFromPositionToPixel(it->second.topLeft);
				POINT tTopRight = instance->ConvertCoordFromPositionToPixel(it->second.topRight);
				POINT tBottomLeft = instance->ConvertCoordFromPositionToPixel(it->second.bottomLeft);
				POINT tBottomRight = instance->ConvertCoordFromPositionToPixel(it->second.bottomRight);

				vector<POINT> RwyPolygon = { tTopLeft, tTopRight, tBottomRight, tBottomLeft };

				float Distance = float(Ac.GetGS())*0.514444f;
				Distance = Distance * float(i);
				CPosition TempPosition = Haversine(Ac.GetPosition().GetPosition(), float(Ac.GetTrackHeading()), Distance);
				POINT TempPoint = instance->ConvertCoordFromPositionToPixel(TempPosition);

				if (Is_Inside(TempPoint, RwyPolygon) && !Is_Inside(AcPosPix, RwyPolygon)) {
					TimeTable[it->first][i] = string(Ac.GetCallsign());

					// If aircraft is 30 seconds from landing, then it's considered on the runway

					if (i == 30)
						AcOnRunway.insert(std::pair<string, string>(it->first, string(Ac.GetCallsign())));

					return string(it->first);
					break;
				}
			}
		}


	}

	return CRimcas::string_false;
}

CRimcas::RunwayAreaType CRimcas::GetRunwayArea(CRadarScreen *instance, CPosition Left, CPosition Right, float hwidth, float hlenght) {

	float heading = float(Left.DirectionTo(Right));
	float rheading = float(fmod(heading + 180, 360));

	Left = Haversine(Left, rheading, 250.0f);
	Right = Haversine(Right, heading, 250.0f);
	POINT LeftPt = instance->ConvertCoordFromPositionToPixel(Left);
	POINT RightPt = instance->ConvertCoordFromPositionToPixel(Right);

	float TopLeftheading = float(fmod(heading + 90, 360));
	CPosition TopLeft = Haversine(Left, TopLeftheading, 92.5f);
	POINT TopLeftPt = instance->ConvertCoordFromPositionToPixel(TopLeft);

	float BottomRightheading = float(fmod(heading - 90, 360));
	CPosition BottomRight = Haversine(Right, BottomRightheading, 92.5f);
	POINT BottomRightPt = instance->ConvertCoordFromPositionToPixel(BottomRight);

	CPosition TopRight = Haversine(Right, TopLeftheading, 92.5f);
	POINT TopRightPt = instance->ConvertCoordFromPositionToPixel(TopRight);

	CPosition BottomLeft = Haversine(Left, BottomRightheading, 92.5f);
	POINT BottomLeftPt = instance->ConvertCoordFromPositionToPixel(BottomLeft);

	RunwayAreaType toRender;

	toRender.topLeft = TopLeft;
	toRender.topRight = TopRight;
	toRender.bottomLeft = BottomLeft;
	toRender.bottomRight = BottomRight;

	toRender.set = true;

	return toRender;

}

void CRimcas::ProcessAircrafts() {

	for (std::map<string, RunwayAreaType>::iterator it = RunwayAreas.begin(); it != RunwayAreas.end(); ++it)
	{

		if (!MonitoredRunwayArr[string(it->first)] && !MonitoredRunwayDep[string(it->first)])
			continue;

		bool isOnClosedRunway = false;
		if (ClosedRunway.find(it->first) == ClosedRunway.end()) {
		}
		else {
			if (ClosedRunway[it->first])
				isOnClosedRunway = true;
		}

		if (AcOnRunway.count(string(it->first)) > 1 || isOnClosedRunway) {

			auto AcOnRunwayRange = AcOnRunway.equal_range(it->first);

			// Iterate over all map elements with key == theKey

			for (std::map<string, string>::iterator it2 = AcOnRunwayRange.first; it2 != AcOnRunwayRange.second; ++it2)
			{
				if (isOnClosedRunway)
					AcColor[it2->second] = WarningColor;
				else
					AcColor[it2->second] = WarningColor;
			}

		}

	}

}

void CRimcas::BuildMenu(CRadarScreen *instance, HDC hDC, string menu, RECT Position, map<string, int> items) {
	CDC dc;
	dc.Attach(hDC);

	dc.FillSolidRect(new CRect(Position), RGB(127, 122, 122));

	instance->AddScreenObject(8001, menu.c_str(), Position, true, menu.c_str());

	string menu_title = menu + "        X";
	dc.TextOutA(Position.left + 4, Position.top + 3, menu_title.c_str());

	int topTextPos = Position.top + 3 + dc.GetTextExtent(menu_title.c_str()).cy + 3;
	int offset = 3;

	for (std::map<string, int>::iterator it = items.begin(); it != items.end(); ++it)
	{
		dc.TextOutA(Position.left + 4, topTextPos, it->first.c_str());
		if (it->second != 0) {
			CRect TextRect = { Position.left + 4, topTextPos, Position.left + 4 + dc.GetTextExtent(it->first.c_str()).cx, +topTextPos + dc.GetTextExtent(it->first.c_str()).cy };
			instance->AddScreenObject(it->second, it->first.c_str(), TextRect, false, it->first.c_str());
		}
		topTextPos = topTextPos + dc.GetTextExtent(it->first.c_str()).cy + offset;
	}

	dc.Detach();

}

bool CRimcas::isAcOnRunway(CRadarTarget rt) {

	for (std::map<string, string>::iterator it = AcOnRunway.begin(); it != AcOnRunway.end(); ++it)
	{
		if (it->second == string(rt.GetCallsign()))
			return true;
	}

	return false;
}

COLORREF CRimcas::GetAircraftColor(string AcCallsign, COLORREF Color) {
	if (AcColor.find(AcCallsign) == AcColor.end()) {
		return Color;
	}
	else {
		return AcColor[AcCallsign];
	}
}