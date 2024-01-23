#pragma once
#include "Common.h"
#include <VesselAPI.h>

/**
 * @file
 * @brief UACS astronaut API header.
*/

namespace UACS
{
	namespace Core { class Astronaut; }

	struct NearestAirlock
	{
		OBJHANDLE hVessel;
		size_t airlockIdx;

		/// The airlock position is converted from the airlock vessel-relative coordinates to astronaut-relative coordinates.
		AirlockInfo airlockInfo;

		/// The first empty station index.
		size_t stationIdx;
	};

	struct NearestAction
	{
		OBJHANDLE hVessel;
		size_t actionIdx;
		ActionInfo actionInfo;
	};

	/// UACS astronaut API.
	class Astronaut : public VESSEL4
	{
	public:
		Astronaut(OBJHANDLE hVessel, int fModel = 1);
		~Astronaut();

		/**
		 * @brief The astronaut information set callback. It must be implemented by astronauts to change the astronaut status accordingly.
		 * @param astrInfo The astronaut information.
		 * @return True if the astronaut information is set, false if not.
		*/
		virtual bool clbkSetAstrInfo(const AstrInfo& astrInfo) = 0;

		/**
		 * @brief The astronaut information callback. It must be implemented by astronauts.
		 * @return A const pointer to the AstrInfo struct, which must live until the astronaut is destroyed. Don't pass the address of a temporary variable.
		*/
		virtual const AstrInfo* clbkGetAstrInfo() = 0;

		/**
		 * @brief Gets UACS version.
		 * @return UACS version.
		*/
		std::string_view GetUACSVersion();

		/**
		 * @brief Gets the astronaut count in the scenario.
		 * @return The astronaut count in the scenario.
		*/
		size_t GetScnAstrCount();

		/**
		 * @brief Gets an astronaut information by the astronaut index.
		 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
		 * @return A pair of the astronaut vessel handle and a pointer to the astronaut AstrInfo struct.
		*/
		std::pair<OBJHANDLE, const AstrInfo*> GetAstrInfoByIndex(size_t astrIdx);

		/**
		 * @brief Gets an astronaut information by the astronaut handle.
		 * @param hAstr The astronaut vessel handle.
		 * @return A pointer to the astronaut AstrInfo struct, or nullptr if hAstr is invalid or not an astronaut.
		*/
		const AstrInfo* GetAstrInfoByHandle(OBJHANDLE hAstr);

		/**
		 * @brief Gets a vessel astronaut information by the vessel handle.
		 * @param hVessel The vessel handle.
		 * @return A pointer to the vessel VslAstrInfo struct, or nullptr if hVessel is invalid or not an astronaut.
		*/
		const VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel);

		/**
		 * @brief Gets the nearest airlock within the passed range.
		 * @param range The search range in meters.
		 * @param airlockOpen Set true if the airlock must be open, false if not.
		 * @param stationEmpty Set true if the vessel must have at least one empty station, false if not.
		 * @return The nearest airlock.
		*/
		std::optional<NearestAirlock> GetNearestAirlock(double range, bool airlockOpen = true, bool stationEmpty = true);

		/**
		 * @brief Gets the nearest breathable vessel (either a cargo or station) within the passed range.
		 * @return A pair of the nearest breathable vessel handle and its relative position. If no vessel was found, a nullptr and empty position is returned.
		*/
		std::pair<OBJHANDLE, VECTOR3> GetNearestBreathable(double range);

		/**
		 * @brief Gets the nearest enabled action area in the passed range.
		 * @param range The search range in meters.
		 * @param actionEnabled Set true if the action area must be enabled, false if not.
		 * @return The nearest action.
		*/
		std::optional<NearestAction> GetNearestAction(double range, bool areaEnabled = true);

		/**
		 * @brief Determines whether the vessel is in a breathable vessel or atmosphere.
		 *
		 * The surrounding atmosphere is considered breathable if its temperature is between 223 and 373 kelvin, and pressure between 36 and 250 kPa.
		 * The vessel is considered in a breathable vessel if the distance between the vessel and the nearest breathable vessel is less than the breathable vessel radius.
		 * @param checkAtm Set true to check if the vessel is in a breathable atmosphere or vessel, false to check for breathable vessels only.
		 * @return True if the vessel is in a breathable area, false if not.
		*/
		bool InBreathable(bool checkAtm = true);

		/**
		 * @brief Ingresses the astronaut to the passed station via the passed airlock in the passed vessel.
		 * @param hVessel The vessel handle. If nullptr is passed, the nearest airlock in a 10 km range is used.
		 * @param airlockIdx The airlock index. If nullopt is passed, the first open airlock is used.
		 * @param stationIdx The station index. If nullopt is passed, the first empty station is used.
		 * @return The ingress result.
		*/
		IngressResult Ingress(OBJHANDLE hVessel = nullptr, std::optional<size_t> airlockIdx = {}, std::optional<size_t> stationIdx = {});

		/**
		 * @brief Triggers the passed action area in the passed vessel.
		 * @param hVessel The vessel handle. If nullptr is passed, the nearest action area in a 10 km range is used.
		 * @param actionIdx The action area index. If nullopt is passed, the first enabled action area is used.
		 * @return The trigger result as the IngressResult enum.
		*/
		IngressResult TriggerAction(OBJHANDLE hVessel = nullptr, std::optional<size_t> actionIdx = {});

	private:
		HINSTANCE coreDLL;
		Core::Astronaut* pCoreAstr{};
	};
}