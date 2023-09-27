#include "Common.h"

#include <map>

DLLCLBK UACS::Core::Astronaut* CreateAstronaut(UACS::Astronaut* pAstr) { return new UACS::Core::Astronaut(pAstr); }

namespace UACS
{
	namespace Core
	{
		Astronaut::Astronaut(UACS::Astronaut* pAstr) : pAstr(pAstr) { astrVector.push_back(pAstr); }

		void Astronaut::Destroy() noexcept
		{
			std::erase(astrVector, pAstr);
			delete this;
		}

		std::string_view Astronaut::GetUACSVersion() { return Core::GetUACSVersion(); }

		size_t Astronaut::GetScnAstrCount() { return astrVector.size(); }

		std::pair<OBJHANDLE, const UACS::AstrInfo*> Astronaut::GetAstrInfoByIndex(size_t astrIdx) { return Core::GetAstrInfoByIndex(astrIdx); }

		const UACS::AstrInfo* Astronaut::GetAstrInfoByHandle(OBJHANDLE hAstr) { return Core::GetAstrInfoByHandle(hAstr); }

		const UACS::VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return Core::GetVslAstrInfo(hVessel); }

		std::optional<UACS::NearestAirlock> Astronaut::GetNearestAirlock(double range)
		{
			UACS::NearestAirlock nearAirlock;
			UACS::VslAstrInfo* nearVslInfo;

			double nearDistance = range;

			bool found{};

			for (const auto& [hVessel, vesselInfo] : vslAstrMap)
			{
				if (vesselInfo->airlocks.empty() || vesselInfo->stations.empty()) continue;

				VECTOR3 vesselPos;
				pAstr->GetRelativePos(hVessel, vesselPos);

				const double distance = length(vesselPos);

				if (distance > nearDistance + oapiGetSize(hVessel) || vesselInfo->stations.at(GetEmptyStationIndex(vesselInfo->stations)).astrInfo) continue;

				found = true;

				nearAirlock.hVessel = hVessel;
				nearDistance = distance;
				nearVslInfo = vesselInfo;
			}

			if (!found) return {};

			nearAirlock.stationIdx = GetEmptyStationIndex(nearVslInfo->stations);

			nearDistance = range;

			found = {};

			for (size_t airlockIdx{}; airlockIdx < nearVslInfo->airlocks.size(); ++airlockIdx)
			{
				const auto& airlockInfo = nearVslInfo->airlocks.at(airlockIdx);

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

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_B") && std::strcmp(attachLabel, "UACS_RB"))) continue;

				VECTOR3 targetPos;
				pAstr->GetRelativePos(pTarget->GetHandle(), targetPos);
				const double distance = length(targetPos);

				if (passCheck)
				{
					if (distance <= pTarget->GetSize()) return { pTarget->GetHandle(), targetPos };
					continue;
				}

				if (distance >= range + pTarget->GetSize()) continue;

				hNearest = pTarget->GetHandle();
				nearestPos = targetPos;
				range = distance;
			}

			for (UACS::Cargo* pCargo : cargoVector)
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

				if (distance >= range + pCargo->GetSize()) continue;

				hNearest = pCargo->GetHandle();
				nearestPos = cargoPos;
				range = distance;
			}

			return { hNearest, nearestPos };
		}

		bool Astronaut::InBreathableArea()
		{
			const double pressure = pAstr->GetAtmPressure();
			const double temp = pAstr->GetAtmTemperature();

			if (temp > 223 && temp < 373 && pressure > 3.6e4 && pressure < 2.5e5) return true;

			passCheck = true;
			auto result = GetNearestBreathable(0);
			passCheck = false;

			return result.first;
		}

		UACS::IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
		{
			if (hVessel)
			{
				const auto& vesselPair = vslAstrMap.find(hVessel);

				if (vesselPair == vslAstrMap.end()) return UACS::INGRS_ARLCK_UNDEF;

				const auto& airlocks = vesselPair->second->airlocks;

				if (airlocks.empty()) return UACS::INGRS_ARLCK_UNDEF;

				const auto& stations = vesselPair->second->stations;

				if (stations.empty()) return UACS::INGRS_STN_UNDEF;

				VESSEL* pVessel = oapiGetVesselInterface(hVessel);

				if (!airlockIdx)
				{
					static auto pred = [pVessel](const UACS::AirlockInfo& airlockInfo) { return airlockInfo.open && !(airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)); };

					auto airlockIt = std::ranges::find_if(airlocks, pred);

					if (airlockIt == airlocks.end()) return UACS::INGRS_ARLCK_CLSD;

					airlockIdx = airlockIt - airlocks.begin();
				}

				else if (!airlocks.at(*airlockIdx).open) return UACS::INGRS_ARLCK_CLSD;

				if (!stationIdx) stationIdx = GetEmptyStationIndex(stations);

				if (stations.at(*stationIdx).astrInfo) return UACS::INGRS_STN_OCCP;

				const UACS::AirlockInfo& airlockInfo = airlocks.at(*airlockIdx);

				if (airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)) return UACS::INGRS_ARLCK_DCKD;

				VECTOR3 airlockPos;
				pVessel->Local2Global(airlockInfo.pos, airlockPos);
				pAstr->Global2Local(airlockPos, airlockPos);

				if (length(airlockPos) > 10) return UACS::INGRS_NOT_IN_RNG;
			}
			else
			{
				auto nearAirlock = GetNearestAirlock(10);

				if (!nearAirlock) return UACS::INGRS_NOT_IN_RNG;

				hVessel = nearAirlock->hVessel;

				stationIdx = nearAirlock->stationIdx;
			}

			auto& astrInfo = vslAstrMap.at(hVessel)->stations.at(*stationIdx).astrInfo = *pAstr->clbkGetAstrInfo();

			if (VESSEL* pVessel = oapiGetVesselInterface(hVessel); pVessel->Version() >= 3)
			{
				if (!static_cast<VESSEL3*>(pVessel)->clbkGeneric(UACS::MSG, UACS::ASTR_INGRS, &(*stationIdx)))
				{
					astrInfo = {};
					return UACS::INGRS_VSL_REJC;
				}
			}

			oapiDeleteVessel(pAstr->GetHandle(), hVessel);

			oapiSetFocusObject(hVessel);

			return UACS::INGRS_SUCCED;
		}
	}
}