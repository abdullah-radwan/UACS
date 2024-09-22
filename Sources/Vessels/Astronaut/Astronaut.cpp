#include "Astronaut.h"
#include "..\..\BaseCommon.h"

#include <format>
#include <sstream>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hVessel, int fModel) { return new UACS::Vessel::Astronaut(hVessel, fModel); }

DLLCLBK void ovcExit(VESSEL* pVessel) { if (pVessel) delete static_cast<UACS::Vessel::Astronaut*>(pVessel); }

namespace UACS
{
	namespace Vessel
	{
		void Astronaut::LoadConfig()
		{
			configLoaded = true;

			FILEHANDLE hConfig = oapiOpenFile("UACS.cfg", FILE_IN_ZEROONFAIL, CONFIG);

			if (hConfig)
			{
				if (!oapiReadItem_bool(hConfig, "EnableCockpit", enableCockpit))
					oapiWriteLog("UACS warning: Couldn't read EnableCockpit option from config file, will use default value (TRUE)");

				if (!oapiReadItem_bool(hConfig, "ShowMeshInCockpit", showMeshInCockpit))
					oapiWriteLog("UACS warning: Couldn't read ShowMeshInCockpit option from config file, will use default value (TRUE)");

				if (!oapiReadItem_bool(hConfig, "EnhancedMovements", enhancedMovements))
					oapiWriteLog("UACS warning: Couldn't read EnhancedMovements option from config file, will use default value (TRUE)");

				if (oapiReadItem_float(hConfig, "SearchRange", searchRange)) searchRange *= 1000;
				else oapiWriteLog("UACS warning: Couldn't read SearchRange option from config file, will use default value (60)");

				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
			}
			else oapiWriteLog("UACS warning: Couldn't load config file, will use default config");
		}

		Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : UACS::Astronaut(hVessel, fModel), mdlAPI(this, nullptr, &vslCargoInfo)
		{
			if (!configLoaded) LoadConfig();

			astrInfo.fuelLvl = astrInfo.oxyLvl = 1;
			astrInfo.alive = true;
			astrInfo.className = GetClassNameA();

			lonSpeed.maxLimit = 10;
			lonSpeed.minLimit = latSpeed.minLimit = -(latSpeed.maxLimit = 1);
			steerAngle.minLimit = -(steerAngle.maxLimit = 45 * RAD);

			hudInfo.deadFont = oapiCreateFont(50, true, "Courier New");

			vslCargoInfo.astrMode = true;
			vslCargoInfo.grappleRange = 5;
			vslCargoInfo.packRange = 5;
			vslCargoInfo.drainRange = 5;

			SetDefaultValues();

			hudInfo.message = std::format("UACS version: v{}", GetUACSVersion());
		}

		Astronaut::~Astronaut() { oapiReleaseFont(hudInfo.deadFont); }

		void Astronaut::clbkSetClassCaps(FILEHANDLE cfg)
		{
			char cBuffer[512];

			if (!oapiReadItem_string(cfg, "DefaultName", cBuffer)) LogTerminate("DefaultName", GetClassNameA());
			astrInfo.name = cBuffer;

			if (!oapiReadItem_string(cfg, "DefaultRole", cBuffer)) LogTerminate("DefaultRole", GetClassNameA());
			astrInfo.role = cBuffer;

			if (!oapiReadItem_string(cfg, "SuitMesh", cBuffer)) LogTerminate("SuitMesh", GetClassNameA());
			suitMesh = AddMesh(cBuffer);

			if (!oapiReadItem_string(cfg, "BodyMesh", cBuffer)) LogTerminate("BodyMesh", GetClassNameA());
			bodyMesh = AddMesh(cBuffer);

			if (!oapiReadItem_float(cfg, "SuitMass", suitMass)) LogTerminate("SuitMass", GetClassNameA());

			if (!oapiReadItem_float(cfg, "DefaultBodyMass", astrInfo.mass)) LogTerminate("DefaultBodyMass", GetClassNameA());

			if (!oapiReadItem_float(cfg, "SuitHeight", suitHeight)) LogTerminate("SuitHeight", GetClassNameA());

			if (!oapiReadItem_float(cfg, "BodyHeight", bodyHeight)) LogTerminate("BodyHeight", GetClassNameA());

			if (!oapiReadItem_vec(cfg, "SuitHoldDir", suitHoldDir)) LogTerminate("SuitHoldDir", GetClassNameA());

			if (!oapiReadItem_vec(cfg, "BodyHoldDir", bodyHoldDir)) LogTerminate("BodyHoldDir", GetClassNameA());

			VECTOR3 camOffset{};
			oapiReadItem_vec(cfg, "CameraOffset", camOffset);

			if (oapiReadItem_vec(cfg, "SpotLightPos", spotInfo.pos) && oapiReadItem_vec(cfg, "SpotLightDir", spotInfo.dir))
			{
				for (size_t idx{ 1 }; ; ++idx)
				{
					VECTOR3 pos;
					if (!oapiReadItem_vec(cfg, std::format("Beacon{}Pos", idx).data(), pos)) break;

					BeaconInfo& beaconInfo = beacons.emplace_front();
					beaconInfo.pos = pos;

					AddBeacon(&beaconInfo.spec);
				}

				if (!beacons.empty())
				{
					spotLight = static_cast<SpotLight*>(AddSpotLight(spotInfo.pos, spotInfo.dir, spotInfo.range,
						spotInfo.att0, spotInfo.att1, spotInfo.att2, spotInfo.umbra, spotInfo.penumbra,
						spotInfo.diffuse, spotInfo.specular, spotInfo.ambient));
					spotLight->Activate(false);
				}
			}

			astrInfo.height = suitHeight;

			tdVtx =
			{ {
			{ { 0,        -astrInfo.height,   0.1858 },   1.3e4, 2.7e3, 3, 3},
			{ { -0.178,   -astrInfo.height,  -0.1305 },   1.3e4, 2.7e3, 3, 3},
			{ { 0.178,    -astrInfo.height,  -0.1305 },   1.3e4, 2.7e3, 3, 3},

			{ {-0.203865, -0.047471, -0.455052},  1.3e4, 2.7e3, 3, 3},
			{ { 0.208315, -0.047471, -0.455052 }, 1.3e4, 2.7e3, 3, 3},

			{ { -0.237215, 0.430504, -0.369051 }, 1.3e4, 2.7e3, 3, 3},
			{ { 0.241665,  0.430504, -0.369051 }, 1.3e4, 2.7e3, 3, 3},

			{ { -0.147315, 0.621811,  0.038013 }, 1.3e4, 2.7e3, 3, 3},
			{ { 0.134205,  0.61959,   0.035462 }, 1.3e4, 2.7e3, 3, 3},

			{ { -0.043125, 0.244441,  0.177825 }, 1.3e4, 2.7e3, 3, 3},
			{ { 0.042435,  0.244441,  0.177825 }, 1.3e4, 2.7e3, 3, 3},

			{ { -0.410339, 0.101468,  0.094267 }, 1.3e4, 2.7e3, 3, 3},
			{ { 0.406829,  0.108122,  0.0914 },   1.3e4, 2.7e3, 3, 3}
			} };

			SetTouchdownPoints(tdVtx.data(), tdVtx.size());

			if (enableCockpit)
			{
				cockpitMesh = AddMesh("UACS/AstronautCockpit", &camOffset);
				SetMeshVisibilityMode(cockpitMesh, MESHVIS_NEVER);

				static UINT VisorGrp[1] = { 2 };
				static MGROUP_ROTATE VisorRot(cockpitMesh, VisorGrp, 1, { 0, -0.03, -0.01 }, { 1, 0, 0 }, float(-PI05));

				visorAnim.id = CreateAnimation(0);
				AddAnimationComponent(visorAnim.id, 0, 1, &VisorRot);
			}
		}

		void Astronaut::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			char* line;

