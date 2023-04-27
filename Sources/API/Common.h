#pragma once
#include <OrbiterAPI.h>
#include <vector>
#include <optional>

namespace UACS
{
	namespace API
	{
		constexpr int UACS_MSG = 0x55414353;

		struct AstrInfo
		{
			/// This is the astronaut actual name, NOT the astronaut vessel name in the scenario.
			std::string name;

			std::string role;

			/// The astronaut body mass in kilograms. It does NOT include suit, fuel, or oxygen mass.
			double mass;

			/// The astronaut height is meters. This is used to set the astronaut height when released on ground.
			double height;

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

		struct GroundInfo
		{
			/**
			 * @brief The ground release position in vessel-relative coordinates. If no value is passed, it will be the same as airlock/slot position.
			 * @note Y value is ignored, because it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> pos;

			/**
			 * @brief The ground column direction in vessel-relative coordinates. If no value is passed, it will be calculated based on pos (see UACS manual).
			 * @note It should be normalised to length 1, and it should be perpendicular to rowDir.
			 * Y value is ignored, as it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> colDir;

			/**
			 * @brief The ground row direction in vessel-relative coordinates. If no value is passed, it will be calculated based on pos (see UACS manual).
			 * @note It should be normalised to length 1, and it should be perpendicular to colDir.
			 * Y value is ignored, as it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> rowDir;

			/// The maximum cargo count in a column.
			size_t cargoCount{ 3 };

			/// The maximum column count.
			size_t colCount{ 3 };

			/// The space betweem each cargo in meters.
			double cargoSpace{ 1.5 };

			/// The space between each column in meters.
			double colSpace{ 1.5 };
		};

		struct AirlockInfo
		{
			std::string name;

			/**
			 * @brief The airlock position in vessel-relative coordinates.
			 * It's used to position astronauts if egressed in space.
			*/
			VECTOR3 pos;

			/**
			 * @brief The airlock direction in vessel-relative coordinates.
			 * It's used to orientate astronauts if egressed in space.
			 * @note It should be normalised to length 1, and it should be perpendicular to rot.
			*/
			VECTOR3 dir;

			/**
			 * @brief The airlock longitudinal alignment vector in vessel-relative coordinates.
			 * It's used to orientate astronauts if egressed in space.
			 * @note It should be normalised to length 1, and it should be perpendicular to dir.
			*/
			VECTOR3 rot;

			bool open{ true };

			/// The astronaut egress velocity (if egressed in space) in meters per second.
			double relVel{};

			/// The airlock ground egress information.
			GroundInfo gndInfo{};

			/// The dock handle associated with the airlock, which is used to transfer astronaut to a docked vessel.
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

		enum CargoType { STATIC, UNPACKABLE };
	}
}