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
		bool configLoaded{};
		double suitMass{ 60 };
		double maxNearestRange{ 60e3 };
		bool realMode{};

		void LoadConfig()
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

		Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : API::Astronaut(hVessel, fModel), pVslAPI(API::Vessel::CreateInstance(this, nullptr, &cargoInfo))
		{
			if (!configLoaded) LoadConfig();

			astrInfo.fuelLvl = astrInfo.oxyLvl = 1;
			astrInfo.alive = true;
			astrInfo.className = "Astronaut";

			lonSpeed.maxLimit = 10;
			lonSpeed.minLimit = latSpeed.minLimit = -(latSpeed.maxLimit = 1);
			steerAngle.minLimit = -(steerAngle.maxLimit = 45 * RAD);

			hudInfo.deadFont = oapiCreateFont(50, true, "Courier New");

			cargoInfo.astrMode = true;
			cargoInfo.breathableRange = maxNearestRange;
			cargoInfo.grappleRange = 5;
			cargoInfo.relVel = 0;
			cargoInfo.relRowCount = 1;
			cargoInfo.packRange = 5;
			cargoInfo.drainRange = 5;

			SetDefaultValues();
		}

		Astronaut::~Astronaut() { oapiReleaseFont(hudInfo.deadFont); }

		void Astronaut::clbkSetClassCaps(FILEHANDLE cfg)
		{
			char cBuffer[256];

			if (!oapiReadItem_string(cfg, "Name", cBuffer)) WarnAndTerminate("name", GetClassNameA(), "astronaut");
			astrInfo.name = cBuffer;

			if (!oapiReadItem_string(cfg, "Role", cBuffer)) WarnAndTerminate("role", GetClassNameA(), "astronaut");
			astrInfo.role = cBuffer;

			if (!oapiReadItem_string(cfg, "Mesh", cBuffer)) WarnAndTerminate("mesh", GetClassNameA(), "astronaut");
			mesh = cBuffer;

			if (!oapiReadItem_float(cfg, "Mass", astrInfo.mass)) WarnAndTerminate("mass", GetClassNameA(), "astronaut");

			SetEmptyMass(suitMass + astrInfo.mass);

			SetSize(2);

			SetCrossSections({ 0.76, 0.44, 0.96 });

			SetPMI({ 0.27, 0.07, 0.27 });

			SetCameraOffset({ 0, 0.4905, 0.075 });

			suitMesh = AddMesh((mesh + "Suit").c_str());
			astrMesh = AddMesh(mesh.c_str());

			AddBeacon(&beacon1Info.spec);

			beacon2Info.pos.x = -beacon1Info.pos.x;

			AddBeacon(&beacon2Info.spec);

			spotLight1 = static_cast<SpotLight*>(AddSpotLight(spotInfo.pos, spotInfo.dir, spotInfo.range,
				spotInfo.att0, spotInfo.att1, spotInfo.att2, spotInfo.umbra, spotInfo.penumbra,
				spotInfo.diffuse, spotInfo.specular, spotInfo.ambient));

			spotInfo.pos.x = -spotInfo.pos.x;

			spotLight2 = static_cast<SpotLight*>(AddSpotLight(spotInfo.pos, spotInfo.dir, spotInfo.range,
				spotInfo.att0, spotInfo.att1, spotInfo.att2, spotInfo.umbra, spotInfo.penumbra,
				spotInfo.diffuse, spotInfo.specular, spotInfo.ambient));

			spotLight1->Activate(false);
			spotLight2->Activate(false);

			cargoInfo.slots.push_back({ CreateAttachment(false, { 0, -0.45, 1.05 }, { 0, -1, 0 }, { 0, 0, -1 }, "UACS"), true });

			hFuel = CreatePropellantResource(10);
			hOxy = CreatePropellantResource(1);

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

			static std::array<TOUCHDOWNVTX, 13> tdvtx
			{ {
			{ { 0,        -1.22799,   0.1858 },   1.3e4, 2.7e3, 3, 3},
			{ { -0.178,   -1.22799,  -0.1305 },   1.3e4, 2.7e3, 3, 3},
			{ { 0.178,    -1.22799,  -0.1305 },   1.3e4, 2.7e3, 3, 3},

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

			SetTouchdownPoints(tdvtx.data(), tdvtx.size());
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

					else if (data == "MESH") ss >> mesh;

					else if (data == "MASS") ss >> astrInfo.mass;

					else if (data == "ALIVE") ss >> astrInfo.alive;

					else if (data == "SUIT_ON") ss >> suitOn;

					else if (data == "HUD_MODE") ss >> hudInfo.mode;

					else if (data == "HEADLIGHT") { bool active; ss >> active; SetHeadlight(active); }

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
			oapiWriteScenario_string(scn, "MESH", mesh.data());
			oapiWriteScenario_float(scn, "MASS", astrInfo.mass);

			oapiWriteScenario_int(scn, "ALIVE", astrInfo.alive);
			oapiWriteScenario_int(scn, "SUIT_ON", suitOn);
			oapiWriteScenario_int(scn, "HUD_MODE", hudInfo.mode);
			oapiWriteScenario_int(scn, "HEADLIGHT", beacon1Info.spec.active);
		}

		void Astronaut::clbkPostCreation()
		{
			SetEmptyMass(suitMass + astrInfo.mass);

			SetSuit(suitOn, false);

			if (!astrInfo.alive) { Kill(); return; }

			astrInfo.fuelLvl = GetPropellantMass(hFuel);
			astrInfo.oxyLvl = GetPropellantMass(hOxy);

			if (GetFlightStatus()) SetAttitudeMode(RCS_NONE);
		}

		void Astronaut::clbkSetAstrInfo(const API::AstrInfo& astrInfo)
		{
			this->astrInfo = astrInfo;

			SetEmptyMass(suitMass + astrInfo.mass);
			SetPropellantMass(hFuel, astrInfo.fuelLvl * 10);
			SetPropellantMass(hOxy, astrInfo.oxyLvl);
		}

		const API::AstrInfo* Astronaut::clbkGetAstrInfo()
		{
			astrInfo.fuelLvl = GetPropellantMass(hFuel) / 10;

			return &astrInfo;
		}

		int Astronaut::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
		{
			if (!astrInfo.alive || !down) return 0;

			if (hudInfo.mode == HUD_CARGO && KEYMOD_ALT(kstate) && key == OAPI_KEY_F)
			{
				hudInfo.drainFuel = !hudInfo.drainFuel;
				return 1;
			}

			if (!KEYMOD_SHIFT(kstate)) return 0;

			switch (hudInfo.mode)
			{
			case HUD_VESSEL:
				if (key == OAPI_KEY_UP || key == OAPI_KEY_DOWN)
				{
					if (oapiGetVesselCount() - pVslAPI->GetScenarioCargoCount() > 1)
					{
						do
						{
							if (key == OAPI_KEY_UP) hudInfo.vslIdx + 1 < oapiGetVesselCount() ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
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

			case HUD_NEAREST:
				if (key == OAPI_KEY_RIGHT)
				{
					hudInfo.vslInfo.statIdx + 1 < hudInfo.vslInfo.info->stations.size() ? ++hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = 0;
					return 1;
				}

				else if (key == OAPI_KEY_LEFT)
				{
					hudInfo.vslInfo.statIdx > 0 ? --hudInfo.vslInfo.statIdx : hudInfo.vslInfo.statIdx = hudInfo.vslInfo.info->stations.size() - 1;
					return 1;
				}

				break;

			case HUD_CARGO:
			{
				if (key == OAPI_KEY_UP || key == OAPI_KEY_DOWN)
				{
					if (pVslAPI->GetScenarioCargoCount() && GetFirstCargoIndex())
					{
						do
						{
							if (key == OAPI_KEY_UP) int(hudInfo.vslIdx + 1) < pVslAPI->GetScenarioCargoCount() ? ++hudInfo.vslIdx : hudInfo.vslIdx = 0;
							else hudInfo.vslIdx > 0 ? --hudInfo.vslIdx : hudInfo.vslIdx = pVslAPI->GetScenarioCargoCount() - 1;
						} while (pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).attached);

						hudInfo.hVessel = pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).handle;
					}

					return 1;
				}

				OBJHANDLE hCargo = KEYMOD_CONTROL(kstate) ? hudInfo.hVessel : nullptr;

				switch (key)
				{
				case OAPI_KEY_RIGHT:
					int(hudInfo.availIdx + 1) < pVslAPI->GetAvailableCargoCount() ? ++hudInfo.availIdx : hudInfo.availIdx = 0;
					return 1;

				case OAPI_KEY_LEFT:
					hudInfo.availIdx > 0 ? --hudInfo.availIdx : hudInfo.availIdx = pVslAPI->GetAvailableCargoCount() - 1;
					return 1;

				case OAPI_KEY_A:
					switch (pVslAPI->AddCargo(hudInfo.availIdx))
					{
					case API::GRPL_SUCCED:
						hudInfo.modeMsg = "The selected cargo was added successfully.";
						break;

					case API::GRPL_SLT_OCCP:
						hudInfo.modeMsg = "Couldn't add the selected cargo: You're already holding a cargo!";
						break;

					case API::GRPL_FAIL:
						hudInfo.modeMsg = "Couldn't add the selected cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_D:
					switch (pVslAPI->DeleteCargo())
					{
					case API::RLES_SUCCED:
						hudInfo.modeMsg = "The grappled cargo was deleted successfully.";
						break;

					case API::RLES_SLT_EMPTY:
						hudInfo.modeMsg = "Couldn't delete cargo: There is nothing to delete!";
						break;

					case API::RLES_FAIL:
						hudInfo.modeMsg = "Couldn't delete cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_G:
					switch (pVslAPI->GrappleCargo(hCargo))
					{
					case API::GRPL_SUCCED:
						if (hCargo) hudInfo.modeMsg = "The selected cargo was grappled successfully.";
						else hudInfo.modeMsg = "The nearest cargo was grappled successfully.";
						break;

					case API::GRPL_SLT_OCCP:
						hudInfo.modeMsg = "Couldn't grapple cargo: You're already holding a cargo!";
						break;

					case API::GRPL_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Couldn't grapple cargo: The selected cargo is too far away.";
						else hudInfo.modeMsg = "Couldn't grapple cargo: No cargo in range.";
						break;

					case API::GRPL_FAIL:
						hudInfo.modeMsg = "Couldn't grapple cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_R:
					switch (pVslAPI->ReleaseCargo())
					{
					case API::RLES_SUCCED:
						hudInfo.modeMsg = "The grappled cargo was released successfully.";
						break;

					case API::RLES_SLT_EMPTY:
						hudInfo.modeMsg = "Couldn't release cargo: There is nothing to release!";
						break;

					case API::RLES_NO_EMPTY_POS:
						hudInfo.modeMsg = "Couldn't release cargo: No empty position nearby.";
						break;

					case API::RLES_FAIL:
						hudInfo.modeMsg = "Couldn't release cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_P:
					switch (pVslAPI->PackCargo(hCargo))
					{
					case API::PACK_SUCCED:
						if (hCargo) hudInfo.modeMsg = "The selected cargo was packed successfully.";
						else hudInfo.modeMsg = "The nearest cargo was packed successfully.";
						break;

					case API::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Couldn't pack cargo: The selected cargo is too far away.";
						else hudInfo.modeMsg = "Couldn't pack cargo: No packable cargo in range.";
						break;

					case API::PACK_CRG_PCKD:
						hudInfo.modeMsg = "Couldn't pack cargo: The cargo is already packed.";
						break;

					case API::PACK_CRG_NOT_PCKABL:
						hudInfo.modeMsg = "Couldn't pack cargo: The cargo isn't packable.";
						break;

					case API::PACK_FAIL:
						hudInfo.modeMsg = "Couldn't pack cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_U:
					switch (pVslAPI->UnpackCargo(hCargo))
					{
					case API::PACK_SUCCED:
						if (hCargo) hudInfo.modeMsg = "The selected cargo was unpacked successfully.";
						else hudInfo.modeMsg = "The nearest cargo was unpacked successfully.";
						break;

					case API::PACK_NOT_IN_RNG:
						if (hCargo) hudInfo.modeMsg = "Couldn't unpack cargo: The selected cargo is too far away.";
						else hudInfo.modeMsg = "Couldn't unpack cargo: No unpackable cargo in range.";
						break;

					case API::PACK_CRG_UNPCKD:
						hudInfo.modeMsg = "Couldn't unpack cargo: The cargo is already unpacked.";
						break;

					case API::PACK_CRG_NOT_PCKABL:
						hudInfo.modeMsg = "Couldn't unpack cargo: The cargo isn't unpackable.";
						break;

					case API::PACK_FAIL:
						hudInfo.modeMsg = "Couldn't unpack cargo.";
						break;
					}

					hudInfo.modeTimer = 0;
					return 1;

				case OAPI_KEY_F:
				{
					const char* resource;
					double reqMass;

					if (hudInfo.drainFuel) { resource = "fuel"; reqMass = 10 - GetPropellantMass(hFuel); }
					else { resource = "oxygen"; reqMass = 1 - astrInfo.oxyLvl; }

					if (reqMass)
					{
						API::DrainInfo drainInfo = pVslAPI->DrainGrappledResource(resource, reqMass);

						if (drainInfo.result != API::DRIN_SUCCED) drainInfo = pVslAPI->DrainUngrappledResource(resource, reqMass, hCargo);

						switch (drainInfo.result)
						{
						case UACS::API::DRIN_SUCCED:
							if (hudInfo.drainFuel) SetPropellantMass(hFuel, GetPropellantMass(hFuel) + drainInfo.mass);
							else SetPropellantMass(hOxy, astrInfo.oxyLvl + drainInfo.mass);

							hudInfo.modeMsg = std::format("{:g} kg of {} was drained.", drainInfo.mass, resource);
							break;

						case UACS::API::DRIN_NOT_IN_RNG:
							if (hCargo) hudInfo.modeMsg = "Couldn't drain resource: The selected cargo is too far away.";
							else hudInfo.modeMsg = "Couldn't drain resource: No resource cargo in range.";
							break;

						case UACS::API::DRIN_NOT_RES:
							hudInfo.modeMsg = "Couldn't drain resource: The selected cargo isn't a resource.";
							break;

						case UACS::API::DRIN_RES_NOMATCH:
							hudInfo.modeMsg = "Couldn't drain resource: The selected cargo resource doesn't match.";
							break;

						case UACS::API::DRIN_FAIL:
							hudInfo.modeMsg = "Couldn't drain resource.";
							break;
						}
					}
					else hudInfo.modeMsg = "Couldn't drain resource: The selected resource is already full!";

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
				if (suitOn) SetHeadlight(!beacon1Info.spec.active);
				return 1;

			case OAPI_KEY_S:
				if (pVslAPI->GetCargoInfoBySlot(0))
				{
					if (suitOn) hudInfo.message = "Couldn't remove suit: You're holding a cargo.";
					else hudInfo.message = "Couldn't wear suit: You're holding a cargo.";
					hudInfo.timer = 0;
				}

				else if (!astrInfo.oxyLvl && !suitOn)
				{
					hudInfo.message = "Couldn't wear suit: No oxygen available.";
					hudInfo.timer = 0;
				}

				else SetSuit(!suitOn, true);

				return 1;

			case OAPI_KEY_H:
				hudInfo.mode < 3 ? ++hudInfo.mode : hudInfo.mode = 0;

				hudInfo.vslIdx = 0;
				hudInfo.hVessel = nullptr;
				hudInfo.vslInfo = HudInfo::VesselInfo();
				return 1;

			case OAPI_KEY_I:
			{
				if (pVslAPI->GetCargoInfoBySlot(0))
				{
					hudInfo.message = "Couldn't ingress: You're holding a cargo.";
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
					if (selected) hudInfo.message = "Couldn't ingress: The selected airlock is too far away.";
					else hudInfo.message = "Couldn't ingress: No airlock in range.";
					break;

				case API::INGRS_ARLCK_UNDEF:
					hudInfo.message = "Couldn't ingress: The selected vessel has no airlocks.";
					break;

				case API::INGRS_ARLCK_CLSD:
					hudInfo.message = "Couldn't ingress: The selected airlock is closed.";
					break;

				case API::INGRS_STN_UNDEF:
					hudInfo.message = "Couldn't ingress: The selected vessel has no stations.";
					break;

				case API::INGRS_STN_OCCP:
					hudInfo.message = "Couldn't ingress: The selected station is occupied.";
					break;

				case API::INGRS_FAIL:
					hudInfo.message = "Couldn't ingress.";
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
			if (!astrInfo.alive || !GetFlightStatus()) return 0;

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
			if (!astrInfo.alive) return;

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
				const double time = astrInfo.oxyLvl / (consumptionRate * 3600);
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
			case HUD_NEAREST:
				DrawNearestHUD(x, y, hps, skp);
				break;

			case HUD_VESSEL:
				DrawVesselHUD(x, y, skp);
				break;

			case HUD_CARGO:
				DrawCargoHUD(x, y, hps, skp);
				break;
			}

			return true;
		}

		void Astronaut::SetOxygenConsumption(double simdt)
		{
			astrInfo.oxyLvl = GetPropellantMass(hOxy);

			if (suitOn)
			{
				const double speed = abs(lonSpeed.value) + abs(latSpeed.value) + abs(steerAngle.value) / PI;

				if (speed > 2.7) consumptionRate = speed * 3.233e-5;
				else if (speed) consumptionRate = speed * (1.079e-5 * speed + 3.233e-6);
				else consumptionRate = 0;

				consumptionRate += 9.43e-6;

				astrInfo.oxyLvl -= consumptionRate * simdt;

				if (astrInfo.oxyLvl > 0) SetPropellantMass(hOxy, astrInfo.oxyLvl);
				else
				{
					SetPropellantMass(hOxy, 0);

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
					SetGroundRotation(status, 1.228);
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

				if (lonSpeed.value > 1.55)
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

			if (lngOffset || latOffset) SetGroundRotation(status, lngOffset, latOffset, 1.228);
			else SetGroundRotation(status, 1.228);

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
			beacon1Info.spec.active = beacon2Info.spec.active = active;
			spotLight1->Activate(active);
			spotLight2->Activate(active);
		}

		void Astronaut::SetSuit(bool on, bool checkBreath)
		{
			if (on)
			{
				SetAttachmentParams(cargoInfo.slots.at(0).hAttach, { 0, -0.45, 1.05 }, { 0, -1, 0 }, { 0, 0, -1 });

				SetMeshVisibilityMode(suitMesh, MESHVIS_ALWAYS);
				SetMeshVisibilityMode(astrMesh, MESHVIS_NEVER);

				suitOn = true;
			}
			else if (!checkBreath || (checkBreath && InBreathableArea(true)))
			{
				SetAttachmentParams(cargoInfo.slots.at(0).hAttach, { 0, -0.45, 0.81 }, { 0, -1, 0 }, { 0, 0, -1 });

				SetHeadlight(false);
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

			if (pressure < 3.6e4) hudInfo.message = "Can't remove suit, pressure is too low";
			else if (pressure > 2.5e5) hudInfo.message = "Can't remove suit, pressure is too high";
			else if (temp < 223) hudInfo.message = "Can't remove suit, temperature is too low";
			else if (temp > 373) hudInfo.message = "Can't remove suit, temperature is too high";
			else return true;

			hudInfo.timer = 0;

			return false;
		}

		void Astronaut::Kill()
		{
			SetHeadlight(false);

			DelPropellantResource(hFuel);
			DelPropellantResource(hOxy);

			astrInfo.alive = false;
		}

		void Astronaut::DrawNearestHUD(int& x, int& y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
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

				buffer = std::format("Seleceted station name: {}, {}", stationInfo.name.c_str(), stationInfo.astrInfo ? "occupied" : "empty");
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 30;
			}

			buffer = std::format("Airlock name: {}", (*nearAirlock).airlockInfo.name.c_str());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			DrawVesselInfo(x, y, skp, (*nearAirlock).airlockInfo.pos);

		breathLabel:
			x = hps->W - 5;
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
			DrawVesselInfo(x, y, skp, relPos);
		}

		void Astronaut::DrawVesselHUD(int& x, int& y, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Target vessel information", 25);

			y += 30;

			size_t vesselCount = oapiGetVesselCount() - pVslAPI->GetScenarioCargoCount() - 1;

			if (!hudInfo.hVessel)
			{
				if (!vesselCount) { skp->Text(x, y, "No vessel in scenario", 21); return; }

				hudInfo.vslIdx = *GetFirstVesselIndex();
				hudInfo.hVessel = oapiGetVesselByIndex(hudInfo.vslIdx);
			}

			buffer = std::format("Vessel count: {}", vesselCount);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Selected vessel name: {}", oapiGetVesselInterface(hudInfo.hVessel)->GetName());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			hudInfo.vslInfo.info = GetVslAstrInfo(hudInfo.hVessel);

			y += 30;

			if (!hudInfo.vslInfo.info->stations.size() || !hudInfo.vslInfo.info->airlocks.size())
			{
				VECTOR3 relPos;
				oapiGetGlobalPos(hudInfo.hVessel, &relPos);
				Global2Local(relPos, relPos);
				DrawVesselInfo(x, y, skp, relPos);

				y += 30;

				if (!hudInfo.vslInfo.info->stations.size()) skp->Text(x, y, "No station is defined for selected vessel", 41);
				else if (!hudInfo.vslInfo.info->airlocks.size()) skp->Text(x, y, "No airlock is defined for selected vessel", 41);

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

			DrawVesselInfo(x, y, skp, airlockPos);
		}

		void Astronaut::DrawCargoHUD(int& x, int& y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			skp->Text(x, y, "Target cargo information", 24);

			y += 30;

			if (pVslAPI->GetScenarioCargoCount())
			{
				buffer = std::format("Cargo count: {}", pVslAPI->GetScenarioCargoCount());
				skp->Text(x, y, buffer.c_str(), buffer.size());

				y += 20;

				if (!oapiIsVessel(hudInfo.hVessel) || pVslAPI->GetCargoInfoByIndex(hudInfo.vslIdx).attached)
				{
					auto index = GetFirstCargoIndex();

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
					DrawVesselInfo(x, y, skp, relPos);
				}
			}

			else skp->Text(x, y, "No cargo in scenario", 20);

			x = hps->W - 5;
			y = int(0.215 * hps->H);

			skp->SetTextAlign(oapi::Sketchpad::RIGHT);

			if (hudInfo.modeTimer < 5)
			{
				skp->Text(x, y, hudInfo.modeMsg.c_str(), hudInfo.modeMsg.size());
				y += 30;
			}

			buffer = std::format("Selected cargo to add: {}", pVslAPI->GetAvailableCargoName(hudInfo.availIdx).data());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			buffer = std::format("Selected resource to drain: {}", hudInfo.drainFuel ? "Fuel" : "Oxygen");
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 30;

			skp->Text(x, y, "Grappled cargo information", 26);

			y += 30;

			if (auto cargoInfo = pVslAPI->GetCargoInfoBySlot(0)) DrawCargoInfo(x, y, skp, *cargoInfo, true);
			else skp->Text(x, y, "No cargo is grappled", 20);
		}

		void Astronaut::DrawVesselInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos)
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

		void Astronaut::DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const API::CargoInfo& cargoInfo, bool drawBreathable)
		{
			buffer = std::format("Name: {}", cargoInfo.name.c_str());
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			buffer = std::format("Mass: {:g}kg", cargoInfo.mass);
			skp->Text(x, y, buffer.c_str(), buffer.size());

			y += 20;

			switch (cargoInfo.type)
			{
			case API::STATIC:
				skp->Text(x, y, "Type: Static", 12);
				break;

			case API::UNPACKABLE_ONLY:
				skp->Text(x, y, "Type: Unpackable only", 21);

				if (drawBreathable)
				{
					y += 20;
					buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}

				break;

			case API::PACKABLE_UNPACKABLE:
				skp->Text(x, y, "Type: Packacble and unpackable", 30);

				if (drawBreathable)
				{
					y += 20;
					buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}

				break;
			}

			y += 20;

			if (!cargoInfo.resource.empty())
			{
				buffer = std::format("Resource: {}", cargoInfo.resource.c_str());
				skp->Text(x, y, buffer.c_str(), buffer.size());
			}
		}

		std::optional<size_t> Astronaut::GetFirstVesselIndex()
		{
			for (size_t index{}; index < oapiGetVesselCount(); ++index) if (!pVslAPI->GetCargoInfoByHandle(oapiGetVesselByIndex(index))) return index;

			return {};
		}

		std::optional<size_t> Astronaut::GetFirstCargoIndex()
		{
			for (size_t index{}; index < pVslAPI->GetScenarioCargoCount(); ++index) 
				if (!pVslAPI->GetCargoInfoByIndex(index).attached) return index;

			return {};
		}
	}
}