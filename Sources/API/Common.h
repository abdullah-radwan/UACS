#pragma once
#include <OrbiterAPI.h>
#include <vector>
#include <optional>

namespace UACS
{
	namespace API
	{
		constexpr int UACS_MSG = 0x55414353;

		struct GroundInfo
		{
			/**
			 * @brief The ground initial release position in vessel-relative coordinates. If nullopt is passed, it's set to airlock/slot position.
			 * @note Y value is ignored, as it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> pos;

			/**
			 * @brief If true and an object (astronaut/cargo) is in the initial release position, the onboard object can't be released.
			 * Otherwise, the onboard object is released in a table as specified by the other options.
			 * @note It should be set true if the vessel is an astronaut.
			*/
			bool singleObject{ false };

			/**
			 * @brief The ground column direction in vessel-relative coordinates. If nullopt is passed, it is calculated based on pos (see UACS developer manual).
			 * @note It must be perpendicular to rowDir and normalised. Y value is ignored, as it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> colDir;

			/**
			 * @brief The ground row direction in vessel-relative coordinates. If nullopt is passed, it is calculated based on pos (see UACS developer manual).
			 * @note It must be perpendicular to colDir and normalised. Y value is ignored, as it's determined by the surface elevation.
			*/
			std::optional<VECTOR3> rowDir;

			size_t colCount{ 3 };

			size_t rowCount{ 3 };

			/// The space betweem each column in meters.
			double colSpace{ 1.5 };

			/// The space between each row in meters.
			double rowSpace{ 1.5 };
		};

		struct AstrInfo
		{
			/// This is the astronaut person name, NOT the astronaut vessel name in the scenario.
			std::string name;

			/// Use standard astronaut roles (see UACS developer manual).
			std::string role;

			/// The astronaut body mass in kilograms. It does NOT include suit, fuel, or oxygen mass.
			double mass;

			/// The astronaut height with the suit on in meters. This is used to set the astronaut height when egressed on ground.
			double height;

			/// The astronaut fuel level, from 0 to 1.
			double fuelLvl{ 1 };

			/// The astronaut oxygen level, from 0 to 1.
			double oxyLvl{ 1 };

			/// The astronaut life flag.
			bool alive{ true };

			/// The astronaut custom data, which is passed to the astronaut when egressed from a vessel.
			std::string customData{};

			/**
			 * @brief The astronaut class name, which is used to spawn the astronaut when egressed from a vessel.
			 *
			 * This is the astronaut vessel config file path from 'Config\Vessels\UACS\Astronauts' folder without '.cfg'.
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

			/**
			 * @brief The airlock position in vessel-relative coordinates.
			 * It's used to position astronauts when egressed in space.
			*/
			VECTOR3 pos;

			/**
			 * @brief The airlock direction in vessel-relative coordinates.
			 * It's used to orientate astronauts when egressed in space.
			 * @note It must be perpendicular to rot and normalised.
			*/
			VECTOR3 dir;

			/**
			 * @brief The airlock longitudinal alignment vector in vessel-relative coordinates.
			 * It's used to orientate astronauts when egressed in space.
			 * @note It must be perpendicular to dir and normalised.
			*/
			VECTOR3 rot;

			bool open{ true };

			/// The astronaut egress velocity (if egressed in space) in meters per second.
			double relVel{ 0 };

			/// The airlock ground egress information.
			GroundInfo gndInfo{};

			/// The dock handle associated with the airlock, which is used to transfer astronaut to a docked vessel.
			DOCKHANDLE hDock{};
		};

		struct VslAstrInfo
		{
			std::vector<StationInfo> stations;
			std::vector<AirlockInfo> airlocks;			
		};

		enum IngressResult
		{
			INGRS_SUCCED,

			/// No suitable airlock is within a 10-meter range if hVessel is nullptr, or the passed vessel is outside the 10-meter range.
			INGRS_NOT_IN_RNG,

			/// The passed vessel has no airlocks.
			INGRS_ARLCK_UNDEF,

			/// The passed airlock (or all airlocks if no airlock is passed) is closed.
			INGRS_ARLCK_CLSD,

			/// A vessel is docked to the passed airlock docking port.
			INGRS_ARLCK_DCKD,

			/// The passed vessel has no stations.
			INGRS_STN_UNDEF,

			/// The passed station (or all stations if no station is passed) is occupied.
			INGRS_STN_OCCP,

			INGRS_FAIL
		};

		enum CargoType { STATIC, UNPACKABLE };
	}
}