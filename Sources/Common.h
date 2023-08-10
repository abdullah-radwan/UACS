#pragma once
#include <Orbitersdk.h>
#include <string>
#include <optional>

namespace UACS
{
	inline void WarnAndTerminate(const char* warning, const char* className, const char* type)
	{
		oapiWriteLogV("UACS fatal error: The %s of %s %s is not specified", warning, className, type);

		std::terminate();
	}

	inline VESSELSTATUS2 GetVesselStatus(const VESSEL* pVessel)
	{
		VESSELSTATUS2 status;
		memset(&status, 0, sizeof(status));
		status.version = 2;
		pVessel->GetStatusEx(&status);

		return status;
	}

	inline void SetSpawnName(std::string& name)
	{
		for (size_t index{}; ++index;)
		{
			// Add the index to the string. c_str() is used to avoid a bug
			std::string spawnName = name.c_str() + std::to_string(index);
			// If the spawn name doesn't exists
			if (!oapiGetVesselByName(spawnName.data())) { name = spawnName; return; }
		}
	}

	inline double DistLngLat(double bodySize, double lng1, double lat1, double lng2, double lat2)
	{
		double latOffset = (lat2 - lat1) * 0.5;
		double lngOffset = (lng2 - lng1) * 0.5;

		double a = sin(latOffset) * sin(latOffset) + cos(lat1) * cos(lat2) * sin(lngOffset) * sin(lngOffset);
		double c = 2 * atan2(sqrt(a), sqrt(1 - a));

		return c * bodySize;
	}

	inline MATRIX3 RotationMatrix(VECTOR3 angles)
	{
		const MATRIX3 RM_X = _M(1, 0, 0, 0, cos(angles.x), -sin(angles.x), 0, sin(angles.x), cos(angles.x));
		const MATRIX3 RM_Y = _M(cos(angles.y), 0, sin(angles.y), 0, 1, 0, -sin(angles.y), 0, cos(angles.y));
		const MATRIX3 RM_Z = _M(cos(angles.z), -sin(angles.z), 0, sin(angles.z), cos(angles.z), 0, 0, 0, 1);

		return mul(RM_X, mul(RM_Y, RM_Z));
	}

	inline void SetGroundRotation(VESSELSTATUS2& status, double height)
	{
		const MATRIX3 rot1 = RotationMatrix({ 0, PI05 - status.surf_lng, 0 });
		const MATRIX3 rot2 = RotationMatrix({ -status.surf_lat, 0, 0 });
		const MATRIX3 rot3 = RotationMatrix({ 0, 0, PI + status.surf_hdg });
		const MATRIX3 rot4 = RotationMatrix({ PI05, 0, 0 });
		const MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

		status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
		status.arot.y = -asin(RotMatrix_Def.m13);
		status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);

		status.vrot.x = height;
	}

	inline void SetGroundRotation(VESSELSTATUS2& status, double height, double lngOffset, double latOffset, bool needTrans = false)
	{
		if (needTrans)
		{
			const double bodySize = oapiGetSize(status.rbody);

			status.surf_lng += lngOffset / bodySize;
			status.surf_lat += latOffset / bodySize;
		}
		else
		{
			status.surf_lng += lngOffset;
			status.surf_lat += latOffset;
		}

		SetGroundRotation(status, height);
	}

	inline void SetGroundRotation(VESSELSTATUS2& status, VECTOR3 frontPos, std::optional<VECTOR3> rightPos, std::optional<VECTOR3> leftPos,
		double lngOffset = 0, double latOffset = 0, bool needTrans = false)
	{
		if (!rightPos || !leftPos)
		{
			SetGroundRotation(status, abs(frontPos.y), lngOffset, latOffset, needTrans);
			return;
		}

		const double bodySize = oapiGetSize(status.rbody);

		if (needTrans)
		{
			status.surf_lng += lngOffset / bodySize;
			status.surf_lat += latOffset / bodySize;
		}
		else
		{
			status.surf_lng += lngOffset;
			status.surf_lat += latOffset;
		}

		const double frontLng = (frontPos.z * sin(status.surf_hdg)) / bodySize;
		const double frontLat = (frontPos.z * cos(status.surf_hdg)) / bodySize;
		const double frontElev = oapiSurfaceElevation(status.rbody, status.surf_lng + frontLng, status.surf_lat + frontLat);

		const double rightLng = ((*rightPos).z * sin(status.surf_hdg) + (*rightPos).x * cos(status.surf_hdg)) / bodySize;
		const double rightLat = ((*rightPos).z * cos(status.surf_hdg) - (*rightPos).x * sin(status.surf_hdg)) / bodySize;
		const double rightElev = oapiSurfaceElevation(status.rbody, status.surf_lng + rightLng, status.surf_lat + rightLat);

		const double leftLng = ((*leftPos).z * sin(status.surf_hdg) + (*leftPos).x * cos(status.surf_hdg)) / bodySize;
		const double leftLat = ((*leftPos).z * cos(status.surf_hdg) - (*leftPos).x * sin(status.surf_hdg)) / bodySize;
		const double leftElev = oapiSurfaceElevation(status.rbody, status.surf_lng + leftLng, status.surf_lat + leftLat);

		const double pitchAngle = atan2(((rightElev + leftElev) * 0.5) - frontElev, (*rightPos).z - frontPos.z);
		const double rollAngle = -atan2(leftElev - rightElev, (*leftPos).x - (*rightPos).x);

		const MATRIX3 rot1 = RotationMatrix({ 0, PI05 - status.surf_lng, 0 });
		const MATRIX3 rot2 = RotationMatrix({ -status.surf_lat, 0, 0 });
		const MATRIX3 rot3 = RotationMatrix({ 0, 0, PI + status.surf_hdg });
		const MATRIX3 rot4 = RotationMatrix({ PI05 - pitchAngle, 0, rollAngle });
		const MATRIX3 RotMatrix_Def = mul(rot1, mul(rot2, mul(rot3, rot4)));

		status.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
		status.arot.y = -asin(RotMatrix_Def.m13);
		status.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);

		status.vrot.x = abs(frontPos.y);
	}
}