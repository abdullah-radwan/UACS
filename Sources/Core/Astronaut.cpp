#include "CoreCommon.h"

#include <map>

DLLCLBK UACS::Core::Astronaut* CreateAstronaut(UACS::API::Astronaut* pAstr) { return new UACS::Core::Astronaut(pAstr); }

namespace UACS
{
	namespace Core
	{
		Astronaut::Astronaut(API::Astronaut* pAstr) : pAstr(pAstr) { astrVector.push_back(pAstr); }

		void Astronaut::Destroy() noexcept
		{
			std::erase(astrVector, pAstr);
			delete this;
		}

		std::string_view Astronaut::GetUACSVersion() { return Core::GetUACSVersion(); }

		size_t Astronaut::GetScnAstrCount() { return Core::GetScnAstrCount(); }

		std::pair<OBJHANDLE, const API::AstrInfo*> Astronaut::GetAstrInfoByIndex(size_t astrIdx) { return Core::GetAstrInfoByIndex(astrIdx); }

		const API::AstrInfo* Astronaut::GetAstrInfoByHandle(OBJHANDLE hAstr) { return Core::GetAstrInfoByHandle(hAstr); }

		const API::VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return Core::GetVslAstrInfo(hVessel); }

		void Astronaut::SetScnAstrInfoByIndex(size_t astrIdx, API::AstrInfo astrInfo) { Core::SetScnAstrInfoByIndex(astrIdx, astrInfo); }

		bool Astronaut::SetScnAstrInfoByHandle(OBJHANDLE hAstr, API::AstrInfo astrInfo) { return Core::SetScnAstrInfoByHandle(hAstr, astrInfo); }

		std::optional<API::NearestAirlock> Astronaut::GetNearestAirlock(double range)
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

		std::pair<OBJHANDLE, VECTOR3> Astronaut::GetNearestBreathable(double range)
		{
			OBJHANDLE hNearest{};
			VECTOR3 nearestPos{};

			for (size_t idx{}; idx < oapiGetVesselCount(); ++idx)
			{
				VESSEL* pTarget = oapiGetVesselInterface(oapiGetVesselByIndex(idx));

				int attachIdx = int(pTarget->AttachmentCount(true)) - 1;

				if (attachIdx < 0 || !std::strncmp(pTarget->GetClassNameA(), "UACS", 4)) continue;

				const char* attachLabel = pTarget->GetAttachmentId(pTarget->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_B") && std::strcmp(attachLabel, "UACS_BS"))) continue;

				VECTOR3 targetPos;
				pAstr->GetRelativePos(pTarget->GetHandle(), targetPos);
				const double distance = length(targetPos);

				if (passCheck)
				{
					if (distance <= pTarget->GetSize()) return { pTarget->GetHandle(), targetPos };
					continue;
				}

				if (distance >= range) continue;

				hNearest = pTarget->GetHandle();
				nearestPos = targetPos;
				range = distance;
			}

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();
				if (!cargoInfo->breathable || !cargoInfo->unpacked) continue;

				VECTOR3 cargoPos;
				pAstr->GetRelativePos(pCargo->GetHandle(), cargoPos);
				const double distance = length(cargoPos);

				if (passCheck)
				{
					if (distance <= pCargo->GetSize()) return { pCargo->GetHandle(), cargoPos };
					continue;
				}

				if (distance >= range) continue;

				hNearest = pCargo->GetHandle();
				nearestPos = cargoPos;
				range = distance;
			}

			return { hNearest, nearestPos };
		}

		bool Astronaut::InBreathableArea()
		{
			passCheck = true;
			auto result = GetNearestBreathable(0);
			passCheck = false;

			return result.first;
		}

		API::IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
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