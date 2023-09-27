#include "Carrier.h"

#include <format>
#include <array>
#include <sstream>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel) { return new UACS::Vessel::Carrier(hvessel, flightmodel); }

DLLCLBK void ovcExit(VESSEL* vessel) { if (vessel) delete static_cast<UACS::Vessel::Carrier*>(vessel); }

namespace UACS
{
	namespace Vessel
	{
		Carrier::Carrier(OBJHANDLE hVessel, int flightmodel) : VESSEL4(hVessel, flightmodel), mdlAPI(this, &vslAstrInfo, &vslCargoInfo)
		{
			cargoHUD.msg = std::format("UACS version: {}", mdlAPI.GetUACSVersion());
			cargoHUD.timer = 0;
		}

		void Carrier::clbkSetClassCaps(FILEHANDLE cfg)
		{
			UACS::AirlockInfo airInfo;

			airInfo.name = "Airlock";
			airInfo.pos = { 0,-0.74, 3.5 };
			airInfo.dir = { 0,0,-1 };
			airInfo.rot = { -1,0,0 };
			airInfo.hDock = CreateDock({ 0,-1,-1 }, { 0,-1,0 }, { 0,0,-1 });
			airInfo.gndInfo.pos = { 4,0,-1.3 };

			vslAstrInfo.airlocks.push_back(airInfo);
			vslAstrInfo.stations.emplace_back("Pilot");

			UACS::SlotInfo slotInfo;

			slotInfo.hAttach = CreateAttachment(false, { 0,1.3,-1 }, { 0,1,0 }, { 0,0,1 }, "UACS");
			slotInfo.holdDir = { 0, -1, 0 };
			slotInfo.relVel = 0.05;
			slotInfo.gndInfo.pos = { -4,0,-1.3 };

			vslCargoInfo.slots.push_back(slotInfo);

			// Specs
			SetSize(3.5);
			SetEmptyMass(500);
			SetPMI({ 2.28,2.31,0.79 });
			SetCrossSections({ 10.5,15.0,5.8 });
			SetRotDrag({ 0.025,0.025,0.02 });

			AddMesh("ShuttlePB");
			SetCameraOffset({ 0, 0.8, 0 });

			static std::array<TOUCHDOWNVTX, 12> tdVtx
			{ {
			{ {  0,   -1.5,  2 },   2e4, 1e3, 1.6, 1},
			{ { -1,   -1.5, -1.5 }, 2e4, 1e3, 3,   1},
			{ {  1,   -1.5, -1.5 }, 2e4, 1e3, 3,   1},
			{ { -0.5, -0.75, 3 },   2e4, 1e3, 3},
			{ {  0.5, -0.75, 3 },   2e4, 1e3, 3},
			{ { -2.6, -1.1, -1.9 }, 2e4, 1e3, 3},
			{ {  2.6, -1.1, -1.9 }, 2e4, 1e3, 3},
			{ { -1,    1.3,  0 },   2e4, 1e3, 3},
			{ {  1,    1.3,  0 },   2e4, 1e3, 3},
			{ { -1,    1.3, -2 },   2e4, 1e3, 3},
			{ {  1,    1.3, -2 },   2e4, 1e3, 3},
			{ {  0,    0.3, -3.8 }, 2e4, 1e3, 3}
			} };
			SetTouchdownPoints(tdVtx.data(), tdVtx.size());

			// airfoil definitions
			CreateAirfoil3(LIFT_VERTICAL, { 0,0,0 }, vlift, nullptr, 2, 2, 2.5);
			CreateAirfoil3(LIFT_HORIZONTAL, { 0,0,0 }, hlift, nullptr, 2, 1.5, 2);

			// control surface animations
			UINT anim_Laileron = CreateAnimation(0.5);
			UINT anim_Raileron = CreateAnimation(0.5);
			UINT anim_elevator = CreateAnimation(0.5);
			AddAnimationComponent(anim_Laileron, 0, 1, &trans_Laileron);
			AddAnimationComponent(anim_Raileron, 0, 1, &trans_Raileron);
			AddAnimationComponent(anim_elevator, 0, 1, &trans_Lelevator);
			AddAnimationComponent(anim_elevator, 0, 1, &trans_Relevator);

			// aerodynamic control surface defintions
			CreateControlSurface(AIRCTRL_ELEVATOR, 1.5, 0.7, { 0, 0, -2.5 }, AIRCTRL_AXIS_XPOS, anim_elevator);
			CreateControlSurface(AIRCTRL_AILERON, 1.5, 0.25, { 1, 0, -2.5 }, AIRCTRL_AXIS_XPOS, anim_Laileron);
			CreateControlSurface(AIRCTRL_AILERON, 1.5, 0.25, { -1, 0, -2.5 }, AIRCTRL_AXIS_XNEG, anim_Raileron);

			// propellant resources
			PROPELLANT_HANDLE hpr = CreatePropellantResource(750);

			THRUSTER_HANDLE th_main, th_hover, th_rcs[14], th_group[4];

			// main engine
			th_main = CreateThruster({ 0, 0, -4.35 }, { 0, 0, 1 }, 3e4, hpr, 5e4);
			CreateThrusterGroup(&th_main, 1, THGROUP_MAIN);
			AddExhaust(th_main, 8, 1, { 0, 0.3, -4.35 }, { 0, 0, -1 });

			PARTICLESTREAMSPEC contrail_main
			{
				0, 5.0, 16, 200, 0.15, 1.0, 5, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
				PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
				PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
			};

			PARTICLESTREAMSPEC exhaust_main
			{
				0, 2.0, 20, 200, 0.05, 0.1, 8, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
				PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
				PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
			};

			AddExhaustStream(th_main, { 0, 0.3, -10 }, &contrail_main);
			AddExhaustStream(th_main, { 0, 0.3, -5 }, &exhaust_main);

			// hover engine
			th_hover = CreateThruster({ 0, -1.5, 0 }, { 0, 1, 0 }, 1.5e4, hpr, 5e4);
			CreateThrusterGroup(&th_hover, 1, THGROUP_HOVER);
			AddExhaust(th_hover, 8, 1, { 0, -1.5, 1 }, { 0, -1, 0 });
			AddExhaust(th_hover, 8, 1, { 0, -1.5, -1 }, { 0, -1, 0 });

			PARTICLESTREAMSPEC contrail_hover
			{
				0, 5.0, 8, 200, 0.15, 1.0, 5, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
				PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
				PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
			};
			PARTICLESTREAMSPEC exhaust_hover
			{
				0, 2.0, 10, 200, 0.05, 0.05, 8, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
				PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
				PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
			};

			AddExhaustStream(th_hover, { 0, -3, 1 }, &contrail_hover);
			AddExhaustStream(th_hover, { 0, -3, -1 }, &contrail_hover);
			AddExhaustStream(th_hover, { 0, -2, 1 }, &exhaust_hover);
			AddExhaustStream(th_hover, { 0, -2, -1 }, &exhaust_hover);

			// RCS engines
			th_rcs[0] = CreateThruster({ 1, 0, 3 }, { 0, 1, 0 }, 2e2, hpr, 5e4);
			th_rcs[1] = CreateThruster({ 1, 0, 3 }, { 0, -1, 0 }, 2e2, hpr, 5e4);
			th_rcs[2] = CreateThruster({ -1, 0, 3 }, { 0, 1, 0 }, 2e2, hpr, 5e4);
			th_rcs[3] = CreateThruster({ -1, 0, 3 }, { 0, -1, 0 }, 2e2, hpr, 5e4);
			th_rcs[4] = CreateThruster({ 1, 0, -3 }, { 0, 1, 0 }, 2e2, hpr, 5e4);
			th_rcs[5] = CreateThruster({ 1, 0, -3 }, { 0, -1, 0 }, 2e2, hpr, 5e4);
			th_rcs[6] = CreateThruster({ -1, 0, -3 }, { 0, 1, 0 }, 2e2, hpr, 5e4);
			th_rcs[7] = CreateThruster({ -1, 0, -3 }, { 0, -1, 0 }, 2e2, hpr, 5e4);
			th_rcs[8] = CreateThruster({ 1, 0, 3 }, { -1, 0, 0 }, 2e2, hpr, 5e4);
			th_rcs[9] = CreateThruster({ -1, 0, 3 }, { 1, 0, 0 }, 2e2, hpr, 5e4);
			th_rcs[10] = CreateThruster({ 1, 0, -3 }, { -1, 0, 0 }, 2e2, hpr, 5e4);
			th_rcs[11] = CreateThruster({ -1, 0, -3 }, { 1, 0, 0 }, 2e2, hpr, 5e4);
			th_rcs[12] = CreateThruster({ 0, 0, -3 }, { 0, 0, 1 }, 2e2, hpr, 5e4);
			th_rcs[13] = CreateThruster({ 0, 0, 3 }, { 0, 0, -1 }, 2e2, hpr, 5e4);

			th_group[0] = th_rcs[0];
			th_group[1] = th_rcs[2];
			th_group[2] = th_rcs[5];
			th_group[3] = th_rcs[7];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_PITCHUP);