			while (oapiReadScenario_nextline(scn, line))
			{
				std::istringstream ss(line);
				std::string data;

				if (ss >> data >> std::ws)
				{
					if (data == "NAME") { std::getline(ss, data); astrInfo.name = data; }

					else if (data == "ROLE") ss >> astrInfo.role;

					else if (data == "MASS") ss >> astrInfo.mass;

					else if (data == "ALIVE") ss >> astrInfo.alive;

					else if (data == "SUIT_ON") { ss >> suitOn; suitRead = true; }

					else if (enableCockpit && data == "VISOR_STATE") ss >> visorAnim.state;

					else if (enableCockpit && data == "VISOR_PROC") { ss >> visorAnim.proc; SetAnimation(visorAnim.id, visorAnim.proc); }

					else if (data == "HUD_MODE") ss >> hudInfo.mode;

					else if (spotLight && data == "HEADLIGHT") { bool active; ss >> active; SetHeadlight(active); }

					else if (!mdlAPI.ParseScenarioLine(line)) ParseScenarioLineEx(line, status);
				}

				else if (!mdlAPI.ParseScenarioLine(line)) ParseScenarioLineEx(line, status);
			}
		}

		void Astronaut::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);
			mdlAPI.clbkSaveState(scn);

			oapiWriteScenario_string(scn, "NAME", astrInfo.name.data());
			oapiWriteScenario_string(scn, "ROLE", astrInfo.role.data());
			oapiWriteScenario_float(scn, "MASS", astrInfo.mass);

			oapiWriteScenario_int(scn, "ALIVE", astrInfo.alive);
			oapiWriteScenario_int(scn, "SUIT_ON", suitOn);

			if (enableCockpit)
			{
				oapiWriteScenario_int(scn, "VISOR_STATE", visorAnim.state);
				oapiWriteScenario_float(scn, "VISOR_PROC", visorAnim.proc);
			}

			oapiWriteScenario_int(scn, "HUD_MODE", hudInfo.mode);
			if (spotLight) oapiWriteScenario_int(scn, "HEADLIGHT", spotLight->IsActive());
		}

		void Astronaut::clbkPostCreation()
		{
			InitPropellant();

			UACS::SlotInfo slotInfo{ GetAttachmentHandle(false, 0) };
			slotInfo.holdDir = suitHoldDir;
			vslCargoInfo.slots.emplace_back(slotInfo);

			if (!suitRead) suitOn = !InBreathable();
			SetSuit(suitOn, false);

			mdlAPI.clbkPostCreation();

			if (!astrInfo.alive) Kill(GetFlightStatus());
		}

		bool Astronaut::clbkSetAstrInfo(const UACS::AstrInfo& astrInfo)
		{
			this->astrInfo.name = astrInfo.name;
			this->astrInfo.role = astrInfo.role;
			this->astrInfo.mass = astrInfo.mass;
			this->astrInfo.fuelLvl = astrInfo.fuelLvl;
			this->astrInfo.oxyLvl = astrInfo.oxyLvl;
			this->astrInfo.alive = astrInfo.alive;

			SetEmptyMass((suitOn ? suitMass : 0) + astrInfo.mass);
			SetPropellantMass(hFuel, astrInfo.fuelLvl * GetPropellantMaxMass(hFuel));
			SetPropellantMass(hOxy, astrInfo.oxyLvl * GetPropellantMaxMass(hOxy));

			return true;
		}

		const UACS::AstrInfo* Astronaut::clbkGetAstrInfo()
		{
			// Oxygen level is set in SetOxygenConsumption
			astrInfo.fuelLvl = GetPropellantMass(hFuel) / GetPropellantMaxMass(hFuel);			

			return &astrInfo;
		}

		int Astronaut::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
		{
			if (!astrInfo.alive || !down || !KEYMOD_ALT(kstate)) return 0;

			switch (hudInfo.mode)
			{
			case HUD_VSL:
				switch (key)
				{
				case OAPI_KEY_NUMPAD6:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					SetMapIdx(hudInfo.vslMap, true);
					return 1;

				case OAPI_KEY_NUMPAD4:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					SetMapIdx(hudInfo.vslMap, false);
					return 1;

				case OAPI_KEY_NUMPAD3:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					hudInfo.vslInfo.arlckIdx + 1 < hudInfo.vslInfo.info->airlocks.size() ? ++hudInfo.vslInfo.arlckIdx : hudInfo.vslInfo.arlckIdx = 0;
					return 1;

				case OAPI_KEY_NUMPAD1:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					hudInfo.vslInfo.arlckIdx > 0 ? --hudInfo.vslInfo.arlckIdx : hudInfo.vslInfo.arlckIdx = hudInfo.vslInfo.info->airlocks.size() - 1;
					return 1;

				case OAPI_KEY_NUMPAD9:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					hudInfo.vslInfo.actionIdx + 1 < hudInfo.vslInfo.info->actionAreas.size() ? ++hudInfo.vslInfo.actionIdx : hudInfo.vslInfo.actionIdx = 0;
					return 1;

				case OAPI_KEY_NUMPAD7:
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					hudInfo.vslInfo.actionIdx > 0 ? --hudInfo.vslInfo.actionIdx : hudInfo.vslInfo.actionIdx = hudInfo.vslInfo.info->actionAreas.size() - 1;
					return 1;

				case OAPI_KEY_T:
					hudInfo.drainFuel = !hudInfo.drainFuel;
					return 1;

				case OAPI_KEY_F:
				{
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					const char* resource;
					double reqMass;

					if (hudInfo.drainFuel) { resource = "fuel"; reqMass = GetPropellantMaxMass(hFuel) - GetPropellantMass(hFuel); }
					else { resource = "oxygen"; reqMass = GetPropellantMaxMass(hOxy) - GetPropellantMass(hOxy); }

					if (reqMass)
					{
						auto drainInfo = mdlAPI.DrainStationResource(resource, reqMass, hudInfo.hVessel);

						switch (drainInfo.first)
						{
						case UACS::DRIN_SUCCED:
							if (hudInfo.drainFuel) SetPropellantMass(hFuel, GetPropellantMass(hFuel) + drainInfo.second);
							else SetPropellantMass(hOxy, GetPropellantMass(hOxy) + drainInfo.second);

							hudInfo.message = std::format("Success: {:g} kg {} drained.", drainInfo.second, resource);
							break;

						case UACS::DRIN_NOT_IN_RNG:
							hudInfo.message = "Error: Selected vessel out of range.";
							break;

						case UACS::DRIN_VSL_NOT_RES:
							hudInfo.message = "Error: Selected vessel not resource station.";
							break;

						case UACS::DRIN_RES_NOT_FND:
							hudInfo.message = "Error: Selected station doesn't contain resource.";
							break;

						case UACS::DRIN_FAIL:
							hudInfo.message = "Error: Drainage failed.";
							break;
						}
					}
					else hudInfo.message = "Error: Selected resource full.";

					hudInfo.timer = 0;
					return 1;
				}
				}

				[[fallthrough]];

			case HUD_NST:
				if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

				switch (key)
				{
				case OAPI_KEY_NUMPAD8:
					hudInfo.vslInfo.statIdx + 1 < hudInfo.vslInfo.info->stations.size() ? ++hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = 0;
					return 1;

				case OAPI_KEY_NUMPAD2:
					hudInfo.vslInfo.statIdx > 0 ? --hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = hudInfo.vslInfo.info->stations.size() - 1;
					return 1;

				case OAPI_KEY_I:
					if (vslCargoInfo.slots.front().cargoInfo)
					{
						hudInfo.message = "Error: A cargo is grappled.";
						hudInfo.timer = 0;
						return 1;
					}

					switch (Ingress(hudInfo.hVessel, hudInfo.vslInfo.arlckIdx, hudInfo.vslInfo.statIdx))
					{
					case UACS::INGRS_SUCCED:
						return 1;

					case UACS::INGRS_NOT_IN_RNG:
						if (hudInfo.mode == HUD_VSL) hudInfo.message = "Error: Selected airlock out of range.";
						else hudInfo.message = "Error: Nearest airlock out of range.";
						break;

					case UACS::INGRS_ARLCK_UNDEF:
						hudInfo.message = "Error: Selected vessel has no airlocks.";
						break;

					case UACS::INGRS_ARLCK_CLSD:
						hudInfo.message = "Error: Selected airlock closed.";
						break;

					case UACS::INGRS_STN_UNDEF:
						hudInfo.message = "Error: Selected vessel has no stations.";
						break;

					case UACS::INGRS_STN_OCCP:
						hudInfo.message = "Error: Selected station occupied.";
						break;

					case UACS::INGRS_VSL_REJC:
						hudInfo.message = "Error: Selected vessel rejected ingress.";
						break;

					case UACS::INGRS_FAIL:
						hudInfo.message = "Error: Ingress failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_A:
					switch (TriggerAction(hudInfo.hVslAction, hudInfo.vslInfo.actionIdx))
					{
					case UACS::INGRS_SUCCED:
						hudInfo.message = "Success: Action triggered.";
						break;

					case UACS::INGRS_NOT_IN_RNG:
						if (hudInfo.mode == HUD_VSL) hudInfo.message = "Error: Selected action area out of range.";
						else hudInfo.message = "Error: Nearest action area out of range.";
						break;

					case UACS::INGRS_ARLCK_UNDEF:
						hudInfo.message = "Error: Selected vessel has no action areas.";
						break;

					case UACS::INGRS_ARLCK_CLSD:
						hudInfo.message = "Error: Selected action area disabled.";
						break;

					case UACS::INGRS_VSL_REJC:
						hudInfo.message = "Error: Selected vessel rejected action area trigger.";
						break;

					case UACS::INGRS_FAIL:
						hudInfo.message = "Error: Action area trigger failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;
				}

				break;

			case HUD_AST:
			{
				if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

				if (key == OAPI_KEY_NUMPAD6) SetMapIdx(hudInfo.astrMap, true);
				else if (key == OAPI_KEY_NUMPAD4) SetMapIdx(hudInfo.astrMap, false);
				else break;

				return 1;
			}

			case HUD_CRG:
			{
				OBJHANDLE hCargo{};

				if (KEYMOD_CONTROL(kstate))
				{
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;
					hCargo = hudInfo.hVessel;
				}

				switch (key)
				{
				case OAPI_KEY_NUMPAD6:
				case OAPI_KEY_NUMPAD4:
				{
					if (!oapiCameraInternal() || oapiGetHUDMode() == HUD_NONE) return 0;

					if (mdlAPI.GetScnCargoCount())
					{
						do
						{
							if (key == OAPI_KEY_NUMPAD6) hudInfo.vslIdx + 1 < mdlAPI.GetScnCargoCount() ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
							else hudInfo.vslIdx > 0 ? --hudInfo.vslIdx : hudInfo.vslIdx = mdlAPI.GetScnCargoCount() - 1;
						} while (mdlAPI.GetCargoInfoByIndex(hudInfo.vslIdx).attached);

						hudInfo.hVessel = mdlAPI.GetCargoInfoByIndex(hudInfo.vslIdx).handle;
					}

					return 1;
				}

				case OAPI_KEY_T:
					hudInfo.drainFuel = !hudInfo.drainFuel;
					return 1;
				
				case OAPI_KEY_D:
					switch (mdlAPI.DeleteCargo())
					{
					case UACS::RLES_SUCCED:
						hudInfo.message = "Success: Grappled cargo deleted.";
						break;

					case UACS::RLES_SLT_EMPTY:
						hudInfo.message = "Error: No cargo grappled.";
						break;

					case UACS::RLES_FAIL:
						hudInfo.message = "Error: Deletion failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_G:
					switch (mdlAPI.GrappleCargo(hCargo))
					{
					case UACS::GRPL_SUCCED:
						if (hCargo) hudInfo.message = "Success: Selected cargo grappled.";
						else hudInfo.message = "Success: Nearest cargo grappled.";
						break;

					case UACS::GRPL_SLT_OCCP:
						hudInfo.message = "Error: A cargo is already grappled.";
						break;

					case UACS::GRPL_NOT_IN_RNG:
						if (hCargo) hudInfo.message = "Error: Selected cargo out of range.";
						else hudInfo.message = "Error: No cargo in range.";
						break;

					case UACS::GRPL_FAIL:
						hudInfo.message = "Error: Grapple failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_R:
					switch (mdlAPI.ReleaseCargo())
					{
					case UACS::RLES_SUCCED:
						hudInfo.message = "Success: Grappled cargo released.";
						break;

					case UACS::RLES_SLT_EMPTY:
						hudInfo.message = "Error: No cargo grappled.";
						break;

					case UACS::RLES_FAIL:
						hudInfo.message = "Error: Release failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_P:
					switch (mdlAPI.PackCargo(hCargo))
					{
					case UACS::PACK_SUCCED:
						if (hCargo) hudInfo.message = "Success: Selected cargo packed.";
						else hudInfo.message = "Success: Nearest cargo packed.";
						break;

					case UACS::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.message = "Error: Selected cargo out of range.";
						else hudInfo.message = "Error: No packable cargo in range.";
						break;

					case UACS::PACK_CRG_PCKD:
						hudInfo.message = "Error: Selected cargo already packed.";
						break;

					case UACS::PACK_CRG_NOT_PCKABL:
						hudInfo.message = "Error: Selected cargo not packable.";
						break;

					case UACS::PACK_FAIL:
						hudInfo.message = "Error: Packing failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_U:
					switch (mdlAPI.UnpackCargo(hCargo))
					{
					case UACS::PACK_SUCCED:
						if (hCargo) hudInfo.message = "Success: Selected cargo unpacked.";
						else hudInfo.message = "Success: Nearest cargo unpacked.";
						break;

					case UACS::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.message = "Error: Selected cargo out of range.";
						else hudInfo.message = "Error: No unpackable cargo in range.";
						break;

					case UACS::PACK_CRG_UNPCKD:
						hudInfo.message = "Error: Selected cargo already unpacked.";
						break;

					case UACS::PACK_CRG_NOT_PCKABL:
						hudInfo.message = "Error: Selected cargo not unpackable.";
						break;

					case UACS::PACK_FAIL:
						hudInfo.message = "Error: Unpacking failed.";
						break;
					}

					hudInfo.timer = 0;
					return 1;

				case OAPI_KEY_F:
				{
					const char* resource;
					double reqMass;

					if (hudInfo.drainFuel) { resource = "fuel"; reqMass = GetPropellantMaxMass(hFuel) - GetPropellantMass(hFuel); }
					else { resource = "oxygen"; reqMass = GetPropellantMaxMass(hOxy) - GetPropellantMass(hOxy); }

					if (reqMass)
					{
						auto drainInfo = mdlAPI.DrainScenarioResource(resource, reqMass, hCargo);

						switch (drainInfo.first)
						{
						case UACS::DRIN_SUCCED:
							if (hudInfo.drainFuel) SetPropellantMass(hFuel, GetPropellantMass(hFuel) + drainInfo.second);
							else SetPropellantMass(hOxy, GetPropellantMass(hOxy) + drainInfo.second);

							hudInfo.message = std::format("Success: {:g} kg {} drained.", drainInfo.second, resource);
							break;

						case UACS::DRIN_NOT_IN_RNG:
							if (hCargo) hudInfo.message = "Error: Selected cargo out of range.";
							else hudInfo.message = "Error: No resource cargo in range.";
							break;

						case UACS::DRIN_VSL_NOT_RES:
							hudInfo.message = "Error: Selected cargo not resource cargo.";
							break;

						case UACS::DRIN_RES_NOT_FND:
							hudInfo.message = "Error: Selected cargo doesn't contain resource.";
							break;

						case UACS::DRIN_FAIL:
							hudInfo.message = "Error: Drainage failed.";
							break;
						}
					}
					else hudInfo.message = "Error: Selected resource full.";

					hudInfo.timer = 0;
					return 1;
				}
				}

				break;
			}
			}

			switch (key)
			{
			case OAPI_KEY_M:
				hudInfo.mode < 6 ? ++hudInfo.mode : hudInfo.mode = 0;

				hudInfo.vslIdx = 0;
				hudInfo.hVessel = nullptr;
				hudInfo.vslInfo = HudInfo::VesselInfo();
				return 1;

			case OAPI_KEY_S:
				if (vslCargoInfo.slots.front().cargoInfo)
				{
					hudInfo.message = "Error: A cargo is grappled.";
					hudInfo.timer = 0;
				}

				else if (!astrInfo.oxyLvl && !suitOn)
				{
					hudInfo.message = "Error: No oxygen available.";
					hudInfo.timer = 0;
				}

				else SetSuit(!suitOn, true);

				return 1;

			case OAPI_KEY_V:
				if (enableCockpit && suitOn)
				{
					if (visorAnim.proc) visorAnim.state = -1;
					else visorAnim.state = 1;
				}
				return 1;

			case OAPI_KEY_L:
				if (suitOn && spotLight) SetHeadlight(!spotLight->IsActive());
				return 1;

			default:
				return 0;
			}
		}

		int Astronaut::clbkConsumeDirectKey(char* kstate)
		{
			if (!astrInfo.alive || !GetFlightStatus() || KEYMOD_ALT(kstate)) return 0;

			if (!latSpeed.value) SetValue(lonSpeed, KEYDOWN(kstate, OAPI_KEY_NUMPAD8), KEYDOWN(kstate, OAPI_KEY_NUMPAD2), KEYMOD_CONTROL(kstate));

			if (!lonSpeed.value) SetValue(latSpeed, KEYDOWN(kstate, OAPI_KEY_NUMPAD3), KEYDOWN(kstate, OAPI_KEY_NUMPAD1), KEYMOD_CONTROL(kstate));

			bool setSlowAngle = KEYMOD_CONTROL(kstate);

			if (enhancedMovements && (KEYDOWN(kstate, OAPI_KEY_NUMPAD6) || KEYDOWN(kstate, OAPI_KEY_NUMPAD4)))
			{
				const double speed = abs(lonSpeed.value) + abs(latSpeed.value);

				if (speed)
				{
					steerAngle.minLimit = -(steerAngle.maxLimit = min(surfInfo.steerRatio / (7.7157 * speed), steerAngle.maxMinRateConst));

					if (steerAngle.maxLimit < steerAngle.maxSlowLimit) setSlowAngle = false;
				}

				else steerAngle.minLimit = -(steerAngle.maxLimit = steerAngle.maxMinRateConst);
			}

			SetValue(steerAngle, KEYDOWN(kstate, OAPI_KEY_NUMPAD6), KEYDOWN(kstate, OAPI_KEY_NUMPAD4), setSlowAngle);

			keyDown = true;

			return 0;
		}

		void Astronaut::clbkPreStep(double simt, double simdt, double mjd)
		{
			if (astrInfo.alive) SetOxygenConsumption(simdt);

			if (!astrInfo.alive) 
			{
				SetAttitudeMode(RCS_NONE);
				if (!GetFlightStatus()) SetLandedStatus();

				return;
			}

			if (keyDown) keyDown = false;
			else
			{
				SetValue(lonSpeed, false, false, false);

				SetValue(latSpeed, false, false, false);

				SetValue(steerAngle, false, false, false);
			}

			if (GetFlightStatus())
			{
				SetAttitudeMode(RCS_NONE);

				if (surfInfo.ref != GetSurfaceRef()) SetSurfaceRef();

				if (lonSpeed.value || latSpeed.value || steerAngle.value) SetGroundMovement(simdt);

				avgForce = forceStep = 0;
			}
			else
			{
				if (enhancedMovements) CalcForces();

				SetLandedStatus();

				lonSpeed.value = latSpeed.value = steerAngle.value = 0;
			}

			if (totalRunDist && lonSpeed.value <= 1.55) totalRunDist = max(totalRunDist - (5 * simdt), 0);

			if (enableCockpit && visorAnim.state) SetVisorAnim(simdt);

			if (hudInfo.timer < 5) hudInfo.timer += simdt;
		}

		bool Astronaut::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			if (!astrInfo.alive)
			{
				skp->SetTextColor(0x0000ff);
				skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
				skp->SetFont(hudInfo.deadFont);

				skp->Text(hps->CX, hps->CY, "DEAD", 4);
				return true;
			}

			int x = HIWORD(skp->GetCharSize());
			hudInfo.rightX = hps->W - x;
			int y = hudInfo.startY = int(0.212 * hps->H);

			hudInfo.space = LOWORD(skp->GetCharSize());
			hudInfo.smallSpace = int(0.5 * hudInfo.space);
			hudInfo.largeSpace = int(1.5 * hudInfo.space);

			if (hudInfo.timer < 5)
			{
				skp->SetTextAlign(oapi::Sketchpad::RIGHT);

				skp->Text(hudInfo.rightX, hudInfo.startY, hudInfo.message.c_str(), hudInfo.message.size());
				hudInfo.startY += hudInfo.largeSpace;

				skp->SetTextAlign(oapi::Sketchpad::LEFT);
			}

			buffer = astrInfo.role;
			buffer[0] = std::toupper(buffer[0]);
			buffer = std::format("Name: {} - Role: {}", astrInfo.name, buffer);

			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			if (suitOn)
			{
				const double time = GetPropellantMass(hOxy) / (consumptionRate * 3600);
				const int hours = int(time);
				const double minutesRemainder = (time - hours) * 60;
				const int minutes = int(minutesRemainder);
				const int seconds = int((minutesRemainder - minutes) * 60);

				buffer = std::format("Oxygen Level: {:.1f}% - Duration: {:02d}:{:02d}:{:02d}", astrInfo.oxyLvl * 100, hours, minutes, seconds);
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;
			}
			
			y += hudInfo.smallSpace;

			switch (hudInfo.mode)
			{
			case HUD_NST:
				DrawNearHUD(x, y, skp);
				break;

			case HUD_VSL:
				DrawVslHUD(x, y, skp);
				break;

			case HUD_AST:
				DrawAstrHUD(x, y, skp);
				break;

			case HUD_CRG:
				DrawCargoHUD(x, y, skp);
				break;

			case HUD_SRT1:
				DrawShort1HUD(x, y, skp);
				break;

			case HUD_SRT2:
				DrawShort2HUD(x, y, skp);
				break;
			}

			return true;
		}

		void Astronaut::InitPropellant()
		{
			hFuel = GetPropellantHandleByIndex(0);
			hOxy = GetPropellantHandleByIndex(1);

			THRUSTER_HANDLE thRCS[14], thGroup[4];

			thRCS[0] = CreateThruster({ 1.23, 0, 1.23 }, { 0, 1, 0 }, 1, hFuel, 1e3);
			thRCS[1] = CreateThruster({ 1.23, 0, 1.23 }, { 0, -1, 0 }, 1, hFuel, 1e3);
			thRCS[2] = CreateThruster({ -1.23, 0, 1.23 }, { 0, 1, 0 }, 1, hFuel, 1e3);
			thRCS[3] = CreateThruster({ -1.23, 0, 1.23 }, { 0, -1, 0 }, 1, hFuel, 1e3);
			thRCS[4] = CreateThruster({ 1.23, 0, -1.23 }, { 0, 1, 0 }, 1, hFuel, 1e3);
			thRCS[5] = CreateThruster({ 1.23, 0, -1.23 }, { 0, -1, 0 }, 1, hFuel, 1e3);
			thRCS[6] = CreateThruster({ -1.23, 0, -1.23 }, { 0, 1, 0 }, 1, hFuel, 1e3);
			thRCS[7] = CreateThruster({ -1.23, 0, -1.23 }, { 0, -1, 0 }, 1, hFuel, 1e3);
			thRCS[8] = CreateThruster({ 1.23, 0, 1.23 }, { -1, 0, 0 }, 1, hFuel, 1e3);
			thRCS[9] = CreateThruster({ -1.23, 0, 1.23 }, { 1, 0, 0 }, 1, hFuel, 1e3);
			thRCS[10] = CreateThruster({ 1.23, 0, -1.23 }, { -1, 0, 0 }, 1, hFuel, 1e3);
			thRCS[11] = CreateThruster({ -1.23, 0, -1.23 }, { 1, 0, 0 }, 1, hFuel, 1e3);
			thRCS[12] = CreateThruster({ 0, 0, -1.23 }, { 0, 0, 1 }, 1, hFuel, 1e3);
			thRCS[13] = CreateThruster({ 0, 0, 1.23 }, { 0, 0, -1 }, 1, hFuel, 1e3);

			thGroup[0] = thRCS[0];
			thGroup[1] = thRCS[2];
			thGroup[2] = thRCS[5];
			thGroup[3] = thRCS[7];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_PITCHUP);

			thGroup[0] = thRCS[1];
			thGroup[1] = thRCS[3];
			thGroup[2] = thRCS[4];
			thGroup[3] = thRCS[6];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_PITCHDOWN);

			thGroup[0] = thRCS[0];
			thGroup[1] = thRCS[4];
			thGroup[2] = thRCS[3];
			thGroup[3] = thRCS[7];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_BANKLEFT);

			thGroup[0] = thRCS[1];
			thGroup[1] = thRCS[5];
			thGroup[2] = thRCS[2];
			thGroup[3] = thRCS[6];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_BANKRIGHT);

			thGroup[0] = thRCS[0];
			thGroup[1] = thRCS[4];
			thGroup[2] = thRCS[2];
			thGroup[3] = thRCS[6];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_UP);

			thGroup[0] = thRCS[1];
			thGroup[1] = thRCS[5];
			thGroup[2] = thRCS[3];
			thGroup[3] = thRCS[7];
			CreateThrusterGroup(thGroup, 4, THGROUP_ATT_DOWN);

			thGroup[0] = thRCS[8];
			thGroup[1] = thRCS[11];
			CreateThrusterGroup(thGroup, 2, THGROUP_ATT_YAWLEFT);

			thGroup[0] = thRCS[9];
			thGroup[1] = thRCS[10];
			CreateThrusterGroup(thGroup, 2, THGROUP_ATT_YAWRIGHT);

			thGroup[0] = thRCS[8];
			thGroup[1] = thRCS[10];
			CreateThrusterGroup(thGroup, 2, THGROUP_ATT_LEFT);

			thGroup[0] = thRCS[9];
			thGroup[1] = thRCS[11];
			CreateThrusterGroup(thGroup, 2, THGROUP_ATT_RIGHT);

			CreateThrusterGroup(thRCS + 12, 1, THGROUP_ATT_FORWARD);
			CreateThrusterGroup(thRCS + 13, 1, THGROUP_ATT_BACK);

			astrInfo.fuelLvl = GetPropellantMass(hFuel) / GetPropellantMaxMass(hFuel);
			astrInfo.oxyLvl = GetPropellantMass(hOxy) / GetPropellantMaxMass(hOxy);
		}

		void Astronaut::SetOxygenConsumption(double simdt)
		{
			if (suitOn)
			{
				const double speed = abs(lonSpeed.value) + abs(latSpeed.value) + abs(steerAngle.value) / PI;

				if (speed > 2.7) consumptionRate = speed * 3.233e-5;
				else if (speed) consumptionRate = speed * (1.079e-5 * speed + 3.233e-6);
				else consumptionRate = 0;

				consumptionRate += 9.43e-6;

				double oxyMass = GetPropellantMass(hOxy) - (consumptionRate * simdt);

				if (oxyMass > 0) { SetPropellantMass(hOxy, oxyMass); astrInfo.oxyLvl = oxyMass / GetPropellantMaxMass(hOxy); }
				else
				{
					SetPropellantMass(hOxy, 0);
					astrInfo.oxyLvl = 0;

					if (InBreathable()) { mdlAPI.ReleaseCargo(0); SetSuit(false, false); }
					else Kill(GetFlightStatus());
				}
			}
			else if (!InBreathable()) Kill(GetFlightStatus());
		}

		void Astronaut::SetLandedStatus()
		{
			if (GroundContact())
			{
				VECTOR3 angAcc; GetAngularAcc(angAcc);

				if (length(angAcc) < 0.5)
				{
					if (astrInfo.alive)
					{
						VESSELSTATUS2 status = GetVesselStatus(this);
						status.status = 1;

						SetGroundRotation(status, astrInfo.height);
						DefSetStateEx(&status);
					}
					else Kill(true);
				}
			}
		}

		void Astronaut::SetSurfaceRef()
		{
			surfInfo.ref = GetSurfaceRef();

			const double radius = oapiGetSize(surfInfo.ref);

			surfInfo.gravityAccel = GGRAV * oapiGetMass(surfInfo.ref) / (radius * radius);

			surfInfo.steerRatio = 1.118 * surfInfo.gravityAccel;

			if (enhancedMovements)
			{
				if (surfInfo.gravityAccel < 1.55)
				{
					lonSpeed.maxSlowLimit = lonSpeed.maxMinRate = latSpeed.maxMinRate = surfInfo.gravityAccel;

					lonSpeed.minSlowLimit = latSpeed.minSlowLimit = -(latSpeed.maxSlowLimit = 0.323 * lonSpeed.maxMinRate);

					lonSpeed.returnRate = latSpeed.returnRate = 0.5 * lonSpeed.maxMinRate;

					if (surfInfo.gravityAccel < 0.14)
					{
						steerAngle.maxMinRate = steerAngle.maxMinRateConst = 5.618 * surfInfo.gravityAccel;

						steerAngle.minSlowLimit = -(steerAngle.maxSlowLimit = steerAngle.returnRate = 0.5 * steerAngle.maxMinRate);
					}
				}

				else SetDefaultValues();
			}
		}

		void Astronaut::SetGroundMovement(double simdt)
		{
			VESSELSTATUS2 status = GetVesselStatus(this);

			if (steerAngle.value) status.surf_hdg = fmod(status.surf_hdg + steerAngle.value * simdt, PI2);

			if (lonSpeed.value)
			{
				SetLngLatHdg(lonSpeed.value * simdt, 0, status);

				if (enhancedMovements && lonSpeed.value > 1.55)
				{
					totalRunDist += abs(lonSpeed.value) * simdt;
					lonSpeed.maxLimit = totalRunDist / ((suitOn ? 20 : 15) * pow(totalRunDist / 100, 1.06));
				}
				else lonSpeed.maxLimit = 10;
			}

			else if (latSpeed.value) SetLngLatHdg(latSpeed.value * simdt, PI05, status);

			SetGroundRotation(status, astrInfo.height);

			DefSetStateEx(&status);
		}

		void Astronaut::SetLngLatHdg(double distance, double hdgOffset, VESSELSTATUS2& status)
		{
			distance /= oapiGetSize(surfInfo.ref);

			double hdg = fmod(status.surf_hdg + hdgOffset, PI2);
			double finalLat = asin(sin(status.surf_lat) * cos(distance) + cos(status.surf_lat) * sin(distance) * cos(hdg));
			double lngOffset = atan2(sin(hdg) * sin(distance) * cos(status.surf_lat), cos(distance) - sin(status.surf_lat) * sin(finalLat));
			double finalLng = status.surf_lng + lngOffset;

			// Distance less than that will result in rapid heading chnages
			if (abs(distance) > 1.6e-9)
			{
				double y = sin(lngOffset) * cos(finalLat);
				double x = cos(status.surf_lat) * sin(finalLat) - sin(status.surf_lat) * cos(finalLat) * cos(lngOffset);

				if (distance > 0) status.surf_hdg = fmod(atan2(y, x) + PI2 - hdgOffset, PI2);
				else status.surf_hdg = fmod(atan2(y, x) + PI - hdgOffset, PI2);
			}

			status.surf_lng = finalLng;
			status.surf_lat = finalLat;
		}

		void Astronaut::SetDefaultValues()
		{
			lonSpeed.maxSlowLimit = lonSpeed.maxMinRate = latSpeed.maxMinRate = 1.55;

			lonSpeed.minSlowLimit = latSpeed.minSlowLimit = -(latSpeed.maxSlowLimit = 0.5);

			lonSpeed.returnRate = latSpeed.returnRate = 0.775;

			steerAngle.maxMinRate = steerAngle.maxMinRateConst = 45 * RAD;

			steerAngle.minSlowLimit = -(steerAngle.maxSlowLimit = steerAngle.returnRate = 22.5 * RAD);
		}

		void Astronaut::SetValue(ValueInfo& valueInfo, bool setMax, bool setMin, bool setSlow)
		{
			if (setMax)
			{
				if (setSlow && valueInfo.maxSlowLimit)
				{
					if (valueInfo.value < valueInfo.maxSlowLimit) valueInfo.value = min(valueInfo.value + (valueInfo.maxMinRate * oapiGetSimStep()), valueInfo.maxSlowLimit);

					else if (valueInfo.value > valueInfo.maxSlowLimit) valueInfo.value = max(valueInfo.value - (valueInfo.returnRate * oapiGetSimStep()), valueInfo.maxSlowLimit);
				}

				else if (valueInfo.value < valueInfo.maxLimit) valueInfo.value = min(valueInfo.value + (valueInfo.maxMinRate * oapiGetSimStep()), valueInfo.maxLimit);

				else if (valueInfo.value > valueInfo.maxLimit) valueInfo.value = max(valueInfo.value - (valueInfo.returnRate * oapiGetSimStep()), valueInfo.maxLimit);
			}

			else if (setMin)
			{
				if (setSlow && valueInfo.minSlowLimit)
				{
					if (valueInfo.value > valueInfo.minSlowLimit) valueInfo.value = max(valueInfo.value - (valueInfo.maxMinRate * oapiGetSimStep()), valueInfo.minSlowLimit);

					else if (valueInfo.value < valueInfo.minSlowLimit) valueInfo.value = min(valueInfo.value + (valueInfo.returnRate * oapiGetSimStep()), valueInfo.minSlowLimit);
				}

				else if (valueInfo.value > valueInfo.minLimit) valueInfo.value = max(valueInfo.value - (valueInfo.maxMinRate * oapiGetSimStep()), valueInfo.minLimit);

				else if (valueInfo.value < valueInfo.minLimit) valueInfo.value = min(valueInfo.value + (valueInfo.returnRate * oapiGetSimStep()), valueInfo.minLimit);
			}

			else if (valueInfo.value)
			{
				if (valueInfo.value > 0) valueInfo.value = max(valueInfo.value - (valueInfo.returnRate * oapiGetSimStep()), 0);

				else valueInfo.value = min(valueInfo.value + (valueInfo.returnRate * oapiGetSimStep()), 0);
			}
		}

		void Astronaut::CalcForces()
		{
			VECTOR3 force;
			GetForceVector(force);

			avgForce += length(force);
			++forceStep;

			if (forceStep >= 10)
			{
				if ((avgForce / forceStep) > 32e3) Kill(false);
				avgForce = forceStep = 0;
			}
		}

		void Astronaut::SetVisorAnim(double simdt)
		{
			visorAnim.proc += visorAnim.state * simdt * 2;

			if (visorAnim.proc >= 1) { visorAnim.proc = 1; visorAnim.state = 0; }
			else if (visorAnim.proc <= 0) { visorAnim.proc = 0; visorAnim.state = 0; }

			SetAnimation(visorAnim.id, visorAnim.proc);
		}

		void Astronaut::SetHeadlight(bool active)
		{
			spotLight->Activate(active);
			for (auto& beaconInfo : beacons) beaconInfo.spec.active = active;
		}

		void Astronaut::SetSuit(bool on, bool checkBreath)
		{
			if (on)
			{
				SetEmptyMass(suitMass + astrInfo.mass);
				astrInfo.height = suitHeight;

				auto& slotInfo = vslCargoInfo.slots.front();
				slotInfo.hAttach = GetAttachmentHandle(false, 0);
				slotInfo.holdDir = suitHoldDir;

				VECTOR3 dir, rot;
				GetAttachmentParams(slotInfo.hAttach, slotPos, dir, rot);

				SetMeshVisibilityMode(suitMesh, !enableCockpit && showMeshInCockpit ? MESHVIS_ALWAYS : MESHVIS_EXTERNAL);
				SetMeshVisibilityMode(bodyMesh, MESHVIS_NEVER);
				if (enableCockpit) SetMeshVisibilityMode(cockpitMesh, MESHVIS_COCKPIT);

				suitOn = true;
			}
			else
			{
				if (checkBreath && !InBreathable()) { hudInfo.message = "Error: Outside air not breathable."; hudInfo.timer = 0; return; }

				SetEmptyMass(astrInfo.mass);
				astrInfo.height = bodyHeight;

				auto& slotInfo = vslCargoInfo.slots.front();
				slotInfo.hAttach = GetAttachmentHandle(false, 1);
				slotInfo.holdDir = bodyHoldDir;

				VECTOR3 dir, rot;
				GetAttachmentParams(slotInfo.hAttach, slotPos, dir, rot);

				SetMeshVisibilityMode(suitMesh, MESHVIS_NEVER);
				SetMeshVisibilityMode(bodyMesh, showMeshInCockpit ? MESHVIS_ALWAYS : MESHVIS_EXTERNAL);
				if (enableCockpit) SetMeshVisibilityMode(cockpitMesh, MESHVIS_NEVER);

				if (spotLight) SetHeadlight(false);
				suitOn = false;
			}

			tdVtx[0].pos = { 0, -astrInfo.height, 0.1858 };
			tdVtx[1].pos = { -0.178, -astrInfo.height, -0.1305 };
			tdVtx[2].pos = { 0.178, -astrInfo.height, -0.1305 };
			SetTouchdownPoints(tdVtx.data(), tdVtx.size());

			if (GetFlightStatus())
			{
				VESSELSTATUS2 status = GetVesselStatus(this);
				SetGroundRotation(status, astrInfo.height);
				DefSetStateEx(&status);
			}
		}

		void Astronaut::Kill(bool isLanded)
		{
			if (spotLight) SetHeadlight(false);

			astrInfo.alive = false;

			tdVtx[0].pos = { 0, 0.65, 0.15 };
			tdVtx[1].pos = { -0.18, -astrInfo.height, 0.15 };
			tdVtx[2].pos = { 0.18, -astrInfo.height, 0.15 };
			SetTouchdownPoints(tdVtx.data(), tdVtx.size());

			if (isLanded)
			{
				VESSELSTATUS2 status = GetVesselStatus(this);
				status.status = 1;

				const VECTOR3 frontPos = { 0, 0.15, 0.65 };
				const VECTOR3 rightPos = { -0.18, 0, -astrInfo.height };
				const VECTOR3 leftPos = { 0.18, 0, -astrInfo.height };

				SetGroundRotation(status, frontPos, rightPos, leftPos, 0, 0, false, 0);
				DefSetStateEx(&status);
			}
		}

		void Astronaut::DrawNearHUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Nearest airlock information", 27);
			y += hudInfo.space;

			auto nearAirlock = GetNearestAirlock(searchRange);

			if (!nearAirlock) { skp->Text(x, y, "No vessel in range", 18); goto breathLabel; }

			hudInfo.hVessel = nearAirlock->hVessel;
			hudInfo.vslInfo = HudInfo::VesselInfo();

			hudInfo.vslInfo.arlckIdx = nearAirlock->airlockIdx;
			hudInfo.vslInfo.statIdx = nearAirlock->stationIdx;

			hudInfo.vslInfo.info = GetVslAstrInfo(hudInfo.hVessel);

			buffer = std::format("Vessel name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Station count: {}", hudInfo.vslInfo.info->stations.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			{
				const auto& stationInfo = hudInfo.vslInfo.info->stations.at(hudInfo.vslInfo.statIdx);

				buffer = std::format("Selected station name: {}, {}", stationInfo.name, stationInfo.astrInfo ? "occupied" : "empty");
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;
			}

			buffer = std::format("Airlock name: {}, {}", nearAirlock->airlockInfo.name, nearAirlock->airlockInfo.open ? "open" : "closed");
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Airlock ingress range: {:.1f}m", nearAirlock->airlockInfo.range);
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			DrawVslInfo(x, y, skp, nearAirlock->airlockInfo.pos);

		breathLabel:
			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Nearest breathable information", 30);
			y += hudInfo.space;

			OBJHANDLE hVessel = GetNearestBreathable(searchRange).first;

			if (!hVessel) { skp->Text(x, y, "No breathable in range", 22); goto actionLabel; }

			buffer = std::format("Vessel name: {}", oapiGetVesselInterface(hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = mdlAPI.GetCargoInfoByHandle(hVessel) ? "Type: Cargo" : "Type: Station";
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			VECTOR3 relPos;
			oapiGetGlobalPos(hVessel, &relPos);
			Global2Local(relPos, relPos);
			DrawVslInfo(x, y, skp, relPos);

		actionLabel:
			y += hudInfo.largeSpace;

			skp->Text(x, y, "Nearest action area information", 31);
			y += hudInfo.space;

			auto nearAction = GetNearestAction(searchRange);

			if (!nearAction) { skp->Text(x, y, "No action area in range", 23); return; }

			hudInfo.hVslAction = nearAction->hVessel;
			hudInfo.vslInfo.actionIdx = nearAction->actionIdx;

			buffer = std::format("Vessel name: {}", oapiGetVesselInterface(hudInfo.hVslAction)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Action area name: {}", nearAction->actionInfo.name);
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Action area trigger range: {:.1f}m", nearAction->actionInfo.range);
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			DrawVslInfo(x, y, skp, nearAction->actionInfo.pos);
		}

		void Astronaut::DrawVslHUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Scenario vessel information", 27);
			y += hudInfo.space;

			SetVslMap();

			if (hudInfo.vslMap.empty()) { skp->Text(x, y, "No vessel in range", 18); return; }

			if (hudInfo.hVessel) 
			{
				auto vslIt = hudInfo.vslMap.find(hudInfo.vslIdx);

				if (vslIt == hudInfo.vslMap.end() || vslIt->second != hudInfo.hVessel) hudInfo.hVessel = nullptr;
			}

			if (!hudInfo.hVessel)
			{
				hudInfo.vslIdx = hudInfo.vslMap.begin()->first;
				hudInfo.hVessel = hudInfo.vslMap.begin()->second;
				hudInfo.vslInfo = HudInfo::VesselInfo();

				if (auto resources = mdlAPI.GetStationResources(hudInfo.hVessel))
				{
					hudInfo.vslInfo.resources = std::string();

					if (!resources->empty())
					{
						for (std::string resource : *resources)
						{
							resource[0] = std::toupper(resource[0]);
							*hudInfo.vslInfo.resources += resource + ", ";
						}

						hudInfo.vslInfo.resources->pop_back();
						hudInfo.vslInfo.resources->pop_back();
					}
				}
			}

			buffer = std::format("Vessel count: {}", hudInfo.vslMap.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Selected vessel name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			hudInfo.vslInfo.info = GetVslAstrInfo(hudInfo.hVessel);

			if (!hudInfo.vslInfo.info || hudInfo.vslInfo.info->stations.empty() || hudInfo.vslInfo.info->airlocks.empty())
			{
				VECTOR3 relPos;
				oapiGetGlobalPos(hudInfo.hVessel, &relPos);
				Global2Local(relPos, relPos);
				DrawVslInfo(x, y, skp, relPos);
			}
			else
			{
				buffer = std::format("Station count: {}", hudInfo.vslInfo.info->stations.size());
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				const auto& stationInfo = hudInfo.vslInfo.info->stations.at(hudInfo.vslInfo.statIdx);

				buffer = std::format("Selected station name: {}, {}", stationInfo.name, stationInfo.astrInfo ? "occupied" : "empty");
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				buffer = std::format("Airlock count: {}", hudInfo.vslInfo.info->airlocks.size());
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				const auto& airlockInfo = hudInfo.vslInfo.info->airlocks.at(hudInfo.vslInfo.arlckIdx);

				buffer = std::format("Selected airlock name: {}, {}", airlockInfo.name, airlockInfo.open ? "open" : "closed");
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				buffer = std::format("Selected airlock ingress range: {:.1f}m", airlockInfo.range);
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				VECTOR3 airlockPos;
				oapiLocalToGlobal(hudInfo.hVessel, &airlockInfo.pos, &airlockPos);
				Global2Local(airlockPos, airlockPos);
				DrawVslInfo(x, y, skp, airlockPos);
			}

			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			if (hudInfo.vslInfo.info && hudInfo.vslInfo.info->actionAreas.size())
			{
				hudInfo.hVslAction = hudInfo.hVessel;

				buffer = std::format("Action area count: {}", hudInfo.vslInfo.info->actionAreas.size());
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				const auto& actionInfo = hudInfo.vslInfo.info->actionAreas.at(hudInfo.vslInfo.actionIdx);

				buffer = std::format("Selected action area name: {}, {}", actionInfo.name, actionInfo.enabled ? "enabled" : "disabled");
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				buffer = std::format("Selected action area trigger range: {:.1f}m", actionInfo.range);
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				VECTOR3 actionPos;
				oapiLocalToGlobal(hudInfo.hVessel, &actionInfo.pos, &actionPos);
				Global2Local(actionPos, actionPos);
				DrawVslInfo(x, y, skp, actionPos);

				y += hudInfo.largeSpace;
			}

			if (hudInfo.vslInfo.resources)
			{
				buffer = std::format("Selected resource to drain: {}", hudInfo.drainFuel ? "Fuel" : "Oxygen");
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				if (hudInfo.vslInfo.resources->empty()) buffer = "Selected vessel resources: All";
				else buffer = std::format("Selected vessel resources: {}", *hudInfo.vslInfo.resources);

				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		void Astronaut::DrawAstrHUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Scenario astronaut information", 30);
			y += hudInfo.space;

			SetAstrMap();

			if (hudInfo.astrMap.empty()) { skp->Text(x, y, "No astronaut in range", 21); return; }

			buffer = std::format("Astronaut count: {}", hudInfo.astrMap.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			if (hudInfo.hVessel)
			{
				auto astrIt = hudInfo.astrMap.find(hudInfo.vslIdx);

				if (astrIt == hudInfo.astrMap.end() || astrIt->second != hudInfo.hVessel) hudInfo.hVessel = nullptr;
			}

			if (!hudInfo.hVessel)
			{
				hudInfo.vslIdx = hudInfo.astrMap.begin()->first;
				hudInfo.hVessel = hudInfo.astrMap.begin()->second;
			}

			buffer = std::format("Selected astronaut name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			VECTOR3 relPos;
			oapiGetGlobalPos(hudInfo.hVessel, &relPos);
			Global2Local(relPos, relPos);
			DrawVslInfo(x, y, skp, relPos);

			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Selected astronaut information", 30);
			y += hudInfo.space;

			mdlAPI.DrawAstrInfo(*GetAstrInfoByIndex(hudInfo.vslIdx).second, skp, x, y, hudInfo.space);
		}

		void Astronaut::DrawCargoHUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Scenario cargo information", 26);
			y += hudInfo.space;

			SetCargoMap();

			bool cargoRes{};

			if (hudInfo.cargoMap.empty()) skp->Text(x, y, "No free cargo in range", 22);

			else
			{
				buffer = std::format("Cargo count: {}", hudInfo.cargoMap.size());
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				if (hudInfo.hVessel)
				{
					auto cargoIt = hudInfo.cargoMap.find(hudInfo.vslIdx);

					if (cargoIt == hudInfo.cargoMap.end() || cargoIt->second != hudInfo.hVessel) hudInfo.hVessel = nullptr;
				}

				if (!hudInfo.hVessel)
				{
					hudInfo.vslIdx = hudInfo.cargoMap.begin()->first;
					hudInfo.hVessel = hudInfo.cargoMap.begin()->second;
				}

				auto cargoInfo = mdlAPI.GetCargoInfoByIndex(hudInfo.vslIdx);

				DrawCargoInfo(x, y, skp, cargoInfo, true);
				y += hudInfo.space;

				oapiLocalToGlobal(cargoInfo.handle, &cargoInfo.attachPos, &cargoInfo.attachPos);
				Global2Local(cargoInfo.attachPos, cargoInfo.attachPos);
				DrawVslInfo(x, y, skp, cargoInfo.attachPos - slotPos);

				cargoRes = cargoInfo.resource.has_value();
			}

			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			if (cargoRes)
			{
				buffer = std::format("Selected resource to drain: {}", hudInfo.drainFuel ? "Fuel" : "Oxygen");
				skp->Text(x, y, buffer.c_str(), buffer.size());				
				y += hudInfo.largeSpace;
			}

			if (const auto& cargoInfo = vslCargoInfo.slots.front().cargoInfo)
			{
				skp->Text(x, y, "Grappled cargo information", 26);
				y += hudInfo.space;

				mdlAPI.DrawCargoInfo(*cargoInfo, skp, x, y, hudInfo.space);
			}
		}

		void Astronaut::DrawShort1HUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "General shortcuts", 17);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + M: Cycle custom HUD modes", 31);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + S: Toggle suit", 20);
			y += hudInfo.space;

			if (enableCockpit) { skp->Text(x, y, "Alt + V: Toggle visor", 21); y += hudInfo.space; }

			if (spotLight) { skp->Text(x, y, "Alt + L: Toggle headlight", 25); y += hudInfo.space; }
			y += hudInfo.smallSpace;

			skp->Text(x, y, "(Ctrl) Numpad 8/2: Move forward/backward (slowly)", 49);
			y += hudInfo.space;

			skp->Text(x, y, "(Ctrl) Numpad 6/4: Turn right/left (slowly)", 43);
			y += hudInfo.space;

			skp->Text(x, y, "(Ctrl) Numpad 3/1: Move right/left (slowly)", 43);

			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Vessel HUD shortcuts", 20);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 6/4: Select next/previous vessel", 45);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 3/1: Select next/previous airlock", 46);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 8/2: Select next/previous station", 46);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 9/7: Select next/previous action area", 50);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + I: Ingress into selected station", 38);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + A: Trigger selected action area", 37);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + T: Select resource to drain", 33);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + F = Drain resource from selected station", 46);
		}

		void Astronaut::DrawShort2HUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Nearest HUD shortcuts", 21);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 8/2: Select next/previous station", 46);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + I: Ingress into selected station", 38);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + A: Trigger nearest action area", 36);
			y += hudInfo.largeSpace;

			skp->Text(x, y, "Astronaut HUD shortcuts", 23);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 6/4: Select next/previous astronaut", 48);

			x = hudInfo.rightX;
			y = hudInfo.startY;
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Cargo HUD shortcuts", 19);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + Numpad 6/4: Select next/previous cargo", 44);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + T: Select resource to drain", 33);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + R = Release grappled cargo", 32);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + D = Delete grappled cargo", 31);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + G = Grapple nearest cargo", 31);
			y += hudInfo.space;

			skp->Text(x, y, "Ctrl + Alt + G = Grapple selected cargo", 39);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + P = Pack nearest packable cargo", 37);
			y += hudInfo.space;

			skp->Text(x, y, "Ctrl + Alt + P = Pack selected cargo", 36);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + U = Unpack nearest unpackable cargo", 41);
			y += hudInfo.space;

			skp->Text(x, y, "Ctrl + Alt + U = Unpack selected cargo", 38);
			y += hudInfo.space;

			skp->Text(x, y, "Alt + F = Drain resource from nearest cargo", 43);
			y += hudInfo.space;

			skp->Text(x, y, "Ctrl + Alt + F = Drain resource from selected cargo", 51);
		}

		void Astronaut::DrawVslInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos)
		{
			buffer = std::format("Distance: {:.1f}m", length(relPos));
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			double relYaw = atan2(relPos.x, relPos.z) * DEG;

			if (GetFlightStatus())
			{
				buffer = std::format("Relative heading: {:.1f}", relYaw);
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}

			else
			{
				buffer = std::format("Relative yaw: {:.1f}", relYaw);
				skp->Text(x, y, buffer.c_str(), buffer.size());
				y += hudInfo.space;

				double relPitch = atan2(relPos.y, sqrt(relPos.z * relPos.z + relPos.x * relPos.x)) * DEG;
				buffer = std::format("Relative pitch: {:.1f}", relPitch);
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		void Astronaut::DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const UACS::CargoInfo& cargoInfo, bool extraInfo)
		{
			if (extraInfo) buffer = std::format("Selected cargo name: {}", oapiGetVesselInterface(cargoInfo.handle)->GetName());
			else buffer = std::format("Name: {}", oapiGetVesselInterface(cargoInfo.handle)->GetName());

			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			buffer = std::format("Mass: {:g}kg", oapiGetMass(cargoInfo.handle));
			skp->Text(x, y, buffer.c_str(), buffer.size());
			y += hudInfo.space;

			switch (cargoInfo.type)
			{
			case UACS::STATIC:
				skp->Text(x, y, "Type: Static", 12);
				break;

			case UACS::UNPACKABLE:
				if (cargoInfo.unpackOnly) skp->Text(x, y, "Type: Unpackable only", 21);
				else skp->Text(x, y, "Type: Unpackable", 16);

				if (extraInfo)
				{
					y += hudInfo.space;
					buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}

				break;
			}

			if (cargoInfo.resource)
			{
				y += hudInfo.space;

				buffer = *cargoInfo.resource;
				buffer[0] = std::toupper(buffer[0]);
				buffer.insert(0, "Resource: ");

				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		void Astronaut::SetVslMap()
		{
			hudInfo.vslMap.clear();

			for (size_t idx{}; idx < oapiGetVesselCount(); ++idx)
			{
				if (OBJHANDLE hVessel = oapiGetVesselByIndex(idx); !mdlAPI.GetAstrInfoByHandle(hVessel) && !mdlAPI.GetCargoInfoByHandle(hVessel))
				{
					VECTOR3 pos; GetRelativePos(hVessel, pos);
					if (length(pos) <= searchRange) hudInfo.vslMap.emplace(idx, hVessel);
				}
			}
		}

		void Astronaut::SetAstrMap()
		{
			hudInfo.astrMap.clear();

			for (size_t idx{}; idx < GetScnAstrCount(); ++idx)
			{
				if (OBJHANDLE hAstr = GetAstrInfoByIndex(idx).first; hAstr != GetHandle())
				{
					VECTOR3 pos; GetRelativePos(hAstr, pos);
					if (length(pos) <= searchRange) hudInfo.astrMap.emplace(idx, hAstr);
				}
			}
		}

		void Astronaut::SetCargoMap()
		{
			hudInfo.cargoMap.clear();

			for (size_t idx{}; idx < mdlAPI.GetScnCargoCount(); ++idx)
			{
				if (UACS::CargoInfo cargoInfo = mdlAPI.GetCargoInfoByIndex(idx); !cargoInfo.attached)
				{
					VECTOR3 pos; GetRelativePos(cargoInfo.handle, pos);
					if (length(pos) <= searchRange) hudInfo.cargoMap.emplace(idx, cargoInfo.handle);
				}
			}
		}

		void Astronaut::SetMapIdx(const std::map<size_t, OBJHANDLE>& map, bool increase)
		{
			if (map.empty()) return;

			auto it = map.find(hudInfo.vslIdx);

			if (it == map.end()) return;

			if (increase)
			{
				++it;
				it = (it == map.end()) ? map.begin() : it;

				hudInfo.vslIdx = it->first;
				hudInfo.hVessel = it->second;
			}
			else
			{
				it = (it == map.begin()) ? map.end() : it;
				--it;

				hudInfo.vslIdx = it->first;
				hudInfo.hVessel = it->second;
			}

			hudInfo.vslInfo = HudInfo::VesselInfo();
		}
	}
}