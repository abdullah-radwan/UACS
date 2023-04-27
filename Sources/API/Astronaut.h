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

	namespace API
	{
		struct NearestAirlock
		{
			OBJHANDLE hVessel;
			size_t airlockIdx;

			/// The airlock position is converted from the airlock vessel relative coordinates to the astronaut vessel relative coordinates.
			AirlockInfo airlockInfo;

			/// The first empty station index.
			size_t stationIdx;
		};

		class Astronaut : public VESSEL4
		{
		public:
			Astronaut(OBJHANDLE hVessel, int fModel = 1);
			~Astronaut();

			/**
			 * @brief The astronaut information set callback. It must be implemented by astronauts to change the astronaut status accordingly.
			 * 
			 * It will be called whenever the astronaut is egressed or a vessel manually changes the information.
			*/
			virtual void clbkSetAstrInfo(const AstrInfo& astrInfo) = 0;

			/**
			 * @brief The astronaut information callback. It must be implemented by astronauts.
			 * @return A const pointer to the AstrInfo struct. The struct must live until the astronaut is destroyed.
			 * Don't pass the address of a temporary variable.
			*/
			virtual const AstrInfo* clbkGetAstrInfo() = 0;

			/**
			 * @brief Gets UACS version. It can be used to know if UACS is installed.
			 * @return UACS version if UACS is installed, or an empty string if not.
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
			 * @brief Sets a scenario astronaut information by the astronaut index.
			 * @note This is used to change a scenario astronaut information.
			 * To change a vessel astronaut information, change the information in the VslAstrInfo struct directly.
			 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
			 * @param astrInfo The astronaut information.
			*/
			void SetScnAstrInfoByIndex(size_t astrIdx, AstrInfo astrInfo);

			/**
			 * @brief Sets a scenario astronaut information by the astronaut handle.
			 * @note This is used to change a scenario astronaut information.
			 * To change a vessel astronaut information, change the information in the VslAstrInfo struct directly.
			 * @param hAstr The astronaut vessel handle.
			 * @param astrInfo The astronaut information.
			 * @return True if hAstr is an astronaut, false if not.
			*/
			bool SetScnAstrInfoByHandle(OBJHANDLE hAstr, AstrInfo astrInfo);

			/**
			 * @brief Gets the nearest airlock with an empty station in the passed range.
			 * @param range The search range in meters.
			 * @return The nearest airlock.
			*/
			std::optional<NearestAirlock> GetNearestAirlock(double range);

			/**
			 * @brief Gets the nearest breathable vessel in the breathable search range.
			 * @note The nearest breathable vessel isn't necessarily a cargo.
			 * @return A pair of the nearest breathable vessel handle and its relative position. If no vessel was found, a nullptr and empty position will be returned.
			*/
			std::pair<OBJHANDLE, VECTOR3> GetNearestBreathable(double range);

			/**
			 * @brief Determines whether the vessel is in a breathable area.
			 * 
			 * The vessel is considered in a breathable area if the distance between the vessel and the nearest breathable vessel is less than the breathable vessel radius.
			 * @return True if the vessel is in a breathable area, false if not.
			*/
			bool InBreathableArea();

			/**
			 * @brief Ingresses the astronaut to the passed station via the passed airlock in the passed vessel.
			 * @param hVessel The vessel handle. If nullptr is passed, the nearest airlock in a 10-meter range will be used.
			 * @param airlockIdx The airlock index. If nullopt is passed, the first open airlock will be used.
			 * @param stationIdx The station index. If nullopt is passed, the first empty station will be used.
			 * @return The ingress result.
			*/
			IngressResult Ingress(OBJHANDLE hVessel = nullptr, std::optional<size_t> airlockIdx = {}, std::optional<size_t> stationIdx = {});

		private:
			HINSTANCE coreDLL;
			Core::Astronaut* pCoreAstr{};
		};
	}
}