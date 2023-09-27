#include "Common.h"
#include "..\BaseCommon.h"

#include <filesystem>
#include <map>

DLLCLBK UACS::Core::Module* CreateModule(VESSEL* pVessel, UACS::VslAstrInfo* pVslAstrInfo, UACS::VslCargoInfo* pVslCargoInfo)
{ return new UACS::Core::Module(pVessel, pVslAstrInfo, pVslCargoInfo); }

namespace UACS
{
	namespace Core
	{
		void Module::InitAvailAstr()
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UACS/Astronauts"))
				availAstrVector.push_back(entry.path().stem().string());
		}

		void Module::InitAvailCargo()
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path().string() + "/Config/Vessels/UACS/Cargoes"))			
				availCargoVector.push_back(entry.path().stem().string());
		}

		Module::Module(VESSEL* pVessel, UACS::VslAstrInfo* pVslAstrInfo, UACS::VslCargoInfo* pVslCargoInfo) :
			pVessel(pVessel), pVslAstrInfo(pVslAstrInfo), pVslCargoInfo(pVslCargoInfo)
		{
			if (availAstrVector.empty()) InitAvailAstr();

			if (pVslAstrInfo) vslAstrMap.insert({ pVessel->GetHandle(), pVslAstrInfo });

			if (availCargoVector.empty()) InitAvailCargo();
		}

		void Module::Destroy() noexcept { vslAstrMap.erase(pVessel); delete this; }

		std::string_view Module::GetUACSVersion() { return Core::GetUACSVersion(); }

		bool Module::ParseScenarioLine(char* line)
		{
			if (!pVslAstrInfo) return false;

			std::istringstream ss(line);
			std::string data;

			if (!(ss >> data >> std::ws) || !data._Starts_with("ASTR")) return false;

			if (data == "ASTR_STATION")
			{
				ss >> data;

				pLoadAstrInfo = &(pVslAstrInfo->stations.at(std::stoi(data)).astrInfo = UACS::AstrInfo()).value();

				return true;
			}

			if (data == "ASTR_NAME") { std::getline(ss, data); pLoadAstrInfo->name = data; return true; }

			if (data == "ASTR_ROLE") { ss >> data; pLoadAstrInfo->role = data; return true; }

			if (data == "ASTR_MASS") { ss >> data; pLoadAstrInfo->mass = std::stod(data); return true; }

			if (data == "ASTR_OXYGEN") { ss >> data; pLoadAstrInfo->oxyLvl = std::stod(data); return true; }

			if (data == "ASTR_FUEL") { ss >> data; pLoadAstrInfo->fuelLvl = std::stod(data); return true; }

			if (data == "ASTR_ALIVE") { ss >> data; pLoadAstrInfo->alive = std::stod(data); return true; }

			if (data == "ASTR_CLASSNAME") { ss >> data; pLoadAstrInfo->className = data; return true; }

			if (data == "ASTR_CUSTOMDATA") { ss >> data; pLoadAstrInfo->customData = data; return true; }

			return false;
		}

		void Module::clbkPostCreation()
		{
			if (!pVslCargoInfo) return;

			for (auto& slotInfo : pVslCargoInfo->slots) 
			{
				OBJHANDLE hCargo = pVessel->GetAttachmentStatus(slotInfo.hAttach);
				if (!hCargo) continue;

				slotInfo.cargoInfo = GetCargoInfoByHandle(hCargo);
				SetAttachPos(true, slotInfo.cargoInfo->unpacked, slotInfo);
			}
		}

		void Module::clbkSaveState(FILEHANDLE scn)
		{
			if (!pVslAstrInfo) return;

			for (size_t idx{}; idx < pVslAstrInfo->stations.size(); ++idx)
			{
				auto& astrInfoOpt = pVslAstrInfo->stations.at(idx).astrInfo;
				if (!astrInfoOpt) continue;

				auto& astrInfo = *astrInfoOpt;

				oapiWriteScenario_int(scn, "ASTR_STATION", idx);

				oapiWriteScenario_string(scn, "ASTR_NAME", astrInfo.name.data());

				oapiWriteScenario_string(scn, "ASTR_ROLE", astrInfo.role.data());

				oapiWriteScenario_float(scn, "ASTR_MASS", astrInfo.mass);

				oapiWriteScenario_float(scn, "ASTR_OXYGEN", astrInfo.oxyLvl);

				oapiWriteScenario_float(scn, "ASTR_FUEL", astrInfo.fuelLvl);

				oapiWriteScenario_int(scn, "ASTR_ALIVE", astrInfo.alive);

				oapiWriteScenario_string(scn, "ASTR_CLASSNAME", astrInfo.className.data());

				if (!astrInfo.customData.empty()) oapiWriteScenario_string(scn, "ASTR_CUSTOMDATA", astrInfo.customData.data());
			}
		}

		size_t Module::GetScnAstrCount() { return astrVector.size(); }

		std::pair<OBJHANDLE, const UACS::AstrInfo*> Module::GetAstrInfoByIndex(size_t astrIdx) { return Core::GetAstrInfoByIndex(astrIdx); }

		const UACS::AstrInfo* Module::GetAstrInfoByHandle(OBJHANDLE hAstr) { return Core::GetAstrInfoByHandle(hAstr); }

		const UACS::VslAstrInfo* Module::GetVslAstrInfo(OBJHANDLE hVessel) { return Core::GetVslAstrInfo(hVessel); }

		bool Module::SetAstrInfoByIndex(size_t astrIdx, const UACS::AstrInfo& astrInfo)
		{ return astrVector.at(astrIdx)->clbkSetAstrInfo(astrInfo); }

		bool Module::SetAstrInfoByHandle(OBJHANDLE hAstr, const UACS::AstrInfo& astrInfo)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hAstr](UACS::Astronaut* pAstr) { return pAstr->GetHandle() == hAstr; });

			return (astrIt == astrVector.end()) ? false : (*astrIt)->clbkSetAstrInfo(astrInfo);
		}

		size_t Module::GetAvailAstrCount() { return availAstrVector.size(); }

		std::string_view Module::GetAvailAstrName(size_t availIdx) { return availAstrVector.at(availIdx); }

		double Module::GetTotalAstrMass()
		{
			double totalMass{};

			for (const auto& station : pVslAstrInfo->stations) if (station.astrInfo) totalMass += station.astrInfo->mass;

			return totalMass;
		}

		UACS::IngressResult Module::AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx, std::optional<UACS::AstrInfo> astrInfo)
		{
			if (!stationIdx) stationIdx = GetEmptyStationIndex(pVslAstrInfo->stations);

			if (pVslAstrInfo->stations.at(*stationIdx).astrInfo) return UACS::INGRS_STN_OCCP;

			const std::string_view astrName = availAstrVector.at(availIdx);

			std::string className = "UACS\\Astronauts\\";
			className += astrName;

			if (!astrInfo)
			{
				astrInfo = UACS::AstrInfo();

				std::string spawnName = "Astronaut";
				spawnName += astrName;
				SetSpawnName(spawnName);

				VESSELSTATUS2 status = GetVesselStatus(pVessel);

				OBJHANDLE hAstr = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

				if (!hAstr) return UACS::INGRS_FAIL;

				astrInfo = *static_cast<UACS::Astronaut*>(oapiGetVesselInterface(hAstr))->clbkGetAstrInfo();

				oapiDeleteVessel(hAstr);
			}

			astrInfo->className = className;

			pVslAstrInfo->stations.at(*stationIdx).astrInfo = astrInfo;

			return UACS::INGRS_SUCCED;
		}

		UACS::TransferResult Module::TransferAstronaut(std::optional<size_t> stationIdx, std::optional<size_t> airlockIdx, std::optional<size_t> tgtStationIdx)
		{
			if (!stationIdx) stationIdx = GetOccupiedStation();

			std::optional<UACS::AstrInfo>& astrInfo = pVslAstrInfo->stations.at(*stationIdx).astrInfo;

			if (!astrInfo) return UACS::TRNS_STN_EMPTY;

			if (!airlockIdx) airlockIdx = GetTransferAirlock();

			const UACS::AirlockInfo& airlockInfo = pVslAstrInfo->airlocks.at(*airlockIdx);

			if (!airlockInfo.open) return UACS::TRNS_ARLCK_CLSD;

			if (!airlockInfo.hDock) return UACS::TRNS_DOCK_UNDEF;

			OBJHANDLE hTarget = pVessel->GetDockStatus(airlockInfo.hDock);

			if (!hTarget) return UACS::TRNS_DOCK_EMPTY;

			const auto& vesselPair = vslAstrMap.find(hTarget);

			if (vesselPair == vslAstrMap.end()) return UACS::TRNS_TGT_ARLCK_UNDEF;

			UACS::VslAstrInfo* targetInfo = vesselPair->second;

			if (targetInfo->airlocks.empty()) return UACS::TRNS_TGT_ARLCK_UNDEF;

			if (targetInfo->stations.empty()) return UACS::TRNS_TGT_STN_UNDEF;

			VESSEL* pTarget = oapiGetVesselInterface(hTarget);

			DOCKHANDLE hTargetDock{};

			for (UINT idx{}; idx < pTarget->DockCount(); ++idx)
			{
				DOCKHANDLE hDock = pTarget->GetDockHandle(idx);

				if (pTarget->GetDockStatus(hDock) == pVessel->GetHandle()) { hTargetDock = hDock; break; }
			}

			for (const auto& targetAirlockInfo : targetInfo->airlocks)
			{
				if (targetAirlockInfo.hDock != hTargetDock) continue;

				if (!targetAirlockInfo.open) return UACS::TRNS_TGT_ARLCK_CLSD;

				if (!tgtStationIdx) tgtStationIdx = GetEmptyStationIndex(targetInfo->stations);

				if (targetInfo->stations.at(*tgtStationIdx).astrInfo) return UACS::TRNS_TGT_STN_OCCP;

				targetInfo->stations.at(*tgtStationIdx).astrInfo = astrInfo;

				if (pTarget->Version() >= 3)
				{
					if (!static_cast<VESSEL3*>(pTarget)->clbkGeneric(UACS::MSG, UACS::ASTR_INGRS, &(*tgtStationIdx)))
					{
						targetInfo->stations.at(*tgtStationIdx).astrInfo = {};
						return UACS::TRNS_VSL_REJC;
					}
				}

				astrInfo = {};

				return UACS::TRNS_SUCCED;
			}

			return UACS::TRNS_TGT_ARLCK_UNDEF;
		}

		UACS::EgressResult Module::EgressAstronaut(std::optional<size_t> stationIdx, std::optional<size_t> airlockIdx)
		{
			if (!stationIdx) stationIdx = GetOccupiedStation();

			std::optional<UACS::AstrInfo>& astrInfoOpt = pVslAstrInfo->stations.at(*stationIdx).astrInfo;

			if (!astrInfoOpt) return UACS::EGRS_STN_EMPTY;

			if (!airlockIdx) airlockIdx = GetEgressAirlock();

			const UACS::AirlockInfo& airlockInfo = pVslAstrInfo->airlocks.at(*airlockIdx);

			if (!airlockInfo.open) return UACS::EGRS_ARLCK_CLSD;

			if (airlockInfo.hDock && pVessel->GetDockStatus(airlockInfo.hDock)) return UACS::EGRS_ARLCK_DCKD;

			UACS::AstrInfo& astrInfo = *astrInfoOpt;

			VESSELSTATUS2 status = GetVesselStatus(pVessel);

			if (status.status)
			{
				VECTOR3 egressPos = airlockInfo.gndInfo.pos ? *airlockInfo.gndInfo.pos : airlockInfo.pos;
				egressPos.y = 0;
				if (!SetGroundPos<UACS::Astronaut>(status, egressPos, airlockInfo.gndInfo, astrVector)) return UACS::EGRS_NO_EMPTY_POS;

				SetGroundRotation(status, astrInfo.height, egressPos.x, egressPos.z);
			}

			else 
			{
				pVessel->Local2Rel(airlockInfo.pos, status.rpos);

				VECTOR3 xRight, yUp, zForward;

				// Convert to global frame
				pVessel->GlobalRot(airlockInfo.rot, xRight);
				pVessel->GlobalRot(airlockInfo.dir, zForward);

				// Get Y axis by taking cross product
				yUp = crossp(zForward, xRight); normalise(yUp);

				// Global vector to Euler angle conversion
				double alpha = atan2(zForward.y, zForward.z);
				double beta = -asin(zForward.x);
				double gamma = atan2(xRight.x, yUp.x);

				status.arot = { alpha, beta, PI05 - gamma };

				status.rvel += zForward * airlockInfo.relVel;
			}

			std::istringstream ss(astrInfo.name);
			std::string spawnName;
			while (std::getline(ss, spawnName, ' '));

			spawnName.insert(0, "Astronaut");
			SetSpawnName(spawnName);

			OBJHANDLE hAstr = oapiCreateVesselEx(spawnName.c_str(), astrInfo.className.c_str(), &status);

			if (!hAstr) return UACS::EGRS_FAIL;

			auto pAstr = static_cast<UACS::Astronaut*>(oapiGetVesselInterface(hAstr));

			bool infoSet = pAstr->clbkSetAstrInfo(astrInfo);

			if (pVessel->Version() >= 3) static_cast<VESSEL3*>(pVessel)->clbkGeneric(UACS::MSG, UACS::ASTR_EGRS, &stationIdx);

			astrInfoOpt = {};

			return infoSet ? UACS::EGRS_SUCCED : UACS::EGRS_INFO_NOT_SET;
		}

		size_t Module::GetScnCargoCount() { return cargoVector.size(); }

		UACS::CargoInfo Module::GetCargoInfoByIndex(size_t cargoIdx) { return SetCargoInfo(cargoVector.at(cargoIdx)); }

		std::optional<UACS::CargoInfo> Module::GetCargoInfoByHandle(OBJHANDLE hCargo)
		{
			auto cargoIt = std::ranges::find_if(cargoVector, [hCargo](UACS::Cargo* pCargo) { return pCargo->GetHandle() == hCargo; });

			return (cargoIt == cargoVector.end()) ? std::optional<UACS::CargoInfo>{} : SetCargoInfo(*cargoIt);
		}

		std::optional<std::vector<std::string>> Module::GetStationResources(OBJHANDLE hStation)
		{
			VESSEL* pStation = oapiGetVesselInterface(hStation);

			if (!passCheck)
			{
				int attachIdx = int(pStation->AttachmentCount(true)) - 1;

				if (attachIdx < 0) return std::nullopt;

				const char* attachLabel = pStation->GetAttachmentId(pStation->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_R") && std::strcmp(attachLabel, "UACS_RB"))) return std::nullopt;
			}

			std::string configFile = std::format("Vessels/{}.cfg", pStation->GetClassNameA());

			FILEHANDLE hConfig = oapiOpenFile(configFile.c_str(), FILE_IN_ZEROONFAIL, CONFIG);

			if (!hConfig) return std::vector<std::string>{};

			char buffer[256];

			if (!oapiReadItem_string(hConfig, "UACS_RESOURCES", buffer))
			{
				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
				return std::vector<std::string>{};
			}

			oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);

			std::vector<std::string> resources;

			std::istringstream ss(buffer);
			std::string resource;

			while (std::getline(ss, resource, ',')) resources.push_back(resource);

			return resources;
		}

		size_t Module::GetAvailCargoCount() { return availCargoVector.size(); }

		std::string_view Module::GetAvailCargoName(size_t availIdx) { return availCargoVector.at(availIdx); }

		double Module::GetTotalCargoMass()
		{
			double totalMass{};

			for (const auto& slotInfo : pVslCargoInfo->slots) if (slotInfo.cargoInfo) totalMass += oapiGetMass(slotInfo.cargoInfo->handle);

			return totalMass;
		}

		UACS::GrappleResult Module::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
		{
			if (!slotIdx) slotIdx = GetEmptySlot(true);

			UACS::SlotInfo& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

			if (!slotInfo.open) return UACS::GRPL_SLT_CLSD;

			if (slotInfo.cargoInfo) return UACS::GRPL_SLT_OCCP;

			const std::string_view cargoName = availCargoVector.at(availIdx);

			std::string spawnName = "Cargo";
			spawnName += cargoName;
			SetSpawnName(spawnName);

			std::string className = "UACS\\Cargoes\\";
			className += cargoName;

			VESSELSTATUS2 status = GetVesselStatus(pVessel);

			OBJHANDLE hCargo = oapiCreateVesselEx(spawnName.c_str(), className.c_str(), &status);

			if (!hCargo) return UACS::GRPL_FAIL;

			if (pVslCargoInfo->maxCargoMass && oapiGetMass(hCargo) > *pVslCargoInfo->maxCargoMass)
			{
				oapiDeleteVessel(hCargo);
				return UACS::GRPL_MASS_EXCD;
			}

			if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + oapiGetMass(hCargo) > *pVslCargoInfo->maxTotalCargoMass)
			{
				oapiDeleteVessel(hCargo);
				return UACS::GRPL_TTL_MASS_EXCD;
			}

			auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(hCargo));
			bool unpacked = pCargo->clbkGetCargoInfo()->unpacked;

			SetAttachPos(true, unpacked, slotInfo);

			if (!pVessel->AttachChild(hCargo, slotInfo.hAttach, pCargo->clbkGetCargoInfo()->hAttach))
			{
				SetAttachPos(false, unpacked, slotInfo);
				oapiDeleteVessel(hCargo);

				return UACS::GRPL_FAIL;
			}

			pCargo->clbkCargoGrappled();

			slotInfo.cargoInfo = SetCargoInfo(pCargo);

			return UACS::GRPL_SUCCED;
		}

		UACS::ReleaseResult Module::DeleteCargo(std::optional<size_t> slotIdx)
		{
			if (!slotIdx) slotIdx = GetOccupiedSlot(false);

			UACS::SlotInfo& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

			if (!slotInfo.cargoInfo) return UACS::RLES_SLT_EMPTY;

			const auto& cargoInfo = *slotInfo.cargoInfo;

			if (!oapiDeleteVessel(cargoInfo.handle)) return UACS::RLES_FAIL;

			SetAttachPos(false, cargoInfo.unpacked, slotInfo);

			slotInfo.cargoInfo = {};
			return UACS::RLES_SUCCED;
		}

		UACS::GrappleResult Module::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
		{
			if (!slotIdx) slotIdx = GetEmptySlot(true);

			UACS::SlotInfo& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

			if (!slotInfo.open) return UACS::GRPL_SLT_CLSD;

			if (slotInfo.cargoInfo) return UACS::GRPL_SLT_OCCP;			

			VECTOR3 slotPos, slotDir, slotRot;
			pVessel->GetAttachmentParams(slotInfo.hAttach, slotPos, slotDir, slotRot);

			if (hCargo)
			{
				auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) return UACS::GRPL_CRG_UNPCKD;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return UACS::GRPL_CRG_ATCHD;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) return UACS::GRPL_MASS_EXCD;

				if (pVslCargoInfo->maxTotalCargoMass && GetTotalCargoMass() + pCargo->GetMass() > *pVslCargoInfo->maxTotalCargoMass) return UACS::GRPL_TTL_MASS_EXCD;

				VECTOR3 cargoPos, attachDir, attachRot;
				pCargo->GetAttachmentParams(cargoInfo->hAttach, cargoPos, attachDir, attachRot);

				pCargo->Local2Global(cargoPos, cargoPos);
				pVessel->Global2Local(cargoPos, cargoPos);
				cargoPos -= slotPos;

				if (length(cargoPos) > pVslCargoInfo->grappleRange) return UACS::GRPL_NOT_IN_RNG;

				SetAttachPos(true, cargoInfo->unpacked, slotInfo);

				if (!pVessel->AttachChild(hCargo, slotInfo.hAttach, cargoInfo->hAttach))
				{
					SetAttachPos(false, cargoInfo->unpacked, slotInfo);
					return UACS::GRPL_FAIL;
				}

				pCargo->clbkCargoGrappled();
				slotInfo.cargoInfo = SetCargoInfo(pCargo);

				return UACS::GRPL_SUCCED;
			}

			std::map<double, UACS::Cargo*> cargoMap;

			for (UACS::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked && !pVslCargoInfo->astrMode) continue;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				if (pVslCargoInfo->maxCargoMass && pCargo->GetMass() > *pVslCargoInfo->maxCargoMass) continue;

				if (pVslCargoInfo->maxTotalCargoMass && (GetTotalCargoMass() + pCargo->GetMass()) > *pVslCargoInfo->maxTotalCargoMass) continue;

				VECTOR3 cargoPos, attachDir, attachRot;
				pCargo->GetAttachmentParams(cargoInfo->hAttach, cargoPos, attachDir, attachRot);

				pCargo->Local2Global(cargoPos, cargoPos);
				pVessel->Global2Local(cargoPos, cargoPos);
				cargoPos -= slotPos;

				const double distance = length(cargoPos);

				if (distance > pVslCargoInfo->grappleRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return UACS::GRPL_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				SetAttachPos(true, cargoInfo->unpacked, slotInfo);

				if (pVessel->AttachChild(pCargo->GetHandle(), slotInfo.hAttach, cargoInfo->hAttach))
				{
					pCargo->clbkCargoGrappled();
					slotInfo.cargoInfo = SetCargoInfo(pCargo);

					return UACS::GRPL_SUCCED;
				}

				SetAttachPos(false, cargoInfo->unpacked, slotInfo);
			}

			return UACS::GRPL_FAIL;
		}

		UACS::ReleaseResult Module::ReleaseCargo(std::optional<size_t> slotIdx)
		{
			if (!slotIdx) slotIdx = GetOccupiedSlot(true);

			UACS::SlotInfo& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

			if (!slotInfo.open) return UACS::RLES_SLT_CLSD;

			if (!slotInfo.cargoInfo) return UACS::RLES_SLT_EMPTY;

			auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(slotInfo.cargoInfo->handle));

			if (pVessel->GetFlightStatus())
			{
				VESSELSTATUS2 status = GetVesselStatus(pVessel);
				VECTOR3 relPos;

				if (slotInfo.gndInfo.pos) relPos = *slotInfo.gndInfo.pos;

				else
				{
					pCargo->GetGlobalPos(relPos);
					pVessel->Global2Local(relPos, relPos);
				}

				if (!SetGroundPos<UACS::Cargo>(status, relPos, slotInfo.gndInfo, cargoVector, pCargo)) return UACS::RLES_NO_EMPTY_POS;

				if (!pVessel->DetachChild(slotInfo.hAttach)) return UACS::RLES_FAIL;

				if (pVslCargoInfo->astrMode) status.surf_hdg = fmod(GetVesselStatus(pCargo).surf_hdg + PI, PI2);
				
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				SetGroundRotation(status, cargoInfo->frontPos, cargoInfo->rightPos, cargoInfo->leftPos, relPos.x, relPos.z);

				pCargo->DefSetStateEx(&status);
			}
			else if (!pVessel->DetachChild(slotInfo.hAttach, slotInfo.relVel)) return UACS::RLES_FAIL;

			SetAttachPos(false, slotInfo.cargoInfo->unpacked, slotInfo);

			pCargo->clbkCargoReleased();
			slotInfo.cargoInfo = {};

			return UACS::RLES_SUCCED;
		}

		UACS::PackResult Module::PackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked) return UACS::PACK_CRG_PCKD;

				if (cargoInfo->type != UACS::UNPACKABLE || cargoInfo->unpackOnly) return UACS::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return UACS::PACK_CRG_ATCHD;

				if (GetTgtVslDist(pCargo) > pVslCargoInfo->packRange) return UACS::PACK_NOT_IN_RNG;

				if (!pCargo->clbkPackCargo()) return UACS::PACK_FAIL;

				return UACS::PACK_SUCCED;
			}

			std::map<double, UACS::Cargo*> cargoMap;

			for (UACS::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->unpacked || cargoInfo->type != UACS::UNPACKABLE || cargoInfo->unpackOnly || pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				const double distance = GetTgtVslDist(pCargo);

				if (distance > pVslCargoInfo->packRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return UACS::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkPackCargo()) return UACS::PACK_SUCCED;

			return UACS::PACK_FAIL;
		}

		UACS::PackResult Module::UnpackCargo(OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked) return UACS::PACK_CRG_UNPCKD;

				if (cargoInfo->type != UACS::UNPACKABLE) return UACS::PACK_CRG_NOT_PCKABL;

				if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return UACS::PACK_CRG_ATCHD;

				if (GetTgtVslDist(pCargo) > pVslCargoInfo->packRange) return UACS::PACK_NOT_IN_RNG;

				if (!pCargo->clbkUnpackCargo()) return UACS::PACK_FAIL;

				return UACS::PACK_SUCCED;
			}

			std::map<double, UACS::Cargo*> cargoMap;

			for (UACS::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (cargoInfo->unpacked || pCargo->GetAttachmentStatus(cargoInfo->hAttach)) continue;

				if (cargoInfo->type != UACS::UNPACKABLE) continue;

				const double distance = GetTgtVslDist(pCargo);

				if (distance > pVslCargoInfo->packRange) continue;

				cargoMap[distance] = pCargo;
			}

			if (cargoMap.empty()) return UACS::PACK_NOT_IN_RNG;

			for (const auto& [distance, pCargo] : cargoMap) if (pCargo->clbkUnpackCargo()) return UACS::PACK_SUCCED;

			return UACS::PACK_FAIL;
		}

		std::pair<UACS::DrainResult, double> Module::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{
			if (slotIdx)
			{
				const auto& slotInfo = pVslCargoInfo->slots.at(*slotIdx);

				if (!slotInfo.cargoInfo) return { UACS::DRIN_SLT_EMPTY, 0 };

				passCheck = true;
				auto drainInfo = DrainScenarioResource(resource, mass, slotInfo.cargoInfo->handle);
				passCheck = false;

				return drainInfo;
			}

			for (const auto& slotInfo : pVslCargoInfo->slots)
			{
				if (!slotInfo.cargoInfo) continue;

				passCheck = true;
				auto drainInfo = DrainScenarioResource(resource, mass, slotInfo.cargoInfo->handle);
				passCheck = false;

				return drainInfo;
			}

			return { UACS::DRIN_SLT_EMPTY, 0 };
		}

		std::pair<UACS::DrainResult, double> Module::DrainScenarioResource(std::string_view resource, double mass, OBJHANDLE hCargo)
		{
			if (hCargo)
			{
				auto pCargo = static_cast<UACS::Cargo*>(oapiGetVesselInterface(hCargo));

				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->resource) return { UACS::DRIN_VSL_NOT_RES, 0 };

				if (cargoInfo->resource != resource) return { UACS::DRIN_RES_NOT_FND, 0 };

				if (!passCheck)
				{
					if (pCargo->GetAttachmentStatus(cargoInfo->hAttach)) return { UACS::DRIN_RES_ATCHD, 0 };

					if (GetTgtVslDist(pCargo) > pVslCargoInfo->drainRange) return { UACS::DRIN_NOT_IN_RNG, 0 };
				}

				if (double drainedMass = pCargo->clbkDrainResource(mass)) return { UACS::DRIN_SUCCED, drainedMass };
				
				return { UACS::DRIN_FAIL, 0 };
			}

			std::map<double, UACS::Cargo*> cargoMap;

			for (UACS::Cargo* pCargo : cargoVector)
			{
				auto cargoInfo = pCargo->clbkGetCargoInfo();

				if (!cargoInfo->resource || pCargo->GetAttachmentStatus(cargoInfo->hAttach) || cargoInfo->resource != resource) continue;

				const double distance = GetTgtVslDist(pCargo);

				if (distance > pVslCargoInfo->drainRange) continue;

				cargoMap[distance] = pCargo;
			}

			for (const auto& [distance, pCargo] : cargoMap)
				if (double drainedMass = pCargo->clbkDrainResource(mass)) return { UACS::DRIN_SUCCED, drainedMass };

			return { UACS::DRIN_NOT_IN_RNG, 0 };
		}

		std::pair<UACS::DrainResult, double> Module::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
		{
			if (hStation)
			{
				VESSEL* pStation = oapiGetVesselInterface(hStation);

				int attachIdx = int(pStation->AttachmentCount(true)) - 1;

				if (attachIdx < 0) return { UACS::DRIN_VSL_NOT_RES, 0 };

				const char* attachLabel = pStation->GetAttachmentId(pStation->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_R") && std::strcmp(attachLabel, "UACS_RB"))) return { UACS::DRIN_VSL_NOT_RES, 0 };

				if (GetTgtVslDist(pStation) > pVslCargoInfo->drainRange) return {UACS::DRIN_NOT_IN_RNG, 0};

				passCheck = true;
				auto resources = *GetStationResources(pStation->GetHandle());
				passCheck = false;

				if (resources.empty() || std::find(resources.begin(), resources.end(), resource) != resources.end()) return { UACS::DRIN_SUCCED, mass };

				return { UACS::DRIN_RES_NOT_FND, 0 };
			}

			for (size_t idx{}; idx < oapiGetVesselCount(); ++idx)
			{
				VESSEL* pStation = oapiGetVesselInterface(oapiGetVesselByIndex(idx));

				int attachIdx = int(pStation->AttachmentCount(true)) - 1;

				if (attachIdx < 0) continue;

				const char* attachLabel = pStation->GetAttachmentId(pStation->GetAttachmentHandle(true, attachIdx));

				if (!attachLabel || (std::strcmp(attachLabel, "UACS_R") && std::strcmp(attachLabel, "UACS_RB"))) continue;

				if (GetTgtVslDist(pStation) > pVslCargoInfo->drainRange) continue;

				passCheck = true;
				auto resources = *GetStationResources(pStation->GetHandle());
				passCheck = false;

				if (resources.empty() || std::ranges::find(resources, resource) != resources.end()) return { UACS::DRIN_SUCCED, mass };
			}

			return { UACS::DRIN_NOT_IN_RNG, 0 };
		}

		double Module::GetTgtVslDist(VESSEL* pTgtVsl) const
		{
			VECTOR3 tgtVslPos;
			pTgtVsl->GetRelativePos(pVessel->GetHandle(), tgtVslPos);

			return length(tgtVslPos) - pVessel->GetSize() - pTgtVsl->GetSize();
		}

		void Module::SetAttachPos(bool attach, bool unpacked, const UACS::SlotInfo& slotInfo)
		{
			if (unpacked) return;

			VECTOR3 slotPos, slotDir, slotRot;
			pVessel->GetAttachmentParams(slotInfo.hAttach, slotPos, slotDir, slotRot);

			if (attach) slotPos -= slotInfo.holdDir * 0.65;
			else slotPos += slotInfo.holdDir * 0.65;

			pVessel->SetAttachmentParams(slotInfo.hAttach, slotPos, slotDir, slotRot);
		}

		template<typename T>
		bool Module::SetGroundPos(const VESSELSTATUS2& vslStatus, VECTOR3& initPos, UACS::GroundInfo gndInfo, std::span<T*> objSpan, const T* pOrgObj)
		{
			VECTOR3 pos = initPos;

			if (!pVslCargoInfo->astrMode)
			{
				if (!gndInfo.colDir || !gndInfo.rowDir)
				{
					if (abs(initPos.z) > abs(initPos.x))
					{
						gndInfo.colDir = _V((initPos.x < 0 ? -1 : 1), 0, 0);
						gndInfo.rowDir = _V(0, 0, (initPos.z < 0 ? -1 : 1));
					}
					else
					{
						gndInfo.colDir = _V(0, 0, (initPos.z < 0 ? -1 : 1));
						gndInfo.rowDir = _V((initPos.x < 0 ? -1 : 1), 0, 0);
					}
				}
			}

			const double bodySize = oapiGetSize(vslStatus.rbody);

			VECTOR3 finalPos = initPos;

			size_t colCount{}, rowCount{};
			double spaceMargin = 0.5 * gndInfo.colSpace;

		groundPosLoop:
			for (const T* pObject : objSpan)
			{
				if (pObject == pOrgObj) continue;

				auto objStatus = GetVesselStatus(pObject);

				if (!objStatus.status || objStatus.rbody != vslStatus.rbody) continue;

				auto posCoords = Local2LngLat(bodySize, vslStatus.surf_lng, vslStatus.surf_lat, vslStatus.surf_hdg, finalPos);

				if (DistLngLat(bodySize, objStatus.surf_lng, objStatus.surf_lat, posCoords.first, posCoords.second) > spaceMargin) continue;

				else if (pVslCargoInfo->astrMode) return false;

				++colCount;

				if (colCount >= gndInfo.rowCount)
				{
					++rowCount;

					if (rowCount >= gndInfo.rowCount) return false;

					colCount = 0;

					finalPos = initPos;

					finalPos.x += gndInfo.rowSpace * rowCount * gndInfo.colDir->x;
					finalPos.z += gndInfo.rowSpace * rowCount * gndInfo.colDir->z;
				}

				else
				{
					finalPos.x += gndInfo.colSpace * gndInfo.rowDir->x;
					finalPos.z += gndInfo.colSpace * gndInfo.rowDir->z;
				}

				goto groundPosLoop;
			}

			auto posCoords = Local2LngLat(bodySize, vslStatus.surf_lng, vslStatus.surf_lat, vslStatus.surf_hdg, finalPos);

			initPos.x = posCoords.first - vslStatus.surf_lng;
			initPos.z = posCoords.second - vslStatus.surf_lat;

			return true;
		}

		std::pair<double, double> Module::Local2LngLat(double bodySize, double lng, double lat, double hdg, VECTOR3 pos)
		{
			double dist = length(pos) / bodySize;
			double finalHdg = fmod(atan2(pos.x, pos.z) + PI2 + hdg, PI2);

			double finalLat = asin(sin(lat) * cos(dist) + cos(lat) * sin(dist) * cos(finalHdg));
			double finalLng = atan2(sin(finalHdg) * sin(dist) * cos(lat), cos(dist) - sin(lat) * sin(finalLat)) + lng;

			return { finalLng, finalLat };
		}

		size_t Module::GetOccupiedStation()
		{
			for (size_t idx{}; idx < pVslAstrInfo->stations.size(); ++idx)			
				if (pVslAstrInfo->stations.at(idx).astrInfo) return idx;
			
			return 0;
		}

		size_t Module::GetTransferAirlock()
		{
			for (size_t idx{}; idx < pVslAstrInfo->airlocks.size(); ++idx)
			{
				const auto& airlock = pVslAstrInfo->airlocks.at(idx);

				if (!airlock.open || !airlock.hDock) continue;

				OBJHANDLE hTarget = pVessel->GetDockStatus(airlock.hDock);

				if (!hTarget) continue;

				const auto& vesselPair = vslAstrMap.find(hTarget);

				if (vesselPair == vslAstrMap.end()) continue;;

				UACS::VslAstrInfo* targetInfo = vesselPair->second;

				if (!targetInfo->airlocks.empty() && !targetInfo->stations.empty()) return idx;
			}

			return 0;
		}

		size_t Module::GetEgressAirlock()
		{
			for (size_t idx{}; idx < pVslAstrInfo->airlocks.size(); ++idx)
			{
				const auto& airlock = pVslAstrInfo->airlocks.at(idx);
				if (airlock.open && (!airlock.hDock || !pVessel->GetDockStatus(airlock.hDock))) return idx;
			}

			return 0;
		}

		UACS::CargoInfo Module::SetCargoInfo(UACS::Cargo* pCargo)
		{
			UACS::CargoInfo cargoInfo;

			auto cargoCargoInfo = pCargo->clbkGetCargoInfo();

			cargoInfo.handle = pCargo->GetHandle();
			cargoInfo.attached = pCargo->GetAttachmentStatus(cargoCargoInfo->hAttach);
			cargoInfo.type = cargoCargoInfo->type;
			cargoInfo.resource = cargoCargoInfo->resource;
			cargoInfo.unpacked = cargoCargoInfo->unpacked;
			cargoInfo.breathable = cargoCargoInfo->breathable;

			return cargoInfo;
		}

		size_t Module::GetEmptySlot(bool mustBeOpen)
		{
			for (size_t idx{}; idx < pVslCargoInfo->slots.size(); ++idx)
			{
				const auto& slotInfo = pVslCargoInfo->slots.at(idx);

				if (mustBeOpen && !slotInfo.open) continue;
				if (!slotInfo.cargoInfo) return idx;
			}

			return 0;
		}

		size_t Module::GetOccupiedSlot(bool mustBeOpen)
		{
			for (size_t idx{}; idx < pVslCargoInfo->slots.size(); ++idx)
			{
				const auto& slotInfo = pVslCargoInfo->slots.at(idx);

				if (mustBeOpen && !slotInfo.open) continue;
				if (slotInfo.cargoInfo) return idx;
			}

			return 0;
		}
	}
}