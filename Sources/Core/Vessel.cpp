#include "CoreCommon.h"
#include "..\Common.h"

#include <filesystem>
#include <map>

DLLCLBK void AddCargo(UACS::API::Cargo* pCargo) { UACS::Core::cargoVector.push_back(pCargo); }

DLLCLBK void DeleteCargo(UACS::API::Cargo* pCargo) { std::erase(UACS::Core::cargoVector, pCargo); }

DLLCLBK UACS::Core::Vessel* CreateVessel(VESSEL* pVessel, UACS::API::VslAstrInfo* pVslAstrInfo, UACS::API::VslCargoInfo* pVslCargoInfo)
{ return new UACS::Core::Vessel(pVessel, pVslAstrInfo, pVslCargoInfo); }

namespace UACS
{
	namespace Core
	{
		void Vessel::InitAvailAstr()
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UACS/Astronauts"))
			{
				std::string path = entry.path().filename().string();

				availAstrVector.push_back(path.substr(0, path.find(".cfg")));
			}
		}

		void Vessel::InitAvailCargo()
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UACS/Cargoes"))
			{
				std::string path = entry.path().filename().string();

				availCargoVector.push_back(path.substr(0, path.find(".cfg")));
			}
		}

		Vessel::Vessel(VESSEL* pVessel, API::VslAstrInfo* pVslAstrInfo, API::VslCargoInfo* pVslCargoInfo) :
			pVessel(pVessel), pVslAstrInfo(pVslAstrInfo), pVslCargoInfo(pVslCargoInfo)
		{
			if (pVslAstrInfo)
			{ 
				if (availAstrVector.empty()) InitAvailAstr();

				vslAstrMap.insert({ pVessel->GetHandle(), pVslAstrInfo });
			}

			if (pVslCargoInfo && availCargoVector.empty()) InitAvailCargo();
		}

		void Vessel::Destroy() noexcept { vslAstrMap.erase(pVessel); delete this; }

		std::string_view Vessel::GetUACSVersion() { return Core::GetUACSVersion(); }

		bool Vessel::ParseScenarioLine(char* line)
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

		void Vessel::clbkPostCreation()
		{
			for (auto& slotInfo : pVslCargoInfo->slots) slotInfo.cargoInfo = GetCargoInfoByHandle(pVessel->GetAttachmentStatus(slotInfo.hAttach));
		}

		void Vessel::SaveState(FILEHANDLE scn)
		{
			for (size_t idx{}; idx < pVslAstrInfo->stations.size(); ++idx)
			{
				auto& astrInfo = pVslAstrInfo->stations.at(idx).astrInfo;

				if (!astrInfo) continue;

				oapiWriteScenario_int(scn, "ASTR_STATION", idx);

				oapiWriteScenario_string(scn, "ASTR_NAME", (*astrInfo).name.data());

				oapiWriteScenario_string(scn, "ASTR_ROLE", (*astrInfo).role.data());

				oapiWriteScenario_float(scn, "ASTR_MASS", (*astrInfo).mass);

				oapiWriteScenario_float(scn, "ASTR_OXYGEN", (*astrInfo).oxyLvl);

				oapiWriteScenario_float(scn, "ASTR_FUEL", (*astrInfo).fuelLvl);

				oapiWriteScenario_int(scn, "ASTR_ALIVE", (*astrInfo).alive);

				oapiWriteScenario_string(scn, "ASTR_CLASSNAME", (*astrInfo).className.data());

				if (!(*astrInfo).customData.empty()) oapiWriteScenario_string(scn, "ASTR_CUSTOMDATA", (*astrInfo).customData.data());
			}
		}

		size_t Vessel::GetScnAstrCount() { return Core::GetScnAstrCount(); }

		std::pair<OBJHANDLE, const API::AstrInfo*> Vessel::GetAstrInfoByIndex(size_t astrIdx) { return Core::GetAstrInfoByIndex(astrIdx); }

		const API::AstrInfo* Vessel::GetAstrInfoByHandle(OBJHANDLE hAstr) { return Core::GetAstrInfoByHandle(hAstr); }

		const API::VslAstrInfo* Vessel::GetVslAstrInfo(OBJHANDLE hVessel) { return Core::GetVslAstrInfo(hVessel); }

		void Vessel::SetScnAstrInfoByIndex(size_t astrIdx, API::AstrInfo astrInfo) { Core::SetScnAstrInfoByIndex(astrIdx, astrInfo); }

		bool Vessel::SetScnAstrInfoByHandle(OBJHANDLE hAstr, API::AstrInfo astrInfo) { return Core::SetScnAstrInfoByHandle(hAstr, astrInfo); }

		size_t Vessel::GetAvailAstrCount() { return availAstrVector.size(); }

		std::string_view Vessel::GetAvailAstrName(size_t availIdx) { return availAstrVector.at(availIdx); }

		API::IngressResult Vessel::AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx, std::optional<API::AstrInfo> astrInfo)
		{
			if (!stationIdx) stationIdx = GetEmptyStationIndex(pVslAstrInfo->stations);

			if (!stationIdx || pVslAstrInfo->stations.at(*stationIdx).astrInfo) return API::INGRS_STN_OCCP;

			if (!astrInfo)
			{
				astrInfo = API::AstrInfo();

				const std::string_view className = availAstrVector.at(availIdx);

				std::string configFile = std::format("Vessels\\UACS\\Astronauts\\{}.cfg", className);

				FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

				if (!hConfig) return API::INGRS_FAIL;

				char cBuffer[512];

				if (!oapiReadItem_string(hConfig, "Name", cBuffer)) WarnAndTerminate("name", className.data(), "astronaut");
				(*astrInfo).name = cBuffer;

				if (!oapiReadItem_string(hConfig, "Role", cBuffer)) WarnAndTerminate("role", className.data(), "astronaut");
				(*astrInfo).role = cBuffer;

				if (!oapiReadItem_float(hConfig, "Mass", (*astrInfo).mass)) WarnAndTerminate("mass", className.data(), "astronaut");

				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
			}

			(*astrInfo).className = availAstrVector.at(availIdx);

			pVslAstrInfo->stations.at(*stationIdx).astrInfo = astrInfo;

			return API::INGRS_SUCCED;
		}

		API::TransferResult Vessel::TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx)
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

			for (UINT idx{}; idx < pTarget->DockCount(); ++idx)
			{
				DOCKHANDLE hDock = pTarget->GetDockHandle(idx);

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

		API::EgressResult Vessel::EgressAstronaut(size_t stationIdx, size_t airlockIdx)
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

		std::vector<VECTOR3> Vessel::GetNearbyAstrs(const VECTOR3& airlockPos)
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

		bool Vessel::GetNearestAstrEmptyPos(VECTOR3& initialPos)
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

		double Vessel::GetAstrHeight(std::string_view className)
		{
			std::string configFile = std::format("Vessels\\UACS\\Astronauts\\{}.cfg", className);

			FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

			if (!hConfig) return 0;

			double height{};

			oapiReadItem_float(hConfig, "Height", height);

			oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

			return height;
		}

		size_t Vessel::GetScnCargoCount() { return cargoVector.size(); }

		API::CargoInfo Vessel::GetCargoInfoByIndex(size_t cargoIdx) { return SetCargoInfo(cargoVector.at(cargoIdx)); }

		std::optional<API::CargoInfo> Vessel::GetCargoInfoByHandle(OBJHANDLE hCargo)
		{
			auto cargoIt = std::ranges::find_if(cargoVector, [hCargo](API::Cargo* pCargo) { return pCargo->GetHandle() == hCargo; });

			return (cargoIt == cargoVector.end()) ? std::optional<API::CargoInfo>{} : SetCargoInfo(*cargoIt);
		}

		double Vessel::GetTotalCargoMass()
		{
			double totalCargoMass{};

			for (const auto& slotInfo : pVslCargoInfo->slots) totalCargoMass += oapiGetMass((*slotInfo.cargoInfo).handle);

			return totalCargoMass;
		}

		size_t Vessel::GetAvailCargoCount() { return availCargoVector.size(); }

		std::string_view Vessel::GetAvailCargoName(size_t availIdx) { return availCargoVector.at(availIdx); }

		API::GrappleResult Vessel::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
		{
			API::SlotInfo& slotInfo = slotIdx ? pVslCargoInfo->slots.at(*slotIdx) : GetEmptySlot(true);

			if (!slotInfo.open) return API::GRPL_SLT_CLSD;

			if (slotInfo.cargoInfo) return API::GRPL_SLT_OCCP;

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

			if (!pVessel->AttachChild(hCargo, slotInfo.hAttach, pCargo->clbkGetCargoInfo()->hAttach))
			{
				oapiDeleteVessel(hCargo);
				return API::GRPL_FAIL;
			}

			slotInfo.cargoInfo = SetCargoInfo(pCargo);

			return API::GRPL_SUCCED;
		}

		API::ReleaseResult Vessel::DeleteCargo(std::optional<size_t> slotIdx)
		{
			API::SlotInfo& slotInfo = slotIdx ? pVslCargoInfo->slots.at(*slotIdx) : GetOccupiedSlot(false);

			if (!slotInfo.cargoInfo) return API::RLES_SLT_EMPTY;

			if (!oapiDeleteVessel((*slotInfo.cargoInfo).handle)) return API::RLES_FAIL;

			slotInfo.cargoInfo = {};
			return API::RLES_SUCCED;
		}

		API::GrappleResult Vessel::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
		{
			API::SlotInfo& slotInfo = slotIdx ? pVslCargoInfo->slots.at(*slotIdx) : GetEmptySlot(true);

			if (!slotInfo.open) return API::GRPL_SLT_CLSD;

			if (slotInfo.cargoInfo) return API::GRPL_SLT_OCCP;			

			VECTOR3 slotPos, slotDir, slotRot;
			pVessel->GetAttachmentParams(slotInfo.hAttach, slotPos, slotDir, slotRot);

			if (hCargo)
			{
				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) return API::GRPL_CRG_UNPCKD;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::GRPL_CRG_ATCHD;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) return API::GRPL_MAX_MASS_EXCD;

				if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + pCargo->GetMass() > *pVslCargoInfo->maxTotalCargoMass) return API::GRPL_MAX_TTLMASS_EXCD;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(hCargo, cargoPos);
				cargoPos -= slotPos;

				if ((length(cargoPos) - oapiGetSize(hCargo)) > pVslCargoInfo->grappleRange) return API::GRPL_NOT_IN_RNG;

				if (!pVessel->AttachChild(hCargo, slotInfo.hAttach, cargoInfo->hAttach)) return API::GRPL_FAIL;

				pCargo->clbkCargoGrappled();
				slotInfo.cargoInfo = SetCargoInfo(pCargo);

				return API::GRPL_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) continue;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) continue;

				if (pVslCargoInfo->maxTotalCargoMass && (GetTotalCargoMass() + pCargo->GetMass()) > *pVslCargoInfo->maxTotalCargoMass) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);
				cargoPos -= slotPos;

				const double distance = length(cargoPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->grappleRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::GRPL_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap)
			{
				if (pVessel->AttachChild(pCargo->GetHandle(), slotInfo.hAttach, pCargo->clbkGetCargoInfo()->hAttach))
				{
					pCargo->clbkCargoGrappled();
					slotInfo.cargoInfo = SetCargoInfo(pCargo);

					return API::GRPL_SUCCED;
				}
			}

			return API::GRPL_FAIL;
		}

		API::ReleaseResult Vessel::ReleaseCargo(std::optional<size_t> slotIdx)
		{
			API::SlotInfo& slotInfo = slotIdx ? pVslCargoInfo->slots.at(*slotIdx) : GetOccupiedSlot(true);

			if (!slotInfo.open) return API::RLES_SLT_CLSD;

			if (!slotInfo.cargoInfo) return API::RLES_SLT_EMPTY;			

			auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface((*slotInfo.cargoInfo).handle));

			if (pVessel->GetFlightStatus())
			{
				VESSELSTATUS2 status = GetVesselStatus(pVessel);

				VECTOR3 relPos, slotDir, slotRot;
				pVessel->GetAttachmentParams(slotInfo.hAttach, relPos, slotDir, slotRot);

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				VECTOR3 attachPos;
				pCargo->GetAttachmentParams(cargoInfo->hAttach, attachPos, slotDir, slotRot);
				relPos += attachPos;

				if (!GetNearestCargoEmptyPos(relPos)) return API::RLES_NO_EMPTY_POS;

				pVessel->HorizonRot(relPos, relPos);

				if (cargoInfo->frontPos.z || cargoInfo->rightPos.x || cargoInfo->leftPos.x)
					SetGroundRotation(status, relPos.x, relPos.z, cargoInfo->frontPos, cargoInfo->rightPos, cargoInfo->leftPos);

				else SetGroundRotation(status, relPos.x, relPos.z, abs(cargoInfo->frontPos.y));

				if (!pVessel->DetachChild(slotInfo.hAttach)) return API::RLES_FAIL;

				pCargo->DefSetStateEx(&status);
			}
			else if (!pVessel->DetachChild(slotInfo.hAttach, pVslCargoInfo->relVel)) return API::RLES_FAIL;

			pCargo->clbkCargoReleased();
			slotInfo.cargoInfo = {};

			return API::RLES_SUCCED;
		}

		API::PackResult Vessel::PackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked) return API::PACK_CRG_PCKD;

				if (cargoInfo->type != API::PACK_UNPACK) return API::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::PACK_CRG_ATCHD;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(hCargo, cargoPos);

				if ((length(cargoPos) - oapiGetSize(hCargo)) > pVslCargoInfo->packRange) return API::PACK_NOT_IN_RNG;

				if (!pCargo->clbkPackCargo()) return API::PACK_FAIL;

				return API::PACK_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked || cargoInfo->type != API::PACK_UNPACK || pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				const double distance = length(cargoPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->packRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkPackCargo()) return API::PACK_SUCCED;

			return API::PACK_FAIL;
		}

		API::PackResult Vessel::UnpackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked) return API::PACK_CRG_UNPCKD;

				if (cargoInfo->type != API::UNPACK_ONLY && cargoInfo->type != API::PACK_UNPACK) return API::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return API::PACK_CRG_ATCHD;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(hCargo, cargoPos);

				const double distance = length(cargoPos) - oapiGetSize(hCargo);

				if (distance > pVslCargoInfo->packRange) return API::PACK_NOT_IN_RNG;

				if (!pCargo->clbkUnpackCargo()) return API::PACK_FAIL;

				return API::PACK_SUCCED;
			}

			std::map<double, API::Cargo*> cargoMap;

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked || pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				if (cargoInfo->type != API::UNPACK_ONLY && cargoInfo->type != API::PACK_UNPACK) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				const double distance = length(cargoPos) - pCargo->GetSize();

				if (distance > pVslCargoInfo->packRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return API::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkUnpackCargo()) return API::PACK_SUCCED;

			return API::PACK_FAIL;
		}

		std::pair<API::DrainResult, double> Vessel::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{
			if (slotIdx)
			{
				const auto& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

				if (!slotInfo.cargoInfo) return { API::DRIN_SLT_EMPTY, 0 };

				passCheck = true;
				auto drainInfo = DrainUngrappledResource(resource, mass, (*slotInfo.cargoInfo).handle);
				passCheck = false;

				return drainInfo;
			}

			for (const auto& slotInfo : pVslCargoInfo->slots)
			{
				if (!slotInfo.cargoInfo) continue;

				passCheck = true;
				auto drainInfo = DrainUngrappledResource(resource, mass, (*slotInfo.cargoInfo).handle);
				passCheck = false;

				return drainInfo;
			}

			return { API::DRIN_SLT_EMPTY, 0 };
		}

		std::pair<API::DrainResult, double> Vessel::DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<API::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->resource || cargoInfo->resource != resource) return { API::DRIN_RES_NOMATCH, 0 };

				if (!passCheck)
				{
					VECTOR3 cargoPos;
					pVessel->GetRelativePos(hCargo, cargoPos);

					if ((length(cargoPos) - pCargo->GetSize()) > pVslCargoInfo->drainRange) return { API::DRIN_NOT_IN_RNG, 0 };
				}

				return { API::DRIN_SUCCED, pCargo->clbkDrainResource(mass) };
			}

			for (API::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->resource || cargoInfo->resource != resource) continue;

				VECTOR3 cargoPos;
				pVessel->GetRelativePos(pCargo->GetHandle(), cargoPos);

				if ((length(cargoPos) - pCargo->GetSize()) > pVslCargoInfo->drainRange) continue;

				return { API::DRIN_SUCCED, pCargo->clbkDrainResource(mass) };
			}

			return { API::DRIN_NOT_IN_RNG, 0 };
		}

		std::pair<API::DrainResult, double> Vessel::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
		{
			if (hStation)
			{
				VESSEL* pStation = oapiGetVesselInterface(hStation);

				int attachIdx = int(pStation->AttachmentCount(true)) - 1;

				if (attachIdx < 0) return { API::DRIN_RES_NOMATCH, 0 };

				const char* attachLabel = pStation->GetAttachmentId(pStation->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_S") && std::strcmp(attachLabel, "UACS_BS"))) return { API::DRIN_RES_NOMATCH, 0 };

				VECTOR3 stationPos;
				pVessel->GetRelativePos(pStation->GetHandle(), stationPos);

				if ((length(stationPos) - pStation->GetSize()) > pVslCargoInfo->drainRange) return { API::DRIN_NOT_IN_RNG, 0 };

				if (StationHasResource(pStation, resource)) return { API::DRIN_SUCCED, mass };

				return { API::DRIN_RES_NOMATCH, 0 };
			}

			for (size_t idx{}; idx < oapiGetVesselCount(); ++idx)
			{
				VESSEL* pStation = oapiGetVesselInterface(oapiGetVesselByIndex(idx));

				int attachIdx = int(pStation->AttachmentCount(true)) - 1;

				if (attachIdx < 0) continue;

				const char* attachLabel = pStation->GetAttachmentId(pStation->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_S") && std::strcmp(attachLabel, "UACS_BS"))) continue;

				VECTOR3 stationPos;
				pVessel->GetRelativePos(pStation->GetHandle(), stationPos);

				if ((length(stationPos) - pStation->GetSize()) > pVslCargoInfo->drainRange) continue;

				if (StationHasResource(pStation, resource)) return { API::DRIN_SUCCED, mass };
			}

			return { API::DRIN_NOT_IN_RNG, 0 };
		}

		API::CargoInfo Vessel::SetCargoInfo(API::Cargo* pCargo)
		{
			API::CargoInfo cargoInfo;

			auto cargoCargoInfo = pCargo->clbkGetCargoInfo();

			cargoInfo.handle = pCargo->GetHandle();
			cargoInfo.attached = pCargo->GetAttachmentStatus(cargoCargoInfo->hAttach);
			cargoInfo.type = cargoCargoInfo->type;
			cargoInfo.resource = cargoCargoInfo->resource;
			cargoInfo.unpacked = cargoCargoInfo->unpacked;
			cargoInfo.breathable = cargoCargoInfo->breathable;

			return cargoInfo;
		}

		bool Vessel::StationHasResource(VESSEL* pStation, std::string_view resource)
		{
			std::string configFile = std::format("Vessels/{}.cfg", pStation->GetClassNameA());

			FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

			if (!hConfig) return false;

			char buffer[256];

			if (!oapiReadItem_string(hConfig, "UACS_RESOURCES", buffer))
			{
				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
				return false;
			}

			oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

			std::string stationResources = buffer;
			// Add a comma at the end to identify the latest resource by the letter check code
			stationResources.push_back(',');

			std::string stationResource;

			for (const auto& letter : stationResources)
			{
				// If it's a comma, the resource name should be complete
				if (letter == ',')
				{
					if (stationResource == resource) return true;

					// Clear the resource to begin with a new resource
					stationResource.clear();
				}
				else stationResource += letter;
			}

			return false;
		}

		API::SlotInfo& Vessel::GetEmptySlot(bool mustBeOpen)
		{
			for (auto& slotInfo : pVslCargoInfo->slots)
			{
				if (mustBeOpen && !slotInfo.open) continue;

				if (!slotInfo.cargoInfo) return slotInfo;
			}

			return pVslCargoInfo->slots.front();
		}

		API::SlotInfo& Vessel::GetOccupiedSlot(bool mustBeOpen)
		{
			for (auto& slotInfo : pVslCargoInfo->slots)
			{
				if (mustBeOpen && !slotInfo.open) continue;

				if (slotInfo.cargoInfo) return slotInfo;
			}

			return pVslCargoInfo->slots.front();
		}

		std::vector<VECTOR3> Vessel::GetNearbyCargoes(const VECTOR3& slotPos)
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

		bool Vessel::GetNearestCargoEmptyPos(VECTOR3& initialPos)
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