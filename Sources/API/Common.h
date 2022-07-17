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
			/**
			 * @attention This is NOT the astronaut vessel name in the scenario.
			*/
			std::string name;

			std::string role;

			/// The astronaut mass in kilograms.
			double mass;

			/// The astronaut fuel level, from 0 to 1.
			double fuelLvl{ 1 };

			/// The astronaut oxygen level, from 0 to 1.
			double oxyLvl{ 1 };

			/// The astronaut alive flag. True: alive, False: dead.
			bool alive{ true };

			/**
			 * @brief The astronaut class name, used to spawn the astronaut when released from a vessel.
			 *
			 * This is the astronaut vessel config file path from 'Config\Vessels\UACS\Astronauts' folder, without '.cfg'.
			*/
			std::string className{};

			/// The astronaut custom data, which will be passed back to the astronaut when released from a vessel.
			std::string customData{};
		};

		struct StationInfo
		{
			std::string name;

			/**
			 * @brief The station astronaut information.
			 * 
			 * If no astronaut is at station, it will be an empty option.
			*/
			std::optional<AstrInfo> astrInfo;
		};

		struct AirlockInfo
		{
			std::string name;

			/// The airlock position in vessel relative coordinates.
			VECTOR3 pos;

			bool open{ true };

			/// Optional: The dock handle associated with the airlock, which is used to transfer astronaut from a vessel to another.
			DOCKHANDLE hDock{};
		};

		struct VslAstrInfo
		{
			std::vector<AirlockInfo> airlocks;

			std::vector<StationInfo> stations;
		};

		enum CargoType
		{
			STATIC = 0,

			UNPACKABLE_ONLY,

			PACKABLE_UNPACKABLE
		};
	}
}