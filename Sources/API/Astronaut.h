#pragma once
#include "Common.h"
#include <VesselAPI.h>

namespace UACS
{
	namespace API
	{
		struct NearestAirlock
		{
			OBJHANDLE hVessel;
			size_t airlockIdx;
			AirlockInfo airlockInfo;

			/// The first empty station index.
			size_t stationIdx;
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

			/// The passed station (or all stations if no airlock is passed) is occupied.
			INGRS_STN_OCCP,

			INGRS_FAIL
		};

		class AstronautImpl;

		class Astronaut : public VESSEL4
		{
		public:
			Astronaut(OBJHANDLE hVessel, int fModel = 1);
			virtual ~Astronaut();

			/**
			 * @brief Astronaut information set callback. Must be implemented by astronauts.
			 * 
			 * It will be called after the astronaut is egressed.
			*/
			virtual void clbkSetAstrInfo(const AstrInfo& astrInfo) = 0;

			/**
			 * @brief Astronaut information callback. Must be implemented by astronauts.
			 * @return A const pointer to the cargo astronauts struct.
			*/
			virtual const AstrInfo* clbkGetAstrInfo() = 0;

			/**
			 * @brief Gets the nearest open airlock with an empty station in the passed range.
			 * @param range The search range in meters.
			 * @return The nearest airlock.
			*/
			virtual std::optional<NearestAirlock> GetNearestAirlock(double range);

			/**
			 * @brief Gets vessel astronaut information from a vessel.
			 * @param hVessel The vessel handle.
			 * @return A pointer to the vessel astronaut information struct, or nullptr if hVessel is invalid or isn't an astronaut.
			*/
			virtual const VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel);

			/**
			 * @brief Gets astronaut information from an astronaut vessel.
			 * @param hVessel The astronaut vessel handle.
			 * @return A pointer to the astronaut information struct, or nullptr if hVessel is invalid or isn't an astronaut.
			*/
			virtual const AstrInfo* GetAstrInfo(OBJHANDLE hVessel);

			/**
			 * @brief Ingresses the astronaut to the passed station via the passed airlock in the passed vessel.
			 * @param hVessel The vessel handle. If nullptr is passed, the nearest airlock in a 10-meter range will be used.
			 * @param airlockIdx The airlock index.
			 * @param stationIdx The station index.
			 * @return The ingress result as the ingress result enum.
			*/
			virtual IngressResult Ingress(OBJHANDLE hVessel = nullptr, std::optional<size_t> airlockIdx = {}, std::optional<size_t> stationIdx = {});

		private:
			std::unique_ptr<AstronautImpl> astrImpl;
		};
	}
}