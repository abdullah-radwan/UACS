#include "Astronaut.h"
#include "..\..\Common.h"
#include <DrawAPI.h>

#include <format>
#include <sstream>
#include <array>

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
				if (!oapiReadItem_float(hConfig, "SuitMass", suitMass))
					oapiWriteLog("UACS astronaut warning: Couldn't read the suit mass, will use the default mass (60 kg)");

				if (oapiReadItem_float(hConfig, "MaxNearestRange", maxNearestRange)) maxNearestRange *= 1e3;
				else oapiWriteLog("UACS astronaut warning: Couldn't read the max nearest range, will use the default setting (60 km)");

				if (!oapiReadItem_bool(hConfig, "RealMode", realMode))
					oapiWriteLog("UACS astronaut warning: Couldn't read the real mode, will use the default setting (FALSE)");

				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
			}
			else oapiWriteLog("UACS astronaut warning: Couldn't load the configurations file, will use the default configurations");
		}

		Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : API::Astronaut(hVessel, fModel), pVslAPI(API::Vessel::CreateInstance(this, nullptr, &vslCargoInfo))
		{
			if (!configLoaded) LoadConfig();

			astrInfo.fuelLvl = astrInfo.oxyLvl = 1;
			astrInfo.alive = true;
			astrInfo.className = "Astronaut";

			lonSpeed.maxLimit = 10;
			lonSpeed.minLimit = latSpeed.minLimit = -(latSpeed.maxLimit = 1);
			steerAngle.minLimit = -(steerAngle.maxLimit = 45 * RAD);

			hudInfo.deadFont = oapiCreateFont(50, true, "Courier New");

			vslCargoInfo.astrMode = true;
			vslCargoInfo.breathableRange = maxNearestRange;
			vslCargoInfo.grappleRange = 5;
			vslCargoInfo.relVel = 0;
			vslCargoInfo.relRowCount = 1;
			vslCargoInfo.packRange = 5;
			vslCargoInfo.drainRange = 5;

			SetDefaultValues();

			hudInfo.message = std::format("UACS version: {}", GetUACSVersion());
		}

		Astronaut::~Astronaut() { oapiReleaseFont(hudInfo.deadFont); }

		void Astronaut::clbkSetClassCaps(FILEHANDLE cfg)
		{
			char cBuffer[512];

			if (!oapiReadItem_string(cfg, "Name", cBuffer)) WarnAndTerminate("name", GetClassNameA(), "astronaut");
			astrInfo.name = cBuffer;

			if (!oapiReadItem_string(cfg, "Role", cBuffer)) WarnAndTerminate("role", GetClassNameA(), "astronaut");
			astrInfo.role = cBuffer;

			if (!oapiReadItem_string(cfg, "SuitMesh", cBuffer)) WarnAndTerminate("suit mesh", GetClassNameA(), "astronaut");
			suitMesh = AddMesh(cBuffer);

			if (!oapiReadItem_string(cfg, "Mesh", cBuffer)) WarnAndTerminate("mesh", GetClassNameA(), "astronaut");
			astrMesh = AddMesh(cBuffer);

			if (!oapiReadItem_float(cfg, "Mass", astrInfo.mass)) WarnAndTerminate("mass", GetClassNameA(), "astronaut");

			if (!oapiReadItem_float(cfg, "Height", astrInfo.height)) WarnAndTerminate("height", GetClassNameA(), "astronaut");

			for (size_t index{ 1 }; ; ++index)
			{
				VECTOR3 pos, dir;
				if (!oapiReadItem_vec(cfg, std::format("Headlight{}Pos", index).data(), pos) || !oapiReadItem_vec(cfg, std::format("Headlight{}Dir", index).data(), dir)) break;

				HeadlightInfo& hlInfo = headlights.emplace_front();
				hlInfo.spotInfo.pos = hlInfo.beaconInfo.pos = pos;
				hlInfo.spotInfo.dir = dir;

				AddBeacon(&hlInfo.beaconInfo.spec);

				hlInfo.spotLight = static_cast<SpotLight*>(AddSpotLight(hlInfo.spotInfo.pos, hlInfo.spotInfo.dir, hlInfo.spotInfo.range,
					hlInfo.spotInfo.att0, hlInfo.spotInfo.att1, hlInfo.spotInfo.att2, hlInfo.spotInfo.umbra, hlInfo.spotInfo.penumbra,
					hlInfo.spotInfo.diffuse, hlInfo.spotInfo.specular, hlInfo.spotInfo.ambient));

				hlInfo.spotLight->Activate(false);
			}

			std::array<TOUCHDOWNVTX, 13> tdVtx
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
		}

		void Astronaut::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			char* line;

			while (oapiReadScenario_nextline(scn, line))
			{
				std::istringstream ss;
				ss.str(line);
				std::string data;

				if (ss >> data >> std::ws)
				{
					if (data == "NAME") { std::getline(ss, data); astrInfo.name = data; }

					else if (data == "ROLE") ss >> astrInfo.role;

					else if (data == "MASS") ss >> astrInfo.mass;

					else if (data == "ALIVE") ss >> astrInfo.alive;

					else if (data == "SUIT_ON") ss >> suitOn;

					else if (data == "HUD_MODE") ss >> hudInfo.mode;

					else if (!headlights.empty() && data == "HEADLIGHT") { bool active; ss >> active; SetHeadlight(active); }

					else ParseScenarioLineEx(line, status);
				}

				else ParseScenarioLineEx(line, status);
			}
		}

		void Astronaut::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);

			oapiWriteScenario_string(scn, "NAME", astrInfo.name.data());
			oapiWriteScenario_string(scn, "ROLE", astrInfo.role.data());
			oapiWriteScenario_float(scn, "MASS", astrInfo.mass);

			oapiWriteScenario_int(scn, "ALIVE", astrInfo.alive);
			oapiWriteScenario_int(scn, "SUIT_ON", suitOn);
			oapiWriteScenario_int(scn, "HUD_MODE", hudInfo.mode);
			if (!headlights.empty()) oapiWriteScenario_int(scn, "HEADLIGHT", headlights.front().beaconInfo.spec.active);
		}

		void Astronaut::clbkPostCreation()
		{
			SetEmptyMass(suitMass + astrInfo.mass);

			InitPropellant();

			vslCargoInfo.slots.emplace_back(GetAttachmentHandle(false, 0));

			SetSuit(suitOn, false);

			pVslAPI->clbkPostCreation();

			if (!astrInfo.alive) Kill();
		}

		void Astronaut::clbkSetAstrInfo(const API::AstrInfo& astrInfo)
		{
			this->astrInfo.name = astrInfo.name;
			this->astrInfo.role = astrInfo.role;
			this->astrInfo.mass = astrInfo.mass;
			this->astrInfo.fuelLvl = astrInfo.fuelLvl;
			this->astrInfo.oxyLvl = astrInfo.oxyLvl;
			this->astrInfo.alive = astrInfo.alive;

			SetEmptyMass(suitMass + astrInfo.mass);
			SetPropellantMass(hFuel, astrInfo.fuelLvl * GetPropellantMaxMass(hFuel));
			SetPropellantMass(hOxy, astrInfo.oxyLvl * GetPropellantMaxMass(hOxy));
		}

		const API::AstrInfo* Astronaut::clbkGetAstrInfo()
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
				if (key == OAPI_KEY_NUMPAD6 || key == OAPI_KEY_NUMPAD4)
				{
					if (oapiGetVesselCount() - pVslAPI->GetScnCargoCount() > 1)
					{
						do
						{
							if (key == OAPI_KEY_NUMPAD6) hudInfo.vslIdx + 1 < oapiGetVesselCount() ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
							else hudInfo.vslIdx > 0 ? --hudInfo.vslIdx : hudInfo.vslIdx = oapiGetVesselCount() - 1;

							hudInfo.hVessel = oapiGetVesselByIndex(hudInfo.vslIdx);
						} while (hudInfo.hVessel == GetHandle() || pVslAPI->GetCargoInfoByHandle(hudInfo.hVessel));

						hudInfo.vslInfo = HudInfo::VesselInfo();
					}

					return 1;
				}

				else if (key == OAPI_KEY_A)
				{
					hudInfo.vslInfo.arlckIdx + 1 < hudInfo.vslInfo.info->airlocks.size() ? ++hudInfo.vslInfo.arlckIdx : hudInfo.vslInfo.arlckIdx = 0;
					return 1;
				}
				[[fallthrough]];

			case HUD_NST:
				if (key == OAPI_KEY_NUMPAD8)
				{
					hudInfo.vslInfo.statIdx + 1 < hudInfo.vslInfo.info->stations.size() ? ++hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = 0;
					return 1;
				}

				else if (key == OAPI_KEY_NUMPAD2)
				{
					hudInfo.vslInfo.statIdx > 0 ? --hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = hudInfo.vslInfo.info->stations.size() - 1;
					return 1;
				}

				break;

			case HUD_AST:
			{
				switch (key)
				{
				case OAPI_KEY_NUMPAD6:
				case OAPI_KEY_NUMPAD4:
				{
					if (pVslAPI->GetScnAstrCount() - 1)
					{
						do
						{
							if (key == OAPI_KEY_NUMPAD6) hudInfo.vslIdx < pVslAPI->GetScnAstrCount() - 1 ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
							else hudInfo.vslIdx > 0 ? --hudInfo.vslIdx : hudInfo.vslIdx = pVslAPI->GetScnAstrCount() - 2;

							hudInfo.hVessel = pVslAPI->GetAstrInfoByIndex(hudInfo.vslIdx).first;
						} while (hudInfo.hVessel == GetHandle());
					}

					return 1;
				}

				case OAPI_KEY_NUMPAD8:
					hudInfo.availIdx + 1 < pVslAPI->GetAvailCargoCount() ? ++hudInfo.availIdx : hudInfo.availIdx = 0;
					return 1;

				case OAPI_KEY_NUMPAD2:
					hudInfo.availIdx > 0 ? --hudInfo.availIdx : hudInfo.availIdx = pVslAPI->GetAvailCargoCount() - 1;
					return 1;
				}

				break;
			}

			case HUD_CRG:
			{
				OBJHANDLE hCargo = KEYMOD_CONTROL(kstate) ? hudInfo.hVessel : nullptr;

				switch (key)
				{
				case OAPI_KEY_NUMPAD6:
				case OAPI_KEY_NUMPAD4:
				{
					if (pVslAPI->GetScnCargoCount() && GetFirstVslIdx())
					{
						do
						{
							if (key == OAPI_KEY_NUMPAD6) hudInfo.vslIdx + 1 < pVslAPI->GetScnCargoCount() ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
							else hudInfo.vslIdx > 0 ? --hudInfo.vslIdx : hudInfo.vslIdx = pVslAPI->GetScnCargoCount() - 1;
						} while (pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).attached);

						hudInfo.hVessel = pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).handle;
					}

					return 1;
				}

				case OAPI_KEY_NUMPAD8:
					hudInfo.availIdx + 1 < pVslAPI->GetAvailCargoCount() ? ++hudInfo.availIdx : hudInfo.availIdx = 0;
					return 1;

				case OAPI_KEY_NUMPAD2:
					hudInfo.availIdx > 0 ? --hudInfo.availIdx : hudInfo.availIdx = pVslAPI->GetAvailCargoCount() - 1;
					return 1;

				case OAPI_KEY_T:
					hudInfo.drainFuel = !hudInfo.drainFuel;
					return 1;

				case OAPI_KEY_A:
					switch (pVslAPI->AddCargo(hudInfo.availIdx))
					{
					case API::GRPL_SUCCED:
						hudInfo.modeMsg = "Success: Selected available cargo added.";
						break;

					case API::GRPL_SLT_OCCP:
						hudInfo.modeMsg = "Error: A cargo is already grappled.";
						break;

					case API::GRPL_FAIL:
						hudInfo.modeMsg = "Error: Addition failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;
				
				case OAPI_KEY_D:
					switch (pVslAPI->DeleteCargo())
					{
					case API::RLES_SUCCED:
						hudInfo.modeMsg = "Success: Grappled cargo deleted.";
						break;

					case API::RLES_SLT_EMPTY:
						hudInfo.modeMsg = "Error: No cargo grappled.";
						break;

					case API::RLES_FAIL:
						hudInfo.modeMsg = "Error: Deletion failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_G:
					switch (pVslAPI->GrappleCargo(hCargo))
					{
					case API::GRPL_SUCCED:
						if (hCargo) hudInfo.modeMsg = "Success: Selected cargo grappled.";
						else hudInfo.modeMsg = "Success: Nearest cargo grappled.";
						break;

					case API::GRPL_SLT_OCCP:
						hudInfo.modeMsg = "Error: A cargo is already grappled.";
						break;

					case API::GRPL_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Error: Selected cargo out of range.";
						else hudInfo.modeMsg = "Error: No cargo in range.";
						break;

					case API::GRPL_FAIL:
						hudInfo.modeMsg = "Error: Grapple failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_R:
					switch (pVslAPI->ReleaseCargo())
					{
					case API::RLES_SUCCED:
						hudInfo.modeMsg = "Success: Grappled cargo released.";
						break;

					case API::RLES_SLT_EMPTY:
						hudInfo.modeMsg = "Error: No cargo grappled";
						break;

					case API::RLES_NO_EMPTY_POS:
						hudInfo.modeMsg = "Error: No empty position nearby.";
						break;

					case API::RLES_FAIL:
						hudInfo.modeMsg = "Error: Release failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_P:
					switch (pVslAPI->PackCargo(hCargo))
					{
					case API::PACK_SUCCED:
						if (hCargo) hudInfo.modeMsg = "Success: Selected cargo packed.";
						else hudInfo.modeMsg = "Success: Nearest cargo packed.";
						break;

					case API::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Error: Selected cargo out of range.";
						else hudInfo.modeMsg = "Error: No packable cargo in range.";
						break;

					case API::PACK_CRG_PCKD:
						hudInfo.modeMsg = "Error: Selected cargo already packed.";
						break;

					case API::PACK_CRG_NOT_PCKABL:
						hudInfo.modeMsg = "Error: Selected cargo not packable.";
						break;

					case API::PACK_FAIL:
						hudInfo.modeMsg = "Error: Packing failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_U:
					switch (pVslAPI->UnpackCargo(hCargo))
					{
					case API::PACK_SUCCED:
						if (hCargo) hudInfo.modeMsg = "Success: Selected cargo unpacked.";
						else hudInfo.modeMsg = "Success: Nearest cargo unpacked.";
						break;

					case API::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Error: Selected cargo out of range.";
						else hudInfo.modeMsg = "Error: No unpackable cargo in range.";
						break;

					case API::PACK_CRG_UNPCKD:
						hudInfo.modeMsg = "Error: Selected cargo already unpacked.";
						break;

					case API::PACK_CRG_NOT_PCKABL:
						hudInfo.modeMsg = "Error: Selected cargo not unpackable.";
						break;

					case API::PACK_FAIL:
						hudInfo.modeMsg = "Error: Unpacking failed.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_F:
				{
					const char* resource;
					double reqMass;

					if (hudInfo.drainFuel) { resource = "fuel"; reqMass = GetPropellantMaxMass(hFuel) - GetPropellantMass(hFuel); }
					else { resource = "oxygen"; reqMass = GetPropellantMaxMass(hOxy) - GetPropellantMass(hOxy); }

					if (reqMass)
					{
						auto drainInfo = pVslAPI->DrainGrappledResource(resource, reqMass);

						if (drainInfo.first != API::DRIN_SUCCED) drainInfo = pVslAPI->DrainUngrappledResource(resource, reqMass, hCargo);

						switch (drainInfo.first)
						{
						case UACS::API::DRIN_SUCCED:
							if (hudInfo.drainFuel) SetPropellantMass(hFuel, GetPropellantMass(hFuel) + drainInfo.second);
							else SetPropellantMass(hOxy, GetPropellantMass(hOxy) + drainInfo.second);

							hudInfo.modeMsg = std::format("Success: {:g} kg {} drained.", drainInfo.second, resource);
							break;

						case UACS::API::DRIN_NOT_IN_RNG:
							if (hCargo) hudInfo.modeMsg = "Error: Selected cargo out of range.";
							else hudInfo.modeMsg = "Error: No resource cargo in range.";
							break;

						case UACS::API::DRIN_NOT_RES:
							hudInfo.modeMsg = "Error: Selected cargo not a resource.";
							break;

						case UACS::API::DRIN_RES_NOMATCH:
							hudInfo.modeMsg = "Error: Selected cargo resource doesn't match.";
							break;

						case UACS::API::DRIN_FAIL:
							hudInfo.modeMsg = "Error: Drainage failed.";
							break;
						}
					}
					else hudInfo.modeMsg = "Error: Selected resource full.";

					hudInfo.modeTimer = 0;
					return 1;
				}
				}

				break;
			}
			}

			switch (key)
			{
			case OAPI_KEY_L:
				if (!headlights.empty() && suitOn) SetHeadlight(!headlights.front().beaconInfo.spec.active);
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

			case OAPI_KEY_M:
				hudInfo.mode < 6 ? ++hudInfo.mode : hudInfo.mode = 0;

				hudInfo.vslIdx = 0;
				hudInfo.hVessel = nullptr;
				hudInfo.vslInfo = HudInfo::VesselInfo();
				return 1;

			case OAPI_KEY_I:
			{
				if (hudInfo.mode != HUD_NST && hudInfo.mode != HUD_VSL) return 0;

				if (vslCargoInfo.slots.front().cargoInfo)
				{
					hudInfo.message = "Error: A cargo is grappled.";
					hudInfo.timer = 0;
					return 1;
				}

				UACS::API::IngressResult result;

				const bool selected = KEYMOD_CONTROL(kstate) ? hudInfo.hVessel && hudInfo.vslInfo.info->airlocks.size() && hudInfo.vslInfo.info->stations.size() : false;

				if (selected) result = Ingress(hudInfo.hVessel, hudInfo.vslInfo.arlckIdx, hudInfo.vslInfo.statIdx);
				else result = Ingress();

				switch (result)
				{
				case API::INGRS_SUCCED:
					return 1;

				case API::INGRS_NOT_IN_RNG:
					if (selected) hudInfo.message = "Error: Selected airlock out of range.";
					else hudInfo.message = "Error: No airlock in range.";
					break;

				case API::INGRS_ARLCK_UNDEF:
					hudInfo.message = "Error: Selected vessel has no airlocks.";
					break;

				case API::INGRS_ARLCK_CLSD:
					hudInfo.message = "Error: Selected airlock closed.";
					break;

				case API::INGRS_STN_UNDEF:
					hudInfo.message = "Error: Selected vessel has no stations.";
					break;

				case API::INGRS_STN_OCCP:
					hudInfo.message = "Error: Selected station occupied.";
					break;

				case API::INGRS_FAIL:
					hudInfo.message = "Error: Ingress failed.";
					break;
				}

				hudInfo.timer = 0;
				return 1;
			}

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

			if (realMode && (KEYDOWN(kstate, OAPI_KEY_NUMPAD6) || KEYDOWN(kstate, OAPI_KEY_NUMPAD4)))
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
			if (!astrInfo.alive) { SetAttitudeMode(RCS_NONE); return; }

			SetOxygenConsumption(simdt);

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
			}

			else
			{
				SetLandedStatus();

				lonSpeed.value = latSpeed.value = steerAngle.value = 0;
			}

			if (totalRunDist && lonSpeed.value <= 1.55) totalRunDist = max(totalRunDist - (5 * simdt), 0);

			if (hudInfo.timer < 5) hudInfo.timer += simdt;

			if (hudInfo.modeTimer < 5) hudInfo.modeTimer += simdt;
		}

		bool Astronaut::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			int x = 5;
			int y = int(0.215 * hps->H);

			if (!astrInfo.alive)
			{
				skp->SetTextColor(0x0000ff);
				skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
				skp->SetFont(hudInfo.deadFont);

				skp->Text(hps->CX, hps->CY, "DEAD", 4);
				return true;
			}

			VESSEL4::clbkDrawHUD(mode, hps, skp);

			if (suitOn)
			{
				const double time = GetPropellantMass(hOxy) / (consumptionRate * 3600);
				const int hours = int(time);
				const double minutesRemainder = (time - hours) * 60;
				const int minutes = int(minutesRemainder);
				const int seconds = int((minutesRemainder - minutes) * 60);

				std::string buffer = std::format("Oxygen Level: {:.1f}% Time: {:02d}:{:02d}:{:02d}", astrInfo.oxyLvl * 100, hours, minutes, seconds);

				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 20;
			}

			if (hudInfo.timer < 5) { skp->Text(x, y, hudInfo.message.c_str(), hudInfo.message.size()); y += 30; }
			else if (suitOn) y += 10;

			switch (hudInfo.mode)
			{
			case HUD_NST:
				DrawNearHUD(x, y, hps, skp);
				break;

			case HUD_VSL:
				DrawVslHUD(x, y, skp);
				break;

			case HUD_AST:
				DrawAstrHUD(x, y, hps, skp);
				break;

			case HUD_CRG:
				DrawCargoHUD(x, y, hps, skp);
				break;

			case HUD_SRT1:
				DrawShort1HUD(x, y, hps, skp);
				break;

			case HUD_SRT2:
				DrawShort2HUD(x, y, hps, skp);
				break;
			}

			return true;
		}

		void Astronaut::InitPropellant()
		{
			hFuel = GetPropellantHandleByIndex(0);
			hOxy = GetPropellantHandleByIndex(1);

			THRUSTER_HANDLE thRCS[14], thGroup[4];

			thRCS[0] = CreateThruster(_V(1.23, 0, 1.23), _V(0, 1, 0), 1, hFuel, 1e3);
			thRCS[1] = CreateThruster(_V(1.23, 0, 1.23), _V(0, -1, 0), 1, hFuel, 1e3);
			thRCS[2] = CreateThruster(_V(-1.23, 0, 1.23), _V(0, 1, 0), 1, hFuel, 1e3);
			thRCS[3] = CreateThruster(_V(-1.23, 0, 1.23), _V(0, -1, 0), 1, hFuel, 1e3);
			thRCS[4] = CreateThruster(_V(1.23, 0, -1.23), _V(0, 1, 0), 1, hFuel, 1e3);
			thRCS[5] = CreateThruster(_V(1.23, 0, -1.23), _V(0, -1, 0), 1, hFuel, 1e3);
			thRCS[6] = CreateThruster(_V(-1.23, 0, -1.23), _V(0, 1, 0), 1, hFuel, 1e3);
			thRCS[7] = CreateThruster(_V(-1.23, 0, -1.23), _V(0, -1, 0), 1, hFuel, 1e3);
			thRCS[8] = CreateThruster(_V(1.23, 0, 1.23), _V(-1, 0, 0), 1, hFuel, 1e3);
			thRCS[9] = CreateThruster(_V(-1.23, 0, 1.23), _V(1, 0, 0), 1, hFuel, 1e3);
			thRCS[10] = CreateThruster(_V(1.23, 0, -1.23), _V(-1, 0, 0), 1, hFuel, 1e3);
			thRCS[11] = CreateThruster(_V(-1.23, 0, -1.23), _V(1, 0, 0), 1, hFuel, 1e3);
			thRCS[12] = CreateThruster(_V(0, 0, -1.23), _V(0, 0, 1), 1, hFuel, 1e3);
			thRCS[13] = CreateThruster(_V(0, 0, 1.23), _V(0, 0, -1), 1, hFuel, 1e3);

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

					if (InBreathableArea(false)) { pVslAPI->ReleaseCargo(0); SetSuit(false, false); }
					else Kill();
				}
			}
			else if (!InBreathableArea(false)) Kill();
		}

		void Astronaut::SetLandedStatus()
		{
			if (GroundContact())
			{
				VECTOR3 angAcc; GetAngularAcc(angAcc);

				if (length(angAcc) < 0.5)
				{
					VESSELSTATUS2 status = GetVesselStatus(this);

					status.status = 1;
					SetGroundRotation(status, astrInfo.height);
					DefSetStateEx(&status);
				}
			}
		}

		void Astronaut::SetSurfaceRef()
		{
			surfInfo.ref = GetSurfaceRef();

			const double radius = oapiGetSize(surfInfo.ref);

			surfInfo.gravityAccel = GGRAV * oapiGetMass(surfInfo.ref) / (radius * radius);

			surfInfo.steerRatio = 1.118 * surfInfo.gravityAccel;

			if (realMode)
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

			if (steerAngle.value)
			{
				status.surf_hdg += steerAngle.value * simdt;

				if (status.surf_hdg > PI2) status.surf_hdg -= PI2;
				else if (status.surf_hdg < 0) status.surf_hdg += PI2;
			}

			double lngOffset{};
			double latOffset{};

			if (lonSpeed.value)
			{
				lngOffset = lonSpeed.value * simdt * sin(status.surf_hdg);
				latOffset = lonSpeed.value * simdt * cos(status.surf_hdg);

				if (realMode && lonSpeed.value > 1.55)
				{
					totalRunDist += abs(lonSpeed.value) * simdt;
					lonSpeed.maxLimit = totalRunDist / ((suitOn ? 20 : 15) * pow(totalRunDist / 100, 1.06));
				}
				else lonSpeed.maxLimit = 10;
			}

			else if (latSpeed.value)
			{
				double angle = status.surf_hdg + PI05;
				if (angle > PI2) angle -= PI2;

				lngOffset = latSpeed.value * simdt * sin(angle);
				latOffset = latSpeed.value * simdt * cos(angle);
			}

			if (lngOffset || latOffset) SetGroundRotation(status, lngOffset, latOffset, astrInfo.height);
			else SetGroundRotation(status, astrInfo.height);

			DefSetStateEx(&status);
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

		void Astronaut::SetHeadlight(bool active)
		{
			for (auto& hlInfo : headlights)
			{
				hlInfo.beaconInfo.spec.active = active;
				hlInfo.spotLight->Activate(active);
			}
		}

		void Astronaut::SetSuit(bool on, bool checkBreath)
		{
			if (on)
			{
				vslCargoInfo.slots.front().hAttach = GetAttachmentHandle(false, 0);

				SetMeshVisibilityMode(suitMesh, MESHVIS_ALWAYS);
				SetMeshVisibilityMode(astrMesh, MESHVIS_NEVER);

				suitOn = true;
			}
			else if (!checkBreath || (checkBreath && InBreathableArea(true)))
			{
				vslCargoInfo.slots.front().hAttach = GetAttachmentHandle(false, 1);

				if (!headlights.empty()) SetHeadlight(false);
				SetMeshVisibilityMode(suitMesh, MESHVIS_NEVER);
				SetMeshVisibilityMode(astrMesh, MESHVIS_ALWAYS);

				suitOn = false;
			}
		}

		bool Astronaut::InBreathableArea(bool showMessage)
		{
			if (pVslAPI->InBreathableArea()) return true;

			const double pressure = GetAtmPressure();
			const double temp = GetAtmTemperature();

			if (!showMessage) return temp > 223 && temp < 373 && pressure > 3.6e4 && pressure < 2.5e5;

			if (pressure < 3.6e4) hudInfo.message = "Error: Outside pressure low.";
			else if (pressure > 2.5e5) hudInfo.message = "Error: Outside pressure high.";
			else if (temp < 223) hudInfo.message = "Error: Outside temperature low.";
			else if (temp > 373) hudInfo.message = "Error: Outside temperature high.";
			else return true;

			hudInfo.timer = 0;

			return false;
		}

		void Astronaut::Kill()
		{
			if (!headlights.empty()) SetHeadlight(false);

			astrInfo.alive = false;
		}

		void Astronaut::DrawNearHUD(int x, int y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Nearest airlock information", 27);

			y += 30;

			auto nearAirlock = GetNearestAirlock(maxNearestRange);

			if (!nearAirlock) { skp->Text(x, y, "No vessel in range", 18); goto breathLabel; }

			if (hudInfo.hVessel != (*nearAirlock).hVessel)
			{
				hudInfo.hVessel = (*nearAirlock).hVessel;
				hudInfo.vslInfo = HudInfo::VesselInfo();

				hudInfo.vslInfo.arlckIdx = (*nearAirlock).airlockIdx;
				hudInfo.vslInfo.statIdx = (*nearAirlock).stationIdx;

				hudInfo.vslInfo.info = GetVslAstrInfo(hudInfo.hVessel);
			}

			buffer = std::format("Selected vessel name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			buffer = std::format("Station count: {}", hudInfo.vslInfo.info->stations.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			{
				const auto& stationInfo = hudInfo.vslInfo.info->stations.at(hudInfo.vslInfo.statIdx);

				buffer = std::format("Selected station name: {}, {}", stationInfo.name.c_str(), stationInfo.astrInfo ? "occupied" : "empty");
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 30;
			}

			buffer = std::format("Airlock name: {}", (*nearAirlock).airlockInfo.name.c_str());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			DrawVslInfo(x, y, skp, (*nearAirlock).airlockInfo.pos);

		breathLabel:
			x = hps->W - 10;
			y = int(0.215 * hps->H);
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Nearest breathable cargo information", 36);

			y += 30;

			OBJHANDLE hCargo = pVslAPI->GetNearestBreathable();

			if (!hCargo) { skp->Text(x, y, "No cargo in range", 17); return; }

			DrawCargoInfo(x, y, skp, *(pVslAPI->GetCargoInfoByHandle(hCargo)), false);

			y += 30;

			VECTOR3 relPos;
			oapiGetGlobalPos(hCargo, &relPos);
			Global2Local(relPos, relPos);
			DrawVslInfo(x, y, skp, relPos);
		}

		void Astronaut::DrawVslHUD(int x, int y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Vessel information", 18);

			y += 30;

			size_t vesselCount = oapiGetVesselCount() - pVslAPI->GetScnCargoCount() - 1;

			if (!hudInfo.hVessel)
			{
				if (!vesselCount) { skp->Text(x, y, "No vessel in scenario", 21); return; }

				hudInfo.vslIdx = *GetFirstVslIdx();
				hudInfo.hVessel = oapiGetVesselByIndex(hudInfo.vslIdx);
			}

			buffer = std::format("Vessel count: {}", vesselCount);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Selected vessel name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			hudInfo.vslInfo.info = GetVslAstrInfo(hudInfo.hVessel);

			y += 30;

			if (!hudInfo.vslInfo.info || !hudInfo.vslInfo.info->stations.size() || !hudInfo.vslInfo.info->airlocks.size())
			{
				VECTOR3 relPos;
				oapiGetGlobalPos(hudInfo.hVessel, &relPos);
				Global2Local(relPos, relPos);
				DrawVslInfo(x, y, skp, relPos);

				y += 30;

				if (!!hudInfo.vslInfo.info) skp->Text(x, y, "Selected vessel doesn't support UACS", 36);
				else if (!hudInfo.vslInfo.info->stations.size()) skp->Text(x, y, "No station defined for selected vessel", 38);
				else if (!hudInfo.vslInfo.info->airlocks.size()) skp->Text(x, y, "No airlock defined for selected vessel", 38);

				return;
			}

			buffer = std::format("Station count: {}", hudInfo.vslInfo.info->stations.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			const auto& stationInfo = hudInfo.vslInfo.info->stations.at(hudInfo.vslInfo.statIdx);

			buffer = std::format("Seleceted station name: {}, {}", stationInfo.name.c_str(), stationInfo.astrInfo ? "occupied" : "empty");
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			buffer = std::format("Airlock count: {}", hudInfo.vslInfo.info->airlocks.size());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			const auto& airlockInfo = hudInfo.vslInfo.info->airlocks.at(hudInfo.vslInfo.arlckIdx);

			buffer = std::format("Selected airlock name: {}, {}", airlockInfo.name.c_str(), airlockInfo.open ? "opened" : "closed");
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			VECTOR3 airlockPos;
			oapiLocalToGlobal(hudInfo.hVessel, &airlockInfo.pos, &airlockPos);
			Global2Local(airlockPos, airlockPos);

			DrawVslInfo(x, y, skp, airlockPos);
		}

		void Astronaut::DrawAstrHUD(int x, int y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Astronaut information", 21);

			y += 30;

			if (pVslAPI->GetScnAstrCount() - 1)
			{
				buffer = std::format("Astronaut count: {}", pVslAPI->GetScnAstrCount() - 1);
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 20;

				if (!oapiIsVessel(hudInfo.hVessel))
				{
					hudInfo.vslIdx = *GetFirstAstrIdx();
					hudInfo.hVessel = pVslAPI->GetAstrInfoByIndex(hudInfo.vslIdx).first;
				}

				if (hudInfo.hVessel)
				{
					buffer = std::format("Scenario name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
					skp->Text(x, y, buffer.c_str(), buffer.size());

					y += 30;

					VECTOR3 relPos;
					oapiGetGlobalPos(hudInfo.hVessel, &relPos);
					Global2Local(relPos, relPos);
					DrawVslInfo(x, y, skp, relPos);

					x = hps->W - 10;
					y = int(0.215 * hps->H);
					skp->SetTextAlign(oapi::Sketchpad::RIGHT);

					DrawAstrInfo(x, y, skp, *pVslAPI->GetAstrInfoByIndex(hudInfo.vslIdx).second);
				}
			}

			else skp->Text(x, y, "No astronaut in scenario", 24);
		}

		void Astronaut::DrawCargoHUD(int x, int y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Cargo information", 17);

			y += 30;

			if (pVslAPI->GetScnCargoCount())
			{
				buffer = std::format("Cargo count: {}", pVslAPI->GetScnCargoCount());
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 20;

				if (!oapiIsVessel(hudInfo.hVessel) || pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).attached)
				{
					auto index = GetFirstVslIdx();

					if (index)
					{
						hudInfo.vslIdx = *index;
						hudInfo.hVessel = pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).handle;
					}
					else skp->Text(x, y, "No free cargo in scenario", 25);
				}

				if (hudInfo.hVessel)
				{
					DrawCargoInfo(x, y, skp, pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx), true);

					y += 30;

					VECTOR3 relPos;
					oapiGetGlobalPos(hudInfo.hVessel, &relPos);
					Global2Local(relPos, relPos);
					DrawVslInfo(x, y, skp, relPos);
				}
			}

			else skp->Text(x, y, "No cargo in scenario", 20);

			x = hps->W - 10;
			y = int(0.215 * hps->H);
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			if (hudInfo.modeTimer < 5)
			{
				skp->Text(x, y, hudInfo.modeMsg.c_str(), hudInfo.modeMsg.size());
				y += 30;
			}

			buffer = std::format("Selected cargo to add: {}", pVslAPI->GetAvailCargoName(hudInfo.availIdx).data());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Selected resource to drain: {}", hudInfo.drainFuel ? "Fuel" : "Oxygen");
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			skp->Text(x, y, "Grappled cargo information", 26);

			y += 30;

			if (const auto& cargoInfo = vslCargoInfo.slots.front().cargoInfo) DrawCargoInfo(x, y, skp, *cargoInfo, true);
			else skp->Text(x, y, "No cargo is grappled", 20);
		}

		void Astronaut::DrawShort1HUD(int x, int y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "General shortcuts", 17);
			y += 20;

			skp->Text(x, y, "Alt + M: Change HUD mode", 24);
			y += 20;

			skp->Text(x, y, "Alt + S: Toggle suit", 20);
			y += 20;

			if (!headlights.empty()) skp->Text(x, y, "Alt + L: Toggle Headlight", 25);

			x = hps->W - 10;
			y = int(0.215 * hps->H);
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Nearest & Vessel HUD shortcuts", 30);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 8: Select next station", 35);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 2: Select previous station", 39);
			y += 20;

			skp->Text(x, y, "Alt + I: Ingress into nearest station", 37);

			y += 30;
			skp->Text(x, y, "Vessel information HUD shortcuts", 32);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 6: Select next vessel", 34);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 4: Select previous vessel", 38);
			y += 20;

			skp->Text(x, y, "Alt + A: Select next airlock", 28);
			y += 20;

			skp->Text(x, y, "Ctrl + Alt + I: Ingress into selected station", 45);
		}

		void Astronaut::DrawShort2HUD(int x, int y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Cargo information HUD shortcuts", 31);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 6: Select next scenario cargo", 42);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 4: Select previous scenario cargo", 46);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 8: Select next available cargo", 43);
			y += 20;

			skp->Text(x, y, "Alt + Numpad 2: Select previous available cargo", 47);
			y += 20;

			skp->Text(x, y, "Alt + T: Select resource to drain", 33);

			x = hps->W - 10;
			y = int(0.215 * hps->H);
			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			skp->Text(x, y, "Alt + A = Add selected available cargo", 38);
			y += 20;

			skp->Text(x, y, "Alt + G = Grapple nearest cargo", 31);
			y += 20;

			skp->Text(x, y, "Ctrl + Alt + G = Grapple selected cargo", 39);
			y += 20;

			skp->Text(x, y, "Alt + R = Release grappled cargo", 32);
			y += 20;

			skp->Text(x, y, "Alt + P = Pack nearest cargo", 28);
			y += 20;

			skp->Text(x, y, "Ctrl + Alt + P = Pack selected cargo", 36);
			y += 20;

			skp->Text(x, y, "Alt + U = Unpack nearest cargo", 30);
			y += 20;

			skp->Text(x, y, "Ctrl + Alt + U = Unpack selected cargo", 38);
			y += 20;

			skp->Text(x, y, "Alt + F = Drain nearest resource", 32);
			y += 20;

			skp->Text(x, y, "Ctrl + Alt + F = Drain selected resource", 40);
			y += 20;

			skp->Text(x, y, "Alt + D = Delete grappled cargo", 31);
		}

		void Astronaut::DrawVslInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos)
		{
			double distance = length(relPos);

			buffer = std::format("Distance: {:.1f}m", distance);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			double relYaw = (atan2(relPos.z, relPos.x) - PI05) * DEG;

			if (relYaw > 180) relYaw -= 360;
			else if (relYaw < -180) relYaw += 360;

			if (GetFlightStatus())
			{
				buffer = std::format("Relative heading: {:.1f}", relYaw);
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}

			else
			{
				double relPitch = (atan2(sqrt(relPos.z * relPos.z + relPos.x * relPos.x), relPos.y) - PI05) * DEG;

				buffer = std::format("Relative pitch: {:.1f}", relPitch);
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 20;

				buffer = std::format("Relative yaw: {:.1f}", relYaw);
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		void Astronaut::DrawAstrInfo(int x, int& y, oapi::Sketchpad* skp, const API::AstrInfo& astrInfo)
		{
			buffer = std::format("Name: {}", astrInfo.name.c_str());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Role: {}", astrInfo.role.c_str());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Mass: {:g}kg", astrInfo.mass);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			buffer = std::format("Fuel: {:g}%", astrInfo.fuelLvl * 100);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Oxygen: {:g}%", astrInfo.oxyLvl * 100);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Alive: {}", astrInfo.alive ? "Yes" : "No");
			skp->Text(x, y, buffer.c_str(), buffer.size());
		}

		void Astronaut::DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const API::CargoInfo& cargoInfo, bool drawBreathable)
		{
			buffer = std::format("Name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Mass: {:g}kg", oapiGetMass(hudInfo.hVessel));
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			switch (cargoInfo.type)
			{
			case API::STATIC:
				skp->Text(x, y, "Type: Static", 12);
				break;

			case API::UNPACK_ONLY:
				skp->Text(x, y, "Type: Unpackable only", 21);

				if (drawBreathable)
				{
					y += 20;
					buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}

				break;

			case API::PACK_UNPACK:
				skp->Text(x, y, "Type: Packacble and unpackable", 30);

				if (drawBreathable)
				{
					y += 20;
					buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}

				break;
			}

			if (cargoInfo.resource)
			{
				y += 20;

				buffer = std::format("Resource: {}", (*cargoInfo.resource).c_str());
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		std::optional<size_t> Astronaut::GetFirstVslIdx()
		{
			for (size_t idx{}; idx < oapiGetVesselCount(); ++idx) if (!pVslAPI->GetCargoInfoByHandle(oapiGetVesselByIndex(idx))) return idx;

			return {};
		}

		std::optional<size_t> Astronaut::GetFirstAstrIdx()
		{
			for (size_t idx{}; idx < pVslAPI->GetScnAstrCount(); ++idx) if (pVslAPI->GetAstrInfoByIndex(idx).first != GetHandle()) return idx;

			return {};
		}

		std::optional<size_t> Astronaut::GetFirstCargoIdx()
		{
			for (size_t idx{}; idx < pVslAPI->GetScnCargoCount(); ++idx)
				if (!pVslAPI->GetCargoInfoByIndex(idx).attached) return idx;

			return {};
		}
	}
}