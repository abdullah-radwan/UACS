#include "AstronautImpl.h"

#include <map>

DLLCLBK UACS::Core::Astronaut* CreateAstronaut(UACS::API::Astronaut* pAstr) { return new UACS::Core::AstronautImpl(pAstr); }

namespace UACS
{
	namespace Core
	{
		AstronautImpl::AstronautImpl(API::Astronaut* pAstr) : pAstr(pAstr) { astrVector.push_back(pAstr); }

		void AstronautImpl::Destroy() noexcept
		{
			std::erase(astrVector, pAstr);

			delete this;
		}

		std::optional<API::NearestAirlock> AstronautImpl::GetNearestAirlock(double range)
		{
			API::NearestAirlock nearAirlock;
			API::VslAstrInfo* nearVslInfo;

			double nearDistance = range;

			bool found{};

			for (const auto& [hVessel, vesselInfo] : vslAstrMap)
			{
				if (vesselInfo->airlocks.empty() || vesselInfo->stations.empty()) continue;

				VECTOR3 vesselPos;
				pAstr->GetRelativePos(hVessel, vesselPos);

				const double distance = length(vesselPos);

				if (distance > nearDistance) continue;

				found = true;

				nearAirlock.hVessel = hVessel;
				nearDistance = distance;
				nearVslInfo = vesselInfo;
			}

			if (!found) return {};

			auto stationIdx = GetEmptyStationIndex(nearVslInfo->stations);

			if (!stationIdx) return {};

			nearAirlock.stationIdx = *stationIdx;

			nearDistance = range;

			found = {};

			for (size_t airlockIdx{}; airlockIdx < nearVslInfo->airlocks.size(); ++airlockIdx)
			{
				const auto& airlockInfo = nearVslInfo->airlocks.at(airlockIdx);

				if (!airlockInfo.open) continue;

				VECTOR3 airlockPos;
				oapiLocalToGlobal(nearAirlock.hVessel, &airlockInfo.pos, &airlockPos);
				pAstr->Global2Local(airlockPos, airlockPos);

				const double distance = length(airlockPos);

				if (distance > nearDistance) continue;

				found = true;

				nearAirlock.airlockIdx = airlockIdx;
				nearAirlock.airlockInfo = airlockInfo;
				nearAirlock.airlockInfo.pos = airlockPos;
				nearDistance = distance;
			}

			if (!found) return {};

			return nearAirlock;
		}

		const API::VslAstrInfo* AstronautImpl::GetVslAstrInfo(OBJHANDLE hVessel)
		{
			const auto& vesselPair = vslAstrMap.find(hVessel);

			if (vesselPair == vslAstrMap.end()) return {};

			return vesselPair->second;
		}

		const API::AstrInfo* AstronautImpl::GetAstrInfo(OBJHANDLE hVessel)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hVessel](API::Astronaut* pAstr) { return pAstr->GetHandle() == hVessel; });

			return astrIt == astrVector.end() ? nullptr : (*astrIt)->clbkGetAstrInfo();
		}

		API::IngressResult AstronautImpl::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
		{
			if (hVessel)
			{
				const auto& vesselPair = vslAstrMap.find(hVessel);

				if (vesselPair == vslAstrMap.end()) return API::INGRS_ARLCK_UNDEF;

				const auto& airlocks = vesselPair->second->airlocks;

				if (airlocks.empty()) return API::INGRS_ARLCK_UNDEF;

				const auto& stations = vesselPair->second->stations;

				if (stations.empty()) return API::INGRS_STN_UNDEF;

				VESSEL* pVessel = oapiGetVesselInterface(hVessel);

				if (!airlockIdx)
				{
					static auto pred = [pVessel](const API::AirlockInfo& airlockInfo) { return airlockInfo.open && !(airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)); };

					auto airlockIt = std::ranges::find_if(airlocks, pred);

					if (airlockIt == airlocks.end()) return API::INGRS_ARLCK_CLSD;

					airlockIdx = airlockIt - airlocks.begin();
				}

				else if (!airlocks.at(*airlockIdx).open) return API::INGRS_ARLCK_CLSD;

				if (!stationIdx)
				{
					stationIdx = GetEmptyStationIndex(stations);

					if (!stationIdx) return API::INGRS_STN_OCCP;
				}

				else if (stations.at(*stationIdx).astrInfo) return API::INGRS_STN_OCCP;

				const API::AirlockInfo& airlockInfo = airlocks.at(*airlockIdx);

				VECTOR3 airlockPos;
				pVessel->Local2Global(airlockPos, airlockPos);
				pAstr->Global2Local(airlockPos, airlockPos);

				if (length(airlockPos) > 10) return API::INGRS_NOT_IN_RNG;
			}

			else
			{
				auto nearAirlock = GetNearestAirlock(10);

				if (!nearAirlock) return API::INGRS_NOT_IN_RNG;

				hVessel = (*nearAirlock).hVessel;

				stationIdx = (*nearAirlock).stationIdx;
			}

			vslAstrMap.at(hVessel)->stations.at(*stationIdx).astrInfo = *pAstr->clbkGetAstrInfo();

			oapiDeleteVessel(pAstr->GetHandle(), hVessel);

			return API::INGRS_SUCCED;
		}
	}
}