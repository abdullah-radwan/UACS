#include "VesselImpl.h"
#include "..\Common.h"

#include <filesystem>
#include <map>

DLLCLBK void AddCargo(UACS::API::Cargo* pCargo) { UACS::Core::cargoVector.push_back(pCargo); }

DLLCLBK void DeleteCargo(UACS::API::Cargo* pCargo) { std::erase(UACS::Core::cargoVector, pCargo); }

DLLCLBK UACS::Core::Vessel* CreateVessel(VESSEL* pVessel, UACS::API::VslAstrInfo* pVslAstrInfo, UACS::API::VslCargoInfo* pVslCargoInfo)
{ return new UACS::Core::VesselImpl(pVessel, pVslAstrInfo, pVslCargoInfo); }

namespace UACS
{
	namespace Core
	{
		std::vector<std::string> availCargoVector;

		void InitAvailableCargo()
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UACS/Cargoes"))
			{
				std::string path = entry.path().filename().string();

				availCargoVector.push_back(path.substr(0, path.find(".cfg")));
			}
		}

		VesselImpl::VesselImpl(VESSEL* pVessel, API::VslAstrInfo* pVslAstrInfo, API::VslCargoInfo* pVslCargoInfo) :
			pVessel(pVessel), pVslAstrInfo(pVslAstrInfo), pVslCargoInfo(pVslCargoInfo)
		{
			if (pVslAstrInfo) vslAstrMap.insert({ pVessel->GetHandle(), pVslAstrInfo });

			if (pVslCargoInfo && availCargoVector.empty()) InitAvailableCargo();
		}

		void VesselImpl::Destroy() noexcept { vslAstrMap.erase(pVessel); delete this; }

		bool VesselImpl::ParseScenarioLine(char* line)
		{
			std::istringstream ss;
			ss.str(line);

			std::string data;

			if (!(ss >> data >> std::ws) || !data._Starts_with("ASTR")) return false;

			if (data == "ASTR_STATION")
			{
				ss >> data;

				pLoadAstrInfo = &(pVslAstrInfo->stations.at(std::stoi(data)).astrInfo = API::AstrInfo()).value();

				return true;
			}

			if (data == "ASTR_NAME") { std::getline(ss, data); pLoadAstrInfo->name = data; return true; }

			if (data == "ASTR_ROLE") { ss >> data; pLoadAstrInfo->role = data; return true; }

			if (data == "ASTR_MASS") { ss >> data; pLoadAstrInfo->mass = std::stoi(data); return true; }

			if (data == "ASTR_OXYGEN") { ss >> data; pLoadAstrInfo->oxyLvl = std::stod(data); return true; }

			if (data == "ASTR_FUEL") { ss >> data; pLoadAstrInfo->fuelLvl = std::stod(data); return true; }

			if (data == "ASTR_ALIVE") { ss >> data; pLoadAstrInfo->alive = std::stod(data); return true; }

			if (data == "ASTR_CLASSNAME") { ss >> data; pLoadAstrInfo->className = data; return true; }

			if (data == "ASTR_CUSTOMDATA") { ss >> data; pLoadAstrInfo->customData = data; return true; }

			return false;
		}

		void VesselImpl::SaveState(FILEHANDLE scn)
		{
			for (size_t index{}; index < pVslAstrInfo->stations.size(); ++index)
			{
				const auto& astrInfo = pVslAstrInfo->stations.at(index).astrInfo;

				if (!astrInfo) continue;

				oapiWriteScenario_int(scn, "ASTR_STATION", index);

				oapiWriteScenario_string(scn, "ASTR_NAME", const_cast<char*>((*astrInfo).name.c_str()));

				oapiWriteScenario_string(scn, "ASTR_ROLE", const_cast<char*>((*astrInfo).role.c_str()));

				oapiWriteScenario_float(scn, "ASTR_MASS", (*astrInfo).mass);

				oapiWriteScenario_float(scn, "ASTR_OXYGEN", (*astrInfo).oxyLvl);

				oapiWriteScenario_float(scn, "ASTR_FUEL", (*astrInfo).fuelLvl);

				oapiWriteScenario_int(scn, "ASTR_ALIVE", (*astrInfo).alive);

				oapiWriteScenario_string(scn, "ASTR_CLASSNAME", const_cast<char*>((*astrInfo).className.c_str()));

				if (!(*astrInfo).customData.empty()) oapiWriteScenario_string(scn, "ASTR_CUSTOMDATA", const_cast<char*>((*astrInfo).customData.c_str()));
			}
		}

		const API::AstrInfo* VesselImpl::GetAstrInfo(OBJHANDLE hVessel)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hVessel](API::Astronaut* pAstr) { return pAstr->GetHandle() == hVessel; });

			return astrIt == astrVector.end() ? nullptr : (*astrIt)->clbkGetAstrInfo();
		}

		const API::VslAstrInfo* VesselImpl::GetVslAstrInfo(OBJHANDLE hVessel)
		{
			const auto& vslPair = vslAstrMap.find(hVessel);

			return vslPair == vslAstrMap.end() ? nullptr : vslPair->second;
		}

		API::TransferResult VesselImpl::TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx)
		{
			auto& astrInfo = pVslAstrInfo->stations.at(stationIdx).astrInfo;

			if (!astrInfo) return API::TRNS_STN_EMPTY;

			const auto& airlockInfo = pVslAstrInfo->airlocks.at(airlockIdx);

			if (!airlockInfo.open) return API::TRNS_ARLCK_CLSD;

			if (!airlockInfo.hDock) return API::TRNS_DOCK_UNDEF;

			OBJHANDLE hTarget = pVessel->GetDockStatus(airlockInfo.hDock);

			if (!hTarget) return API::TRNS_DOCK_EMPTY;

			const auto& vesselPair = vslAstrMap.find(hTarget);

			if (vesselPair == vslAstrMap.end()) return API::TRNS_TGT_ARLCK_UNDEF;

			API::VslAstrInfo* targetInfo = vesselPair->second;

			if (targetInfo->airlocks.empty()) return API::TRNS_TGT_ARLCK_UNDEF;

			if (targetInfo->stations.empty()) return API::TRNS_TGT_STN_UNDEF;

			const VESSEL* pTarget = oapiGetVesselInterface(hTarget);

			DOCKHANDLE hTargetDock;

			for (UINT index{}; index < pTarget->DockCount(); ++index)
			{
				DOCKHANDLE hDock = pTarget->GetDockHandle(index);

				if (pTarget->GetDockStatus(hDock) == pVessel->GetHandle()) { hTargetDock = hDock; break; }
			}

			for (const auto& targetAirlockInfo : targetInfo->airlocks)
			{
				if (targetAirlockInfo.hDock != hTargetDock) continue;

				if (!targetAirlockInfo.open) return API::TRNS_TGT_ARLCK_CLSD;

				if (!tgtStationIdx)
				{
					tgtStationIdx = GetEmptyStationIndex(targetInfo->stations);

					if (!tgtStationIdx) return API::TRNS_TGT_STN_OCCP;
				}

				else if (targetInfo->stations.at(tgtStationIdx.value()).astrInfo) return API::TRNS_TGT_STN_OCCP;

				targetInfo->stations.at(tgtStationIdx.value()).astrInfo = astrInfo;

				astrInfo = {};

				return API::TRNS_SUCCEDED;
			}

			return API::TRNS_TGT_ARLCK_UNDEF;
		}

		API::EgressResult VesselImpl::EgressAstronaut(size_t stationIdx, size_t airlockIdx)
		{
			auto& astrInfo = pVslAstrInfo->stations.at(stationIdx).astrInfo;

			if (!astrInfo) return API::EGRS_STN_EMPTY;

			const auto& airlockInfo = pVslAstrInfo->airlocks.at(airlockIdx);

			if (!airlockInfo.open) return API::EGRS_ARLCK_CLSD;

			if (airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)) return API::EGRS_ARLCK_DCKD;

			VESSELSTATUS2 status = GetVesselStatus(pVessel);

			if (pVessel->GetFlightStatus())
			{
				status.status = 1;

				VECTOR3 egressPos = airlockInfo.pos;
				if (!GetNearestAstrEmptyPos(egressPos)) return API::EGRS_NO_EMPTY_POS;

				pVessel->HorizonRot(egressPos, egressPos);

				SetGroundRotation(status, egressPos.z, egressPos.x, GetAstrHeight((*astrInfo).className));
			}

			else pVessel->Local2Rel(airlockInfo.pos, status.rpos);

			std::istringstream ss;
			ss.str((*astrInfo).name);

			std::string spawnName; ss >> spawnName; ss >> spawnName;
			spawnName.insert(0, "Astronaut");

			SetSpawnName(spawnName);

			std::string className = "UACS\\Astronauts\\";
			className += (*astrInfo).className;

			OBJHANDLE hAstr = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

			if (!hAstr) return API::EGRS_FAIL;

			API::Astronaut* pAstr = static_cast<API::Astronaut*>(oapiGetVesselInterface(hAstr));

			pAstr->clbkSetAstrInfo(*astrInfo);

			astrInfo = {};

			return API::EGRS_SUCCEDED;
		}

		std::vector<VECTOR3> VesselImpl::GetNearbyAstrs(const VECTOR3& airlockPos)
		{
			std::vector<VECTOR3> nearbyVector;

			for (const API::Astronaut* pAstr : astrVector)
			{
				if (!pAstr->GroundContact()) continue;

				VECTOR3 astrPos;
				pVessel->Local2Global(airlockPos, astrPos);
				pAstr->Global2Local(astrPos, astrPos);

				astrPos.x = -astrPos.x;
				astrPos.y = -astrPos.y;
				astrPos.z = -astrPos.z;

				// If the astronaut is within the release distance (1 meters) plus the column length (10 meters)
				// And the astronaut is lower than or equal to the row length
				if (astrPos.z <= 11 && astrPos.z >= 3.5 && astrPos.x <= 4) nearbyVector.push_back(astrPos);
			}

			return nearbyVector;
		}

		bool VesselImpl::GetNearestAstrEmptyPos(VECTOR3& initialPos)
		{
			std::vector<VECTOR3> nearbyVector = GetNearbyAstrs(initialPos);

			// Add the release distance
			initialPos.z += 1;

			double length{};

			VECTOR3 relPos = initialPos;

		loop:
			for (const VECTOR3& astrPos : nearbyVector)
			{
				// Orbiter SDK function length isn't used, as the elevetion (Y-axis) doesn't matter here
				VECTOR3 subtract = relPos - astrPos;

				// Proceed if the distance is lower than 1.5 meter
				if (sqrt(subtract.x * subtract.x + subtract.z * subtract.z) >= 0.5) continue;

				// Reset the position to the initial position
				relPos = initialPos;

				// Add 1.5m distance between cargoes
				length += 0.5;

				// If the distance will exceed the column length (which is 6 meters), add new row
				// Integer division here so only add if it exceeds (otherwise it won't increase)
				relPos.x += int(length / 6) * 0.5;

				// Don't ask me how I made this, it just works
				relPos.z += length - (int(length / 6) * 6.0);

				// Run the loop again, as the new positon could interfer with a previous cargo
				goto loop;
			}

			// If the availale position is too far
			if (relPos.x - initialPos.x > 4) return false;

			initialPos = relPos;

			return true;
		}

		double VesselImpl::GetAstrHeight(std::string_view className)
		{
			std::string configFile = std::format("Vessels\\UACS\\Astronauts\\{}.cfg", className);

			FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

			if (!hConfig) return 0;

			double height{};

			oapiReadItem_float(hConfig, "Height", height);

			oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

			return height;
		}

		size_t VesselImpl::GetScenarioCargoCount() { return cargoVector.size(); }

		API::CargoInfo VesselImpl::GetCargoInfoByIndex(size_t cargoIdx) { return SetCargoInfo(cargoVector.at(cargoIdx)); }

		std::optional<API::CargoInfo> VesselImpl::GetCargoInfoByHandle(OBJHANDLE hVessel)
		{
			auto cargoIt = std::ranges::find_if(cargoVector, [hVessel](API::Cargo* pCargo) { return pCargo->GetHandle() == hVessel; });

			return cargoIt == cargoVector.end() ? std::optional<API::CargoInfo>{} : SetCargoInfo(*cargoIt);
		}

		std::optional<API::CargoInfo> VesselImpl::GetCargoInfoBySlot(size_t slotIdx)
		{
			OBJHANDLE hCargo{ GetSlotCargoHandle(slotIdx) };

			if (!hCargo) return {};

			return SetCargoInfo(static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo)));
		}

		double VesselImpl::GetTotalCargoMass()
		{
			double totalCargoMass{};

			for (const auto& slotInfo : pVslCargoInfo->slots)
			{
				OBJHANDLE hCargo = pVessel->GetAttachmentStatus(slotInfo.hAttach);

				if (hCargo) totalCargoMass += oapiGetMass(hCargo);
			}

			return totalCargoMass;
		}

		size_t VesselImpl::GetAvailableCargoCount() { return availCargoVector.size(); }

		std::string_view VesselImpl::GetAvailableCargoName(size_t availIdx) { return availCargoVector.at(availIdx); }

		API::GrappleResult VesselImpl::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
		{
			if (slotIdx)
			{
				if (!pVslCargoInfo->slots.at(*slotIdx).open) return API::GRPL_SLT_CLSD;

				if (GetSlotCargoHandle(*slotIdx)) return API::GRPL_SLT_OCCP;
			}
			else
			{
				auto result = GetEmptySlot(true);

				if (!result) return API::GRPL_SLT_OCCP;

				if (!(*result).open) return API::GRPL_SLT_CLSD;

				slotIdx = (*result).slotIdx;
			}

			const std::string_view cargoName = availCargoVector.at(availIdx);

			std::string spawnName = "Cargo";
			spawnName += cargoName;
			SetSpawnName(spawnName);

			std::string className = "UACS\\Cargoes\\";
			className += cargoName;

			VESSELSTATUS2 status = GetVesselStatus(pVessel);

			OBJHANDLE hCargo = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

			if (!hCargo) return API::GRPL_FAIL;

			if (pVslCargoInfo->maxCargoMass && oapiGetMass(hCargo) > *pVslCargoInfo->maxCargoMass)
			{
				oapiDeleteVessel(hCargo);
				return API::GRPL_MAX_MASS_EXCD;
			}

			if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + oapiGetMass(hCargo) > *pVslCargoInfo->maxTotalCargoMass)
			{
				oapiDeleteVessel(hCargo);
				return API::GRPL_MAX_TTLMASS_EXCD;
			}

			auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

			if (!pVessel->AttachChild(hCargo, pVslCargoInfo->slots.at(*slotIdx).hAttach, pCargo->clbkGetCargoInfo()->hAttach))
			{
				oapiDeleteVessel(hCargo);
				return API::GRPL_FAIL;
			}

			return API::GRPL_SUCCED;
		}

		API::ReleaseResult VesselImpl::DeleteCargo(std::optional<size_t> slotIdx)
		{
			OBJHANDLE hCargo;

			if (slotIdx) hCargo = GetSlotCargoHandle(*slotIdx); 
			else
			{
				auto result = GetOccupiedSlot(false);

				hCargo = result ? (*result).hCargo : nullptr;
			}

			if (!hCargo) return API::RLES_SLT_EMPTY;

			if (!oapiDeleteVessel(hCargo)) return API::RLES_FAIL;

			return API::RLES_SUCCED;
		}

		API::GrappleResult VesselImpl::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
		{
			if (slotIdx)
			{
				if (!pVslCargoInfo->slots.at(*slotIdx).open) return API::GRPL_SLT_CLSD;

				if (GetSlotCargoHandle(*slotIdx)) return API::GRPL_SLT_OCCP;
			}
			else
			{
				auto result = GetEmptySlot(true);

				if (!result) return API::GRPL_SLT_OCCP;

				if (!(*result).open) return API::GRPL_SLT_CLSD;

				slotIdx = (*result).slotIdx;
			}

			VECTOR3 slotPos, slotDir, slotRot;
			pVessel->GetAttachmentParams(pVslCargoInfo->slots.at(*slotIdx).hAttach, slotPos, slotDir, slotRot);

			if (hCargo)
			{
				VECTOR3 vesselPos;
				pVessel->Local2Global(slotPos, vesselPos);
				oapiGlobalToLocal(hCargo, &vesselPos, &vesselPos);

				if (length(vesselPos) - oapiGetSize(hCargo) > pVslCargoInfo->grappleRange) return API::GRPL_NOT_IN_RNG;

				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) return API::GRPL_CRG_UNPCKD;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::GRPL_CRG_ATCHD;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) return API::GRPL_MAX_MASS_EXCD;

				if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + pCargo->GetMass() > *pVslCargoInfo->maxTotalCargoMass) return API::GRPL_MAX_TTLMASS_EXCD;

				if (!pVessel->AttachChild(hCargo, pVslCargoInfo->slots.at(*slotIdx).hAttach, cargoInfo->hAttach)) return API::GRPL_FAIL;

				pCargo->clbkCargoGrappled();

				return API::GRPL_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				VECTOR3 vesselPos;
				pVessel->Local2Global(slotPos, vesselPos);
				pCargo->Global2Local(vesselPos, vesselPos);

				const double distance = length(vesselPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->grappleRange) continue;

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) continue;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) continue;

				if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + pCargo->GetMass() > *pVslCargoInfo->maxTotalCargoMass) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::GRPL_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap)
			{
				if (pVessel->AttachChild(pCargo->GetHandle(), pVslCargoInfo->slots.at(*slotIdx).hAttach, pCargo->clbkGetCargoInfo()->hAttach))
				{
					pCargo->clbkCargoGrappled();
					return API::GRPL_SUCCED;
				}
			}

			return API::GRPL_FAIL;
		}

		API::ReleaseResult VesselImpl::ReleaseCargo(std::optional<size_t> slotIdx)
		{
			OBJHANDLE hCargo;

			if (slotIdx)
			{
				if (!pVslCargoInfo->slots.at(*slotIdx).open) return API::RLES_SLT_CLSD;

				hCargo = GetSlotCargoHandle(*slotIdx);
			}

			else
			{
				auto result = GetOccupiedSlot(true);

				if (!result) return API::RLES_SLT_EMPTY;

				if (!(*result).open) return API::RLES_SLT_CLSD;

				slotIdx = (*result).slotIdx;
				hCargo = (*result).hCargo;
			}

			if (!hCargo) return API::RLES_SLT_EMPTY;

			auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

			if (pVessel->GetFlightStatus())
			{
				VESSELSTATUS2 status = GetVesselStatus(pVessel);

				VECTOR3 relPos, slotDir, slotRot;
				pVessel->GetAttachmentParams(pVslCargoInfo->slots.at(*slotIdx).hAttach, relPos, slotDir, slotRot);

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				VECTOR3 attachPos;
				pCargo->GetAttachmentParams(cargoInfo->hAttach, attachPos, slotDir, slotRot);
				relPos += attachPos;

				if (!GetNearestCargoEmptyPos(relPos)) return API::RLES_NO_EMPTY_POS;

				pVessel->HorizonRot(relPos, relPos);

				if (cargoInfo->frontPos.z || cargoInfo->rightPos.x || cargoInfo->leftPos.x)
					SetGroundRotation(status, relPos.x, relPos.z, cargoInfo->frontPos, cargoInfo->rightPos, cargoInfo->leftPos);

				else SetGroundRotation(status, relPos.x, relPos.z, abs(cargoInfo->frontPos.y));

				if (!pVessel->DetachChild(pVslCargoInfo->slots.at(*slotIdx).hAttach)) return API::RLES_FAIL;

				pCargo->DefSetStateEx(&status);
			}
			else if (!pVessel->DetachChild(pVslCargoInfo->slots.at(*slotIdx).hAttach, pVslCargoInfo->relVel)) return API::RLES_FAIL;

			pCargo->clbkCargoReleased();

			return API::RLES_SUCCED;
		}

		API::PackResult VesselImpl::PackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				VECTOR3 cargoPos;
				pVessel->GetRelativePos(hCargo, cargoPos);

				if (length(cargoPos) - oapiGetSize(hCargo) > pVslCargoInfo->packRange) return API::PACK_NOT_IN_RNG;

				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked) return API::PACK_CRG_PCKD;

				if (cargoInfo->type != API::PACKABLE_UNPACKABLE) return API::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::PACK_CRG_ATCHD;

				if (!pCargo->clbkPackCargo()) return API::PACK_FAIL;

				return API::PACK_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				const double distance = length(cargoPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->packRange) continue;

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && cargoInfo->type == API::PACKABLE_UNPACKABLE && !pCargo->GetAttachmentStatus(cargoInfo->hAttach)) cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkPackCargo()) return API::PACK_SUCCED;

			return API::PACK_FAIL;
		}

		API::PackResult VesselImpl::UnpackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				VECTOR3 cargoPos;
				pVessel->GetRelativePos(hCargo, cargoPos);

				const double distance = length(cargoPos) - oapiGetSize(hCargo);

				if (distance > pVslCargoInfo->packRange) return API::PACK_NOT_IN_RNG;

				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked) return API::PACK_CRG_UNPCKD;

				if (cargoInfo->type != API::UNPACKABLE_ONLY && cargoInfo->type != API::PACKABLE_UNPACKABLE) return API::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::PACK_CRG_ATCHD;

				if (!pCargo->clbkUnpackCargo()) return API::PACK_FAIL;

				return API::PACK_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				const double distance = length(cargoPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->packRange) continue;

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked && (cargoInfo->type == API::UNPACKABLE_ONLY || cargoInfo->type == API::PACKABLE_UNPACKABLE) &&
					!pCargo->GetAttachmentStatus(cargoInfo->hAttach)) cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkUnpackCargo()) return API::PACK_SUCCED;

			return API::PACK_FAIL;
		}

		API::DrainInfo VesselImpl::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{
			API::DrainInfo drainInfo;
			drainInfo.result = API::DRIN_SLT_EMPTY;
			drainInfo.mass = {};

			OBJHANDLE hCargo;

			if (slotIdx)
			{
				hCargo = GetSlotCargoHandle(*slotIdx);

				if (!hCargo) return drainInfo;

			drainGrappledLabel:

				passRangeCheck = true;

				drainInfo = DrainUngrappledResource(resource, mass, hCargo);

				passRangeCheck = false;

				return drainInfo;
			}

			else
			{
				for (const auto& slotInfo : pVslCargoInfo->slots)
				{
					hCargo = pVessel->GetAttachmentStatus(slotInfo.hAttach);

					if (!hCargo) continue;

					goto drainGrappledLabel;
				}

				return drainInfo;
			}
		}

		API::DrainInfo VesselImpl::DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE hCargo)
		{
			API::DrainInfo drainInfo;
			drainInfo.result = API::DRIN_NOT_IN_RNG;
			drainInfo.mass = {};

			if (hCargo)
			{
				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				if (!passRangeCheck)
				{
					VECTOR3 cargoPos;
					pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

					if (length(cargoPos) - pCargo->GetSize() > pVslCargoInfo->drainRange) return drainInfo;
				}

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->resource.empty()) { drainInfo.result = API::DRIN_NOT_RES; return drainInfo; }

				if (cargoInfo->resource != resource) { drainInfo.result = API::DRIN_RES_NOMATCH; return drainInfo; }

				drainInfo.result = API::DRIN_SUCCED;
				drainInfo.mass = pCargo->clbkDrainResource(mass);
			}

			else
			{
				for (API::Cargo* pCargo : cargoVector)
				{
					auto cargoInfo = pCargo->clbkGetCargoInfo();

					if (cargoInfo->resource.empty() || cargoInfo->resource != resource) continue;

					VECTOR3 cargoPos;
					pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

					if (length(cargoPos) - pCargo->GetSize() > pVslCargoInfo->drainRange) continue;

					drainInfo.result = API::DRIN_SUCCED;
					drainInfo.mass = pCargo->clbkDrainResource(mass);

					break;
				}
			}

			return drainInfo;
		}

		API::DrainInfo VesselImpl::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
		{
			API::DrainInfo drainInfo;
			drainInfo.result = API::DRIN_NOT_IN_RNG;
			drainInfo.mass = {};

			VESSEL* pStation;
			DWORD vesselIdx{};

			if (hStation)
			{
				pStation = oapiGetVesselInterface(hStation);

				VECTOR3 stationPos;
				pVessel->GetRelativePos(pStation->GetHandle(), stationPos);

				if (length(stationPos) - pStation->GetSize() > pVslCargoInfo->drainRange) return drainInfo;

				goto drainStationLabel;
			}

			for (; vesselIdx < oapiGetVesselCount(); ++vesselIdx)
			{
				pStation = oapiGetVesselInterface(oapiGetVesselByIndex(vesselIdx));

				VECTOR3 stationPos;
				pVessel->GetRelativePos(pStation->GetHandle(), stationPos);

				if (length(stationPos) - pStation->GetSize() > pVslCargoInfo->drainRange) continue;

			drainStationLabel:

				for (DWORD attachIdx{}; attachIdx < pStation->AttachmentCount(false); ++attachIdx)
				{
					if (std::strcmp(pStation->GetAttachmentId(pStation->GetAttachmentHandle(false, attachIdx)), "UACS_ST")) continue;

					std::string configFile = std::format("Vessels/{}.cfg", pStation->GetClassNameA());

					FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

					if (!hConfig)
					{
						if (hStation) { drainInfo.result = API::DRIN_FAIL; return drainInfo; }

						break;
					}

					char buffer[256];

					if (!oapiReadItem_string(hConfig, "UACS_RESOURCES", buffer))
					{
						oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

						if (hStation) { drainInfo.result = API::DRIN_FAIL; return drainInfo; }

						break;
					}

					oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

					std::string stationResources = buffer;
					// Add a comma at the end to identify the latest resource by the letter check code
					stationResources.push_back(',');

					std::string stationResource;

					for (const auto& letter : stationResources)
					{
						// If it's a comma, the resource name should be completed
						if (letter == ',')
						{
							if (stationResource == resource)
							{
								drainInfo.result = API::DRIN_SUCCED;
								drainInfo.mass = mass;

								return drainInfo;
							}
							// Clear the resource to begin with a new resource
							stationResource.clear();
						}
						else stationResource += letter;
					}

					if (hStation) { drainInfo.result = API::DRIN_RES_NOMATCH; return drainInfo; }

					break;
				}
			}

			return drainInfo;
		}

		OBJHANDLE VesselImpl::GetNearestBreathable()
		{
			OBJHANDLE hCargo{};
			double nearestDistance = pVslCargoInfo->breathableRange;

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();
				if (!cargoInfo->breathable || !cargoInfo->unpacked) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				const double distance = length(cargoPos);

				if (distance >= nearestDistance) continue;

				hCargo = pCargo->GetHandle();
				nearestDistance = distance;
			}

			return hCargo;
		}

		bool VesselImpl::InBreathableArea()
		{
			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();
				if (!cargoInfo->breathable || !cargoInfo->unpacked) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				if (length(cargoPos) <= pCargo->GetSize()) return true;
			}

			return false;
		}

		API::CargoInfo VesselImpl::SetCargoInfo(API::Cargo* pCargo)
		{
			API::CargoInfo cargoInfo;

			auto cargoCargoInfo = pCargo->clbkGetCargoInfo();

			cargoInfo.handle = pCargo->GetHandle();
			cargoInfo.attached = pCargo->GetAttachmentStatus(cargoCargoInfo->hAttach);
			cargoInfo.name = pCargo->GetName();
			cargoInfo.mass = pCargo->GetMass();
			cargoInfo.type = cargoCargoInfo->type;
			cargoInfo.resource = cargoCargoInfo->resource;
			cargoInfo.unpacked = cargoCargoInfo->unpacked;
			cargoInfo.breathable = cargoCargoInfo->breathable;

			return cargoInfo;
		}

		OBJHANDLE VesselImpl::GetSlotCargoHandle(size_t slotIdx) { return pVessel->GetAttachmentStatus(pVslCargoInfo->slots.at(slotIdx).hAttach); }

		std::optional<VesselImpl::SlotResult> VesselImpl::GetEmptySlot(bool mustBeOpen)
		{
			bool open = true;

			for (size_t index{}; index < pVslCargoInfo->slots.size(); ++index)
			{
				open = pVslCargoInfo->slots.at(index).open;

				if (!open && mustBeOpen) continue;

				if (!GetSlotCargoHandle(index)) return { {index, open} };
			}

			return {};
		}

		std::optional<VesselImpl::SlotResult> VesselImpl::GetOccupiedSlot(bool mustBeOpen)
		{
			bool open = true;

			for (size_t index{}; index < pVslCargoInfo->slots.size(); ++index)
			{
				open = pVslCargoInfo->slots.at(index).open;

				if (!open && mustBeOpen) continue;

				OBJHANDLE handle = GetSlotCargoHandle(index);

				if (handle) return { {index, open, handle} };
			}

			return {};
		}

		std::vector<VECTOR3> VesselImpl::GetNearbyCargoes(const VECTOR3& slotPos)
		{
			std::vector<VECTOR3> nearbyVector;

			for (const API::Cargo* pCargo : cargoVector)
			{
				if (!pCargo->GroundContact()) continue;

				VECTOR3 cargoPos;
				pVessel->Local2Global(slotPos, cargoPos);
				pCargo->Global2Local(cargoPos, cargoPos);

				cargoPos.x = -cargoPos.x;
				cargoPos.y = -cargoPos.y;
				cargoPos.z = -cargoPos.z;

				// If the cargo is within the release distance (5 meters) plus the column length (6 meters)
				// And the cargo is lower than or equal to the row length
				if (cargoPos.x <= 11 && cargoPos.x >= 3.5 && cargoPos.z <= pVslCargoInfo->relRowCount) nearbyVector.push_back(cargoPos);
			}

			return nearbyVector;
		}

		bool VesselImpl::GetNearestCargoEmptyPos(VECTOR3& initialPos)
		{
			std::vector<VECTOR3> nearbyVector = GetNearbyCargoes(initialPos);

			// Add the release distance
			if (!pVslCargoInfo->astrMode) initialPos.x += 5;

			double length{};

			VECTOR3 relPos = initialPos;

		loop:
			for (const VECTOR3& cargoPos : nearbyVector)
			{
				// Orbiter SDK function length isn't used, as the elevetion (Y-axis) doesn't matter here
				VECTOR3 subtract = relPos - cargoPos;

				// Proceed if the distance is lower than 1.5 meter
				if (sqrt(subtract.x * subtract.x + subtract.z * subtract.z) >= 1.5) continue;
				else if (pVslCargoInfo->astrMode) return false;

				relPos = initialPos;

				// Add 1.5m distance between cargoes
				length += 1.5;

				// If the distance will exceed the column length (which is 6 meters), add new row
				// Integer division here so only add if it exceeds (otherwise it won't increase)
				relPos.z += int(length / 6) * 1.5;

				// Don't ask me how I made this, it just works
				relPos.x += length - (int(length / 6) * 6.0);

				// Run the loop again, as the new positon could interfer with a previous cargo
				goto loop;
			}

			// If the available position is too far
			if (relPos.z - initialPos.z > pVslCargoInfo->relRowCount) return false;

			initialPos = relPos;

			return true;
		}
	}
}