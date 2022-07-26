#pragma once
#include <OrbiterAPI.h>
#include <vector>
#include <optional>

namespace UACS
{
	namespace API
	{
		struct AstrInfo
		{
			/// This is the astronaut actual name, NOT the astronaut vessel name in the scenario.
			std::string name;

			std::string role;

			/// The astronaut mass in kilograms.
			double mass;

			/// The astronaut fuel level, from 0 to 1.
			double fuelLvl{ 1 };

			/// The astronaut oxygen level, from 0 to 1.
			double oxyLvl{ 1 };

			/// The astronaut alive flag.
			bool alive{ true };

			/// The astronaut custom data, which will be passed back to the astronaut when released from a vessel.
			std::string customData{};

			/**
			 * @brief The astronaut class name, used to spawn the astronaut when released from a vessel.
			 * It shouldn't be changed when setting scenario astronaut information by SetScnAstrInfoByIndex or SetScnAstrInfoByHandle.
			 * 
			 * This is the astronaut vessel config file path from 'Config\Vessels\UACS\Astronauts' folder, without '.cfg'.
			*/
			std::string className{};
		};

		struct StationInfo
		{
			std::string name;
			std::optional<AstrInfo> astrInfo;
		};

		struct AirlockInfo
		{
			std::string name;

			/// The airlock position in vessel-relative coordinates.
			VECTOR3 pos;

			bool open{ true };

			/// Optional: The dock handle associated with the airlock, which is used to transfer astronaut to a docked vessel.
			DOCKHANDLE hDock{};
		};

		struct VslAstrInfo
		{
			std::vector<AirlockInfo> airlocks;
			std::vector<StationInfo> stations;
		};

		enum IngressResult
		{
			INGRS_SUCCED,

			/// No suitable airlock is within 10-meter range if hVessel is nullptr, or the passed vessel is outside the 10-meter range.
			INGRS_NOT_IN_RNG,

			/// The passed vessel has no airlocks.
			INGRS_ARLCK_UNDEF,

			/// The passed airlock (or all airlocks if no airlock is passed) is closed.
			INGRS_ARLCK_CLSD,

			/// The passed vessel has no stations.
			INGRS_STN_UNDEF,

			/// The passed station (or all stations if no station is passed) is occupied.
			INGRS_STN_OCCP,

			INGRS_FAIL
		};

		enum CargoType
		{
			STATIC,
			UNPACK_ONLY,
			PACK_UNPACK
		};
	}
}