			th_group[0] = th_rcs[1];
			th_group[1] = th_rcs[3];
			th_group[2] = th_rcs[4];
			th_group[3] = th_rcs[6];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_PITCHDOWN);

			th_group[0] = th_rcs[0];
			th_group[1] = th_rcs[4];
			th_group[2] = th_rcs[3];
			th_group[3] = th_rcs[7];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_BANKLEFT);

			th_group[0] = th_rcs[1];
			th_group[1] = th_rcs[5];
			th_group[2] = th_rcs[2];
			th_group[3] = th_rcs[6];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_BANKRIGHT);

			th_group[0] = th_rcs[0];
			th_group[1] = th_rcs[4];
			th_group[2] = th_rcs[2];
			th_group[3] = th_rcs[6];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_UP);

			th_group[0] = th_rcs[1];
			th_group[1] = th_rcs[5];
			th_group[2] = th_rcs[3];
			th_group[3] = th_rcs[7];
			CreateThrusterGroup(th_group, 4, THGROUP_ATT_DOWN);

			th_group[0] = th_rcs[8];
			th_group[1] = th_rcs[11];
			CreateThrusterGroup(th_group, 2, THGROUP_ATT_YAWLEFT);

			th_group[0] = th_rcs[9];
			th_group[1] = th_rcs[10];
			CreateThrusterGroup(th_group, 2, THGROUP_ATT_YAWRIGHT);

			th_group[0] = th_rcs[8];
			th_group[1] = th_rcs[10];
			CreateThrusterGroup(th_group, 2, THGROUP_ATT_LEFT);

			th_group[0] = th_rcs[9];
			th_group[1] = th_rcs[11];
			CreateThrusterGroup(th_group, 2, THGROUP_ATT_RIGHT);

			CreateThrusterGroup(th_rcs + 12, 1, THGROUP_ATT_FORWARD);
			CreateThrusterGroup(th_rcs + 13, 1, THGROUP_ATT_BACK);
		}

		void Carrier::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			char* line;

			while (oapiReadScenario_nextline(scn, line)) 
			{
				std::istringstream ss(line);
				std::string data;

				if (ss >> data && data == "HUD_MODE") ss >> hudMode;

				else if (!mdlAPI.ParseScenarioLine(line)) ParseScenarioLineEx(line, status);
			}
		}

		void Carrier::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);
			mdlAPI.clbkSaveState(scn);

			oapiWriteScenario_int(scn, "HUD_MODE", hudMode);
		}

		void Carrier::clbkPostCreation()
		{
			mdlAPI.clbkPostCreation();

			SetEmptyMass(GetEmptyMass() + mdlAPI.GetTotalAstrMass());
		}

		int Carrier::clbkGeneric(int msgid, int prm, void* context)
		{
			if (msgid == UACS::MSG)
			{
				switch (prm)
				{
				case UACS::ASTR_INGRS:
				{
					auto& astrInfo = *(vslAstrInfo.stations.at(*static_cast<size_t*>(context)).astrInfo);
					SetEmptyMass(GetEmptyMass() + astrInfo.mass);

					return 1;
				}
				case UACS::ASTR_EGRS:
				{
					auto& astrInfo = *(vslAstrInfo.stations.at(*static_cast<size_t*>(context)).astrInfo);
					SetEmptyMass(GetEmptyMass() - astrInfo.mass);

					return 1;
				}
				default:
					return 0;
				}
			}
			
			return 0;
		}

		int Carrier::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
		{
			if (!down) return 0;

			if (KEYMOD_ALT(kstate) && key == OAPI_KEY_M)
			{
				hudMode < 2 ? ++hudMode : hudMode = 0;
				return 1;
			}
			else if (hudMode != HUD_OP) return 0;

			if (KEYMOD_ALT(kstate))
			{
				switch (key)
				{
				case OAPI_KEY_NUMPAD8:
					astrHUD.idx + 1 < mdlAPI.GetAvailAstrCount() ? ++astrHUD.idx : astrHUD.idx = 0;
					return 1;

				case OAPI_KEY_NUMPAD2:
					astrHUD.idx > 0 ? --astrHUD.idx : astrHUD.idx = mdlAPI.GetAvailAstrCount() - 1;
					return 1;

				case OAPI_KEY_NUMPAD6:
					cargoHUD.idx + 1 < mdlAPI.GetAvailCargoCount() ? ++cargoHUD.idx : cargoHUD.idx = 0;
					return 1;

				case OAPI_KEY_NUMPAD4:
					cargoHUD.idx > 0 ? --cargoHUD.idx : cargoHUD.idx = mdlAPI.GetAvailCargoCount() - 1;
					return 1;
				}
			}

			if (KEYMOD_RALT(kstate))
			{
				switch (key)
				{
				case OAPI_KEY_A:
					switch (mdlAPI.AddAstronaut(astrHUD.idx))
					{
					case UACS::INGRS_SUCCED:
						astrHUD.msg = "Success: Selected astronaut added.";
						break;

					case UACS::INGRS_STN_OCCP:
						astrHUD.msg = "Error: Station occupied.";
						break;

					case UACS::INGRS_FAIL:
						astrHUD.msg = "Error: The addition failed.";
						break;
					}
					astrHUD.timer = 0;
					return 1;

				case OAPI_KEY_E:
				{
					switch (mdlAPI.EgressAstronaut())
					{
					case UACS::EGRS_SUCCED:
						astrHUD.msg = "Success: Astronaut egressed.";
						break;

					case UACS::EGRS_STN_EMPTY:
						astrHUD.msg = "Error: No astronaut onboard.";
						break;

					case UACS::EGRS_ARLCK_DCKD:
						astrHUD.msg = "Error: Airlock blocked by a docked vessel.";
						break;

					case UACS::EGRS_NO_EMPTY_POS:
						astrHUD.msg = "Error: No empty position nearby.";
						break;

					case UACS::EGRS_INFO_NOT_SET:
						astrHUD.msg = "Error: Astronaut egressed but info not set.";
						break;

					case UACS::EGRS_FAIL:
						astrHUD.msg = "Error: The egress failed.";
						break;
					}
					astrHUD.timer = 0;
					return 1;
				}

				case OAPI_KEY_T:
					switch (mdlAPI.TransferAstronaut())
					{
					case UACS::TRNS_SUCCED:
						astrHUD.msg = "Success: Astronaut transfered.";
						break;
					case UACS::TRNS_STN_EMPTY:
						astrHUD.msg = "Error: No astronaut onboard.";
						break;

					case UACS::TRNS_DOCK_EMPTY:
						astrHUD.msg = "Error: No docked vessel.";
						break;

					case UACS::TRNS_TGT_ARLCK_UNDEF:
						astrHUD.msg = "Error: No airlock connected to docking port.";
						break;

					case UACS::TRNS_TGT_ARLCK_CLSD:
						astrHUD.msg = "Error: Docked vessel airlock closed.";
						break;

					case UACS::TRNS_TGT_STN_UNDEF:
						astrHUD.msg = "Error: No stations in docked vessel.";
						break;

					case UACS::TRNS_TGT_STN_OCCP:
						astrHUD.msg = "Error: All docked vessel stations occupied.";
						break;

					case UACS::TRNS_FAIL:
						astrHUD.msg = "Error: The transfer failed.";
						break;
					}
					astrHUD.timer = 0;
					return 1;

				case OAPI_KEY_D:
					vslAstrInfo.stations.front() = {};
					astrHUD.msg = "Success: Astronaut deleted.";

					astrHUD.timer = 0;
					return 1;
				}
			}

			else if (KEYMOD_LALT(kstate))
			{
				switch (key)
				{
				case OAPI_KEY_A:
					switch (mdlAPI.AddCargo(cargoHUD.idx))
					{
					case UACS::GRPL_SUCCED:
						cargoHUD.msg = "Success: Selected cargo added.";
						break;

					case UACS::GRPL_SLT_OCCP:
						cargoHUD.msg = "Error: Slot occupied.";
						break;

					case UACS::GRPL_FAIL:
						cargoHUD.msg = "Error: The addition failed.";
						break;
					}
					cargoHUD.timer = 0;
					return 1;

				case OAPI_KEY_G:
					switch (mdlAPI.GrappleCargo())
					{
					case UACS::GRPL_SUCCED:
						cargoHUD.msg = "Success: Nearest cargo grappled.";
						break;

					case UACS::GRPL_SLT_OCCP:
						cargoHUD.msg = "Error: Slot occupied.";
						break;

					case UACS::GRPL_NOT_IN_RNG:
						cargoHUD.msg = "Error: No grappleable cargo in range.";
						break;

					case UACS::GRPL_FAIL:
						cargoHUD.msg = "Error: The grapple failed.";
						break;
					}
					cargoHUD.timer = 0;
					return 1;

				case OAPI_KEY_R:
					switch (mdlAPI.ReleaseCargo())
					{
					case UACS::RLES_SUCCED:
						cargoHUD.msg = "Success: Cargo released.";
						break;

					case UACS::RLES_SLT_EMPTY:
						cargoHUD.msg = "Error: Slot empty.";
						break;

					case UACS::RLES_NO_EMPTY_POS:
						cargoHUD.msg = "Error: No empty position nearby.";
						break;

					case UACS::RLES_FAIL:
						cargoHUD.msg = "Error: The release failed.";
						break;
					}
					cargoHUD.timer = 0;
					return 1;

				case OAPI_KEY_P:
					switch (mdlAPI.PackCargo())
					{
					case UACS::PACK_SUCCED:
						cargoHUD.msg = "Success: Nearest cargo packed.";
						break;

					case UACS::PACK_NOT_IN_RNG:
						cargoHUD.msg = "Error: No packable cargo in range.";
						break;

					case UACS::PACK_FAIL:
						cargoHUD.msg = "Error: The packing failed.";
						break;
					}
					cargoHUD.timer = 0;
					return 1;

				case OAPI_KEY_U:
					switch (mdlAPI.UnpackCargo())
					{
					case UACS::PACK_SUCCED:
						cargoHUD.msg = "Success: Nearest cargo unpacked.";
						break;

					case UACS::PACK_NOT_IN_RNG:
						cargoHUD.msg = "Error: No unpackable cargo in range.";
						break;

					case UACS::PACK_FAIL:
						cargoHUD.msg = "Error: The unpacking failed.";
						break;
					}
					cargoHUD.timer = 0;
					return 1;

				case OAPI_KEY_F:
				{
					double reqMass = GetMaxFuelMass() - GetFuelMass();

					auto drainInfo = mdlAPI.DrainGrappledResource("fuel", reqMass);

					if (drainInfo.first != UACS::DRIN_SUCCED) drainInfo = mdlAPI.DrainScenarioResource("fuel", reqMass);

					if (drainInfo.first != UACS::DRIN_SUCCED) drainInfo = mdlAPI.DrainStationResource("fuel", reqMass);

					switch (drainInfo.first)
					{
					case UACS::DRIN_SUCCED:
						SetFuelMass(GetFuelMass() + drainInfo.second);
						cargoHUD.msg = std::format("Success: {:g}kg drained.", drainInfo.second);
						break;

					case UACS::DRIN_NOT_IN_RNG:
						cargoHUD.msg = "Error: No resource vessel grappled or in range.";;
						break;

					case UACS::DRIN_FAIL:
						cargoHUD.msg = "Error: The drainage failed.";
						break;
					}

					cargoHUD.timer = 0;
					return 1;
				}
				case OAPI_KEY_D:
					switch (mdlAPI.DeleteCargo())
					{
					case UACS::RLES_SUCCED:
						cargoHUD.msg = "Success: Cargo deleted.";
						break;

					case UACS::RLES_SLT_EMPTY:
						cargoHUD.msg = "Error: Slot empty.";
						break;

					case UACS::RLES_FAIL:
						cargoHUD.msg = "Error: The deletion failed.";
						break;

					default: break;
					}
					cargoHUD.timer = 0;
					return 1;
				}
			}

			return 0;
		}

		void Carrier::clbkPreStep(double simt, double simdt, double mjd)
		{
			if (astrHUD.timer < 5) astrHUD.timer += simdt;
			if (cargoHUD.timer < 5) cargoHUD.timer += simdt;

			if (!GetFlightStatus() && GroundContact() && GetGroundspeed() <= 0.2)
			{
				VECTOR3 thrustVector;

				if (!GetThrustVector(thrustVector))
				{
					VESSELSTATUS2 status = GetVesselStatus(this);
					status.status = 1;

					SetGroundRotation(status, 1.1);
					DefSetStateEx(&status);
				}
			}
		}

		bool Carrier::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
		{
			VESSEL4::clbkDrawHUD(mode, hps, skp);

			int x = HIWORD(skp->GetCharSize());
			int rightX = hps->W - x;
			int startY = int(0.215 * hps->H);
			int y = startY;

			int space = LOWORD(skp->GetCharSize());
			int largeSpace = int(1.5 * space);

			if (hudMode == HUD_OP)
			{
				buffer = std::format("Selected available cargo: {}", mdlAPI.GetAvailCargoName(cargoHUD.idx));
				skp->Text(x, y, buffer.c_str(), buffer.size());

				if (cargoHUD.timer < 5) { y += largeSpace; skp->Text(x, y, cargoHUD.msg.c_str(), cargoHUD.msg.size()); }

				if (const auto& info = vslCargoInfo.slots.front().cargoInfo)
				{
					const auto& cargoInfo = *info;

					y += largeSpace;
					skp->Text(x, y, "Grappled cargo information", 26);
					y += largeSpace;

					buffer = std::format("Name: {}", oapiGetVesselInterface(cargoInfo.handle)->GetName());
					skp->Text(x, y, buffer.c_str(), buffer.size());

					y += space;

					buffer = std::format("Mass: {:g}kg", oapiGetMass(cargoInfo.handle));
					skp->Text(x, y, buffer.c_str(), buffer.size());

					y += space;

					switch (cargoInfo.type)
					{
					case UACS::STATIC:
						skp->Text(x, y, "Type: Static", 12);
						break;

					case UACS::UNPACKABLE:
						if (cargoInfo.unpackOnly) skp->Text(x, y, "Type: Unpackable only", 21);
						else skp->Text(x, y, "Type: Unpackable", 16);

						y += space;

						buffer = std::format("Breathable: {}", cargoInfo.breathable ? "Yes" : "No");
						skp->Text(x, y, buffer.c_str(), buffer.size());

						break;
					}

					if (cargoInfo.resource)
					{
						y += space;

						buffer = *cargoInfo.resource;
						buffer[0] = std::toupper(buffer[0]);

						buffer = std::format("Resource: {}", buffer);
						skp->Text(x, y, buffer.c_str(), buffer.size());
					}
				}

				x = rightX;
				y = startY;
				skp->SetTextAlign(oapi::Sketchpad::RIGHT);

				buffer = std::format("Selected available astronaut: {}", mdlAPI.GetAvailAstrName(astrHUD.idx));
				skp->Text(x, y, buffer.c_str(), buffer.size());

				if (astrHUD.timer < 5) { y += largeSpace; skp->Text(x, y, astrHUD.msg.c_str(), astrHUD.msg.size()); }

				if (const auto& info = vslAstrInfo.stations.front().astrInfo)
				{
					const auto& astrInfo = *info;

					y += largeSpace;
					skp->Text(x, y, "Onboard astronaut information", 29);
					y += largeSpace;

					buffer = std::format("Name: {}", astrInfo.name);
					skp->Text(x, y, buffer.c_str(), buffer.size());
					y += space;

					buffer = astrInfo.role;
					buffer[0] = std::toupper(buffer[0]);

					buffer = std::format("Role: {}", buffer);
					skp->Text(x, y, buffer.c_str(), buffer.size());
					y += space;

					buffer = std::format("Mass: {:g}kg", astrInfo.mass);
					skp->Text(x, y, buffer.c_str(), buffer.size());
					y += largeSpace;

					buffer = std::format("Fuel: {:g}%", astrInfo.fuelLvl * 100);
					skp->Text(x, y, buffer.c_str(), buffer.size());
					y += space;

					buffer = std::format("Oxygen: {:g}%", astrInfo.oxyLvl * 100);
					skp->Text(x, y, buffer.c_str(), buffer.size());
					y += space;

					buffer = std::format("Alive: {}", astrInfo.alive ? "Yes" : "No");
					skp->Text(x, y, buffer.c_str(), buffer.size());
				}
			}
			else if (hudMode == HUD_SRT)
			{
				skp->Text(x, y, "Alt + M: Cycle between custom HUD modes", 39);
				y += largeSpace;

				skp->Text(x, y, "Alt + Numpad 6/4: Select next/previous available cargo", 54);
				y += space;

				skp->Text(x, y, "Left Alt + A: Add selected cargo", 32);
				y += space;

				skp->Text(x, y, "Left Alt + G: Grapple nearest cargo", 35);
				y += space;

				skp->Text(x, y, "Left Alt + R: Release grappled cargo", 36);
				y += space;

				skp->Text(x, y, "Left Alt + P: Pack nearest packable cargo", 41);
				y += space;

				skp->Text(x, y, "Left Alt + U: Unpack nearest unpackable cargo", 45);
				y += space;

				skp->Text(x, y, "Left Alt + F: Drain resource from nearest source", 48);
				y += space;

				skp->Text(x, y, "Left Alt + D: Delete grappled cargo", 35);

				x = rightX;
				y = startY;
				skp->SetTextAlign(oapi::Sketchpad::RIGHT);

				skp->Text(x, y, "Alt + Numpad 8/2: Select next/previous available astronaut", 58);
				y += space;

				skp->Text(x, y, "Right Alt + A: Add selected astronaut", 37);
				y += space;

				skp->Text(x, y, "Right Alt + E: Egress onboard astronaut", 39);
				y += space;

				skp->Text(x, y, "Right Alt + T: Transfer onboard astronaut", 41);
				y += space;

				skp->Text(x, y, "Right Alt + D: Delete onboard astronaut", 39);
			}

			return true;
		}

		void Carrier::vlift(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd)
		{
			// lift coefficient from -pi to pi in 10deg steps
			static const double clp[] { -0.1,-0.5,-0.4,-0.1,0,0,0,0,0,0,0,0,0,0,-0.2,-0.6,-0.6,-0.4,0.2,0.5,0.9,0.8,0.2,0,0,0,0,0,0,0,0,0,0.1,0.4,0.5,0.3,-0.1,-0.5 };
			static const double aoa_step = 10 * RAD;
			double a, fidx, saoa = sin(aoa);
			a = modf((aoa + PI) / aoa_step, &fidx);
			int idx = (int)(fidx + 0.5);
			*cl = clp[idx] * (1.0 - a) + clp[idx + 1] * a;     // linear interpolation
			*cm = 0.0;
			*cd = 0.03 + 0.4 * saoa * saoa;                // profile drag
			*cd += oapiGetInducedDrag(*cl, 1.0, 0.5); // induced drag
			*cd += oapiGetWaveDrag(M, 0.75, 1.0, 1.1, 0.04);  // wave drag
		}

		void Carrier::hlift(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd)
		{
			// lift coefficient from -pi to pi in 45deg steps
			static const double clp[] { 0,0.4,0,-0.4,0,0.4,0,-0.4,0,0.4 };
			static const double aoa_step = 45.0 * RAD;
			double a, fidx;
			a = modf((aoa + PI) / aoa_step, &fidx);
			int idx = (int)(fidx + 0.5);
			*cl = clp[idx] * (1.0 - a) + clp[idx + 1] * a;     // linear interpolation
			*cm = 0.0;
			*cd = 0.03;
			*cd += oapiGetInducedDrag(*cl, 1.5, 0.6); // induced drag
			*cd += oapiGetWaveDrag(M, 0.75, 1.0, 1.1, 0.04);  // wave drag
		}
	}
}