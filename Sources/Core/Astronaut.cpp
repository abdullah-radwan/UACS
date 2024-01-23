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

		std::pair<OBJHANDLE, const AstrInfo*> Astronaut::GetAstrInfoByIndex(size_t astrIdx) { return Core::GetAstrInfoByIndex(astrIdx); }

		const AstrInfo* Astronaut::GetAstrInfoByHandle(OBJHANDLE hAstr) { return Core::GetAstrInfoByHandle(hAstr); }

		const VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return Core::GetVslAstrInfo(hVessel); }

		std::optional<NearestAirlock> Astronaut::GetNearestAirlock(double range, bool airlockOpen, bool stationEmpty)
		{
			std::map<double, std::pair<OBJHANDLE, VslAstrInfo*>> vslMap;

			for (const auto& [hVessel, vslInfo] : vslAstrMap)
			{
				if (vslInfo->airlocks.empty() || vslInfo->stations.empty()) continue;

				VECTOR3 vslPos;
				pAstr->GetRelativePos(hVessel, vslPos);

				const double distance = length(vslPos);

				if (distance <= range + oapiGetSize(hVessel)) vslMap[distance] = { hVessel, vslInfo };
			}

			NearestAirlock nearAirlock;

			for (const auto& [distance, vslInfo] : vslMap)
			{
				nearAirlock.hVessel = vslInfo.first;

				nearAirlock.stationIdx = GetEmptyStationIndex(vslInfo.second->stations);

				if (stationEmpty && vslInfo.second->stations.at(nearAirlock.stationIdx).astrInfo) continue;

				bool found{};
				double nearDistance = range;

				for (size_t airlockIdx{}; airlockIdx < vslInfo.second->airlocks.size(); ++airlockIdx)
				{
					const auto& airlockInfo = vslInfo.second->airlocks.at(airlockIdx);

					if (airlockOpen && !airlockInfo.open) continue;

					VESSEL* pVessel = oapiGetVesselInterface(nearAirlock.hVessel);
					VECTOR3 airlockPos = pVessel->GetFlightStatus() && airlockInfo.gndInfo.pos ? *airlockInfo.gndInfo.pos : airlockInfo.pos;

					pVessel->Local2Global(airlockPos, airlockPos);
					pAstr->Global2Local(airlockPos, airlockPos);

					const double distance = length(airlockPos);

					if (distance > nearDistance) continue;

					found = true;

					nearAirlock.airlockIdx = airlockIdx;
					nearAirlock.airlockInfo = airlockInfo;
					nearAirlock.airlockInfo.pos = airlockPos;
					nearDistance = distance;
				}

				if (found) return nearAirlock;
			}

			return {};
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

		std::optional<NearestAction> Astronaut::GetNearestAction(double range, bool areaEnabled)
		{
			std::map<double, std::pair<OBJHANDLE, VslAstrInfo*>> vslMap;
			
			for (const auto& [hVessel, vslInfo] : vslAstrMap)
			{
				if (vslInfo->actionAreas.empty()) continue;

				VECTOR3 vesselPos;
				pAstr->GetRelativePos(hVessel, vesselPos);

				const double distance = length(vesselPos);

				if (distance <= range + oapiGetSize(hVessel)) vslMap[distance] = { hVessel, vslInfo };
			}

			NearestAction nearAction;

			for (const auto& [distance, vslInfo] : vslMap)
			{
				nearAction.hVessel = vslInfo.first;
				bool found{};
				double nearDistance = range;

				for (size_t actionIdx{}; actionIdx < vslInfo.second->actionAreas.size(); ++actionIdx)
				{
					const auto& actionInfo = vslInfo.second->actionAreas.at(actionIdx);

					if (areaEnabled && !actionInfo.enabled) continue;

					VECTOR3 actionPos;
					oapiLocalToGlobal(nearAction.hVessel, &actionInfo.pos, &actionPos);
					pAstr->Global2Local(actionPos, actionPos);

					const double distance = length(actionPos);

					if (distance > nearDistance) continue;

					found = true;

					nearAction.actionIdx = actionIdx;
					nearAction.actionInfo = actionInfo;
					nearAction.actionInfo.pos = actionPos;
					nearDistance = distance;
				}

				if (found) return nearAction;
			}

			return {};
		}

		bool Astronaut::InBreathable(bool checkAtm)
		{
			if (checkAtm)
			{
				const double pressure = pAstr->GetAtmPressure();
				const double temp = pAstr->GetAtmTemperature();

				if (temp > 223 && temp < 373 && pressure > 3.6e4 && pressure < 2.5e5) return true;
			}

			passCheck = true;
			auto result = GetNearestBreathable(0);
			passCheck = false;

			return result.first;
		}

		IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
		{
			if (hVessel)
			{
				const auto& vesselPair = vslAstrMap.find(hVessel);

				if (vesselPair == vslAstrMap.end()) return INGRS_ARLCK_UNDEF;

				const auto& airlocks = vesselPair->second->airlocks;

				if (airlocks.empty()) return INGRS_ARLCK_UNDEF;

				const auto& stations = vesselPair->second->stations;

				if (stations.empty()) return INGRS_STN_UNDEF;

				VESSEL* pVessel = oapiGetVesselInterface(hVessel);

				if (!airlockIdx)
				{
					static auto pred = [pVessel](const AirlockInfo& airlockInfo) { return airlockInfo.open && !(airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)); };

					auto airlockIt = std::ranges::find_if(airlocks, pred);

					if (airlockIt == airlocks.end()) return INGRS_ARLCK_CLSD;

					airlockIdx = airlockIt - airlocks.begin();
				}

				else if (!airlocks.at(*airlockIdx).open) return INGRS_ARLCK_CLSD;

				if (!stationIdx) stationIdx = GetEmptyStationIndex(stations);

				if (stations.at(*stationIdx).astrInfo) return INGRS_STN_OCCP;

				const AirlockInfo& airlockInfo = airlocks.at(*airlockIdx);

				if (airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)) return INGRS_ARLCK_DCKD;

				VECTOR3 airlockPos = pVessel->GetFlightStatus() && airlockInfo.gndInfo.pos ? *airlockInfo.gndInfo.pos : airlockInfo.pos;

				pVessel->Local2Global(airlockPos, airlockPos);
				pAstr->Global2Local(airlockPos, airlockPos);

				if (length(airlockPos) > airlockInfo.range) return INGRS_NOT_IN_RNG;
			}
			else
			{
				auto nearAirlock = GetNearestAirlock(10e3, true, true);

				if (!nearAirlock || length(nearAirlock->airlockInfo.pos) > nearAirlock->airlockInfo.range) return INGRS_NOT_IN_RNG;

				hVessel = nearAirlock->hVessel;

				stationIdx = nearAirlock->stationIdx;
			}

			auto& astrInfo = vslAstrMap.at(hVessel)->stations.at(*stationIdx).astrInfo = *pAstr->clbkGetAstrInfo();

			if (VESSEL* pVessel = oapiGetVesselInterface(hVessel); pVessel->Version() >= 3)
			{
				if (!static_cast<VESSEL3*>(pVessel)->clbkGeneric(MSG, ASTR_INGRS, &(*stationIdx)))
				{
					astrInfo = {};
					return INGRS_VSL_REJC;
				}
			}

			oapiDeleteVessel(pAstr->GetHandle(), hVessel);
			oapiSetFocusObject(hVessel);

			return INGRS_SUCCED;
		}

		IngressResult Astronaut::TriggerAction(OBJHANDLE hVessel, std::optional<size_t> actionIdx)
		{
			if (hVessel)
			{
				const auto& vesselPair = vslAstrMap.find(hVessel);

				if (vesselPair == vslAstrMap.end()) return INGRS_ARLCK_UNDEF;

				const auto& actionAreas = vesselPair->second->actionAreas;

				if (actionAreas.empty()) return INGRS_ARLCK_UNDEF;

				if (!actionIdx)
				{
					static auto pred = [](const ActionInfo& actionInfo) { return actionInfo.enabled; };

					auto actionIt = std::ranges::find_if(actionAreas, pred);

					if (actionIt == actionAreas.end()) return INGRS_ARLCK_CLSD;

					actionIdx = actionIt - actionAreas.begin();
				}

				else if (!actionAreas.at(*actionIdx).enabled) return INGRS_ARLCK_CLSD;

				const ActionInfo& actionInfo = actionAreas.at(*actionIdx);

				VECTOR3 actionPos;
				oapiLocalToGlobal(hVessel, &actionInfo.pos, &actionPos);
				pAstr->Global2Local(actionPos, actionPos);

				if (length(actionPos) > actionInfo.range) return INGRS_NOT_IN_RNG;
			}
			else
			{
				auto nearAction = GetNearestAction(10e3, true);

				if (!nearAction || length(nearAction->actionInfo.pos) > nearAction->actionInfo.range) return INGRS_NOT_IN_RNG;

				hVessel = nearAction->hVessel;

				actionIdx = nearAction->actionIdx;
			}

			if (VESSEL* pVessel = oapiGetVesselInterface(hVessel); pVessel->Version() >= 3)
				if (!static_cast<VESSEL3*>(pVessel)->clbkGeneric(MSG, ACTN_TRIG, &(*actionIdx)))
					return INGRS_VSL_REJC;

			return INGRS_SUCCED;
		}
	}
}