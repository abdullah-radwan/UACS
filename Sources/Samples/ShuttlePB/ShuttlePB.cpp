// ==============================================================
//                 ORBITER MODULE: ShuttlePB
//                  Part of the ORBITER SDK
//          Copyright (C) 2002-2004 Martin Schweiger
//                   All rights reserved
//          UCSO integration by Abdullah Radwan
//
// ShuttlePB.cpp
// Control module for ShuttlePB vessel class
//
// ==============================================================

#include "ShuttlePB.h"

// ==============================================================
// Overloaded callback functions
// ==============================================================

ShuttlePB::ShuttlePB(OBJHANDLE hVessel, int flightmodel) : VESSEL4(hVessel, flightmodel), uacs(this, &astrInfo, &cargoInfo)
{
	sprintf(buffer, "UACS version: %s", uacs.GetUACSVersion().data());
	cargoMsg = _strdup(buffer);
}

// --------------------------------------------------------------
// Set the capabilities of the vessel class
// --------------------------------------------------------------
void ShuttlePB::clbkSetClassCaps (FILEHANDLE cfg)
{
	THRUSTER_HANDLE th_main, th_hover, th_rcs[14], th_group[4];

	// physical vessel parameters
	SetSize (PB_SIZE);
	SetEmptyMass (PB_EMPTYMASS);
	SetPMI (PB_PMI);
	SetCrossSections (PB_CS);
	SetRotDrag (PB_RD);
	SetTouchdownPoints (tdvtx, ntdvtx);

	// docking port definitions
	

	// airfoil definitions
	CreateAirfoil3 (LIFT_VERTICAL,   PB_COP, vlift, NULL, PB_VLIFT_C, PB_VLIFT_S, PB_VLIFT_A);
	CreateAirfoil3 (LIFT_HORIZONTAL, PB_COP, hlift, NULL, PB_HLIFT_C, PB_HLIFT_S, PB_HLIFT_A);

	// control surface animations
	UINT anim_Laileron = CreateAnimation (0.5);
	UINT anim_Raileron = CreateAnimation (0.5);
	UINT anim_elevator = CreateAnimation (0.5);
	AddAnimationComponent (anim_Laileron, 0, 1, &trans_Laileron);
	AddAnimationComponent (anim_Raileron, 0, 1, &trans_Raileron);
	AddAnimationComponent (anim_elevator, 0, 1, &trans_Lelevator);
	AddAnimationComponent (anim_elevator, 0, 1, &trans_Relevator);

	// aerodynamic control surface defintions
	CreateControlSurface (AIRCTRL_ELEVATOR, 1.5, 0.7, _V( 0,0,-2.5), AIRCTRL_AXIS_XPOS, anim_elevator);
	CreateControlSurface (AIRCTRL_AILERON, 1.5, 0.25, _V( 1,0,-2.5), AIRCTRL_AXIS_XPOS, anim_Laileron);
	CreateControlSurface (AIRCTRL_AILERON, 1.5, 0.25, _V(-1,0,-2.5), AIRCTRL_AXIS_XNEG, anim_Raileron);

	// propellant resources
	PROPELLANT_HANDLE hpr = CreatePropellantResource (PB_FUELMASS);

	// main engine
	th_main = CreateThruster (_V(0,0,-4.35), _V(0,0,1), PB_MAXMAINTH, hpr, PB_ISP);
	CreateThrusterGroup (&th_main, 1, THGROUP_MAIN);
	AddExhaust (th_main, 8, 1, _V(0,0.3,-4.35), _V(0,0,-1));

	PARTICLESTREAMSPEC contrail_main = {
		0, 5.0, 16, 200, 0.15, 1.0, 5, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
		PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
	};
	PARTICLESTREAMSPEC exhaust_main = {
		0, 2.0, 20, 200, 0.05, 0.1, 8, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
		PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
		PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
	};
	AddExhaustStream (th_main, _V(0,0.3,-10), &contrail_main);
	AddExhaustStream (th_main, _V(0,0.3,-5), &exhaust_main);

	// hover engine
	th_hover = CreateThruster (_V(0,-1.5,0), _V(0,1,0), PB_MAXHOVERTH, hpr, PB_ISP);
	CreateThrusterGroup (&th_hover, 1, THGROUP_HOVER);
	AddExhaust (th_hover, 8, 1, _V(0,-1.5,1), _V(0,-1,0));
	AddExhaust (th_hover, 8, 1, _V(0,-1.5,-1), _V(0,-1,0));

	PARTICLESTREAMSPEC contrail_hover = {
		0, 5.0, 8, 200, 0.15, 1.0, 5, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
		PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
	};
	PARTICLESTREAMSPEC exhaust_hover = {
		0, 2.0, 10, 200, 0.05, 0.05, 8, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
		PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
		PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
	};

	AddExhaustStream (th_hover, _V(0,-3, 1), &contrail_hover);
	AddExhaustStream (th_hover, _V(0,-3,-1), &contrail_hover);
	AddExhaustStream (th_hover, _V(0,-2, 1), &exhaust_hover);
	AddExhaustStream (th_hover, _V(0,-2,-1), &exhaust_hover);

	// RCS engines
	th_rcs[ 0] = CreateThruster (_V( 1,0, 3), _V(0, 1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 1] = CreateThruster (_V( 1,0, 3), _V(0,-1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 2] = CreateThruster (_V(-1,0, 3), _V(0, 1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 3] = CreateThruster (_V(-1,0, 3), _V(0,-1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 4] = CreateThruster (_V( 1,0,-3), _V(0, 1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 5] = CreateThruster (_V( 1,0,-3), _V(0,-1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 6] = CreateThruster (_V(-1,0,-3), _V(0, 1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 7] = CreateThruster (_V(-1,0,-3), _V(0,-1,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 8] = CreateThruster (_V( 1,0, 3), _V(-1,0,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[ 9] = CreateThruster (_V(-1,0, 3), _V( 1,0,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[10] = CreateThruster (_V( 1,0,-3), _V(-1,0,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[11] = CreateThruster (_V(-1,0,-3), _V( 1,0,0), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[12] = CreateThruster (_V( 0,0,-3), _V(0,0, 1), PB_MAXRCSTH, hpr, PB_ISP);
	th_rcs[13] = CreateThruster (_V( 0,0, 3), _V(0,0,-1), PB_MAXRCSTH, hpr, PB_ISP);

	th_group[0] = th_rcs[0];
	th_group[1] = th_rcs[2];
	th_group[2] = th_rcs[5];
	th_group[3] = th_rcs[7];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_PITCHUP);

	th_group[0] = th_rcs[1];
	th_group[1] = th_rcs[3];
	th_group[2] = th_rcs[4];
	th_group[3] = th_rcs[6];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_PITCHDOWN);

	th_group[0] = th_rcs[0];
	th_group[1] = th_rcs[4];
	th_group[2] = th_rcs[3];
	th_group[3] = th_rcs[7];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_BANKLEFT);

	th_group[0] = th_rcs[1];
	th_group[1] = th_rcs[5];
	th_group[2] = th_rcs[2];
	th_group[3] = th_rcs[6];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_BANKRIGHT);

	th_group[0] = th_rcs[0];
	th_group[1] = th_rcs[4];
	th_group[2] = th_rcs[2];
	th_group[3] = th_rcs[6];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_UP);

	th_group[0] = th_rcs[1];
	th_group[1] = th_rcs[5];
	th_group[2] = th_rcs[3];
	th_group[3] = th_rcs[7];
	CreateThrusterGroup (th_group, 4, THGROUP_ATT_DOWN);

	th_group[0] = th_rcs[8];
	th_group[1] = th_rcs[11];
	CreateThrusterGroup (th_group, 2, THGROUP_ATT_YAWLEFT);

	th_group[0] = th_rcs[9];
	th_group[1] = th_rcs[10];
	CreateThrusterGroup (th_group, 2, THGROUP_ATT_YAWRIGHT);

	th_group[0] = th_rcs[8];
	th_group[1] = th_rcs[10];
	CreateThrusterGroup (th_group, 2, THGROUP_ATT_LEFT);

	th_group[0] = th_rcs[9];
	th_group[1] = th_rcs[11];
	CreateThrusterGroup (th_group, 2, THGROUP_ATT_RIGHT);

	CreateThrusterGroup (th_rcs+12, 1, THGROUP_ATT_FORWARD);
	CreateThrusterGroup (th_rcs+13, 1, THGROUP_ATT_BACK);

	// camera parameters
	SetCameraOffset (_V(0,0.8,0));

	// associate a mesh for the visual
	AddMesh ("ShuttlePB");

	astrInfo.airlocks.push_back({ "UACS", { 0,-0.75,3.5 }, true, CreateDock(PB_DOCK_POS, PB_DOCK_DIR, PB_DOCK_ROT) });
	astrInfo.stations.push_back({ "Pilot" });

	cargoInfo.slots.push_back({ CreateAttachment(false, { 0,-1.65,-1.3 }, { 0,-1,0 }, { 0,0,1 }, "UACS"), true });
}

void ShuttlePB::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;

	while (oapiReadScenario_nextline(scn, line)) if (!uacs.ParseScenarioLine(line)) ParseScenarioLineEx(line, status);
}

void ShuttlePB::clbkSaveState(FILEHANDLE scn)
{
	VESSEL4::clbkSaveState(scn);

	uacs.SaveState(scn);
}

void ShuttlePB::clbkPostCreation() { uacs.clbkPostCreation(); }

void ShuttlePB::clbkPreStep(double simt, double simdt, double mjd)
{
	if (astrTimer < 5) astrTimer += simdt;
	if (cargoTimer < 5) cargoTimer += simdt;

	if (GroundContact() && !(GetFlightStatus() & 1) && GetGroundspeed() <= 0.2)
	{
		VECTOR3 thrustVector;

		if (!GetThrustVector(thrustVector)) SetStatusLanded();
	}
}

int ShuttlePB::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (!down) return 0;

	if (KEYMOD_ALT(kstate))
	{
		switch (key)
		{
		case OAPI_KEY_NUMPAD8:
			astrIdx + 1 < uacs.GetAvailAstrCount() ? ++astrIdx : astrIdx = 0;
			return 1;

		case OAPI_KEY_NUMPAD2:
			astrIdx > 0 ? --astrIdx : astrIdx = uacs.GetAvailAstrCount() - 1;
			return 1;

		case OAPI_KEY_NUMPAD6:
			cargoIdx + 1 < uacs.GetAvailCargoCount() ? ++cargoIdx : cargoIdx = 0;
			return 1;

		case OAPI_KEY_NUMPAD4:
			cargoIdx > 0 ? --cargoIdx : cargoIdx = uacs.GetAvailCargoCount() - 1;
			return 1;
		}
	}

	if (KEYMOD_RALT(kstate))
	{ 
		switch (key)
		{
		case OAPI_KEY_A:
			switch (uacs.AddAstronaut(astrIdx))
			{
			case UACS::API::INGRS_SUCCED:
				astrMsg = "Success: Selected astronaut added.";
				break;

			case UACS::API::INGRS_STN_OCCP:
				astrMsg = "Error: Station occupied.";
				break;

			case UACS::API::INGRS_FAIL:
				astrMsg = "Error: The addition failed.";
				break;
			}
			astrTimer = 0;
			return 1;

		case OAPI_KEY_E:
			switch (uacs.EgressAstronaut(0, 0))
			{
			case UACS::API::EGRS_SUCCEDED:
				astrMsg = "Success: Astronaut egressed.";
				break;

			case UACS::API::EGRS_STN_EMPTY:
				astrMsg = "Error: No astronaut onboard.";
				break;

			case UACS::API::EGRS_ARLCK_DCKD:
				astrMsg = "Error: Airlock blocked by a docked vessel.";
				break;

			case UACS::API::EGRS_NO_EMPTY_POS:
				astrMsg = "Error: No empty position nearby.";
				break;

			case UACS::API::EGRS_FAIL:
				astrMsg = "Error: The egress failed.";
				break;
			}
			astrTimer = 0;
			return 1;

		case OAPI_KEY_T:
			switch (uacs.TransferAstronaut(0, 0))
			{
			case UACS::API::TRNS_SUCCEDED:
				astrMsg = "Success: Astronaut trasnfered.";
				break;
			case UACS::API::TRNS_STN_EMPTY:
				astrMsg = "Error: No astronaut onboard.";
				break;

			case UACS::API::TRNS_DOCK_EMPTY:
				astrMsg = "Error: No docked vessel.";
				break;

			case UACS::API::TRNS_TGT_ARLCK_UNDEF:
				astrMsg = "Error: No airlock connected to docking port.";
				break;

			case UACS::API::TRNS_TGT_ARLCK_CLSD:
				astrMsg = "Error: Docked vessel airlock closed.";
				break;

			case UACS::API::TRNS_TGT_STN_UNDEF:
				astrMsg = "Error: No stations in docked vessel.";
				break;

			case UACS::API::TRNS_TGT_STN_OCCP:
				astrMsg = "Error: All docked vessel stations occupied.";
				break;

			case UACS::API::TRNS_FAIL:
				astrMsg = "Error: The transfer failed.";
				break;
			}
			astrTimer = 0;
			return 1;

		case OAPI_KEY_D:
			astrInfo.stations.front() = {};
			astrMsg = "Success: Astronaut deleted.";

			astrTimer = 0;
			return 1;
		}
	}

	else if (KEYMOD_LALT(kstate))
	{
		switch (key)
		{
		case OAPI_KEY_A:
			switch (uacs.AddCargo(cargoIdx))
			{
			case UACS::API::GRPL_SUCCED:
				cargoMsg = "Success: Selected cargo added.";
				break;

			case UACS::API::GRPL_SLT_OCCP:
				cargoMsg = "Error: Slot occupied.";
				break;

			case UACS::API::GRPL_FAIL:
				cargoMsg = "Error: The addition failed.";
				break;
			}
			cargoTimer = 0;
			return 1;

		case OAPI_KEY_G:
			switch (uacs.GrappleCargo())
			{
			case UACS::API::GRPL_SUCCED:
				cargoMsg = "Success: Nearest cargo grappled.";
				break;

			case UACS::API::GRPL_SLT_OCCP:
				cargoMsg = "Error: Slot occupied.";
				break;

			case UACS::API::GRPL_NOT_IN_RNG:
				cargoMsg = "Error: No grappleable cargo in range.";
				break;

			case UACS::API::GRPL_FAIL:
				cargoMsg = "Error: The grapple failed.";
				break;
			}
			cargoTimer = 0;
			return 1;

		case OAPI_KEY_R:
			switch (uacs.ReleaseCargo())
			{
			case UACS::API::RLES_SUCCED:
				cargoMsg = "Success: Cargo released.";
				break;

			case UACS::API::RLES_SLT_EMPTY:
				cargoMsg = "Error: Slot empty.";
				break;

			case UACS::API::RLES_NO_EMPTY_POS:
				cargoMsg = "Error: No empty position nearby.";
				break;

			case UACS::API::RLES_FAIL:
				cargoMsg = "Error: The release failed.";
				break;
			}
			cargoTimer = 0;
			return 1;

		case OAPI_KEY_P:
			switch (uacs.PackCargo())
			{
			case UACS::API::PACK_SUCCED:
				cargoMsg = "Success: Nearest cargo packed.";
				break;

			case UACS::API::PACK_NOT_IN_RNG:
				cargoMsg = "Error: No packable cargo in range.";
				break;

			case UACS::API::PACK_FAIL:
				cargoMsg = "Error: The packing failed.";
				break;
			}
			cargoTimer = 0;
			return 1;

		case OAPI_KEY_U:
			switch (uacs.UnpackCargo())
			{
			case UACS::API::PACK_SUCCED:
				cargoMsg = "Success: Nearest cargo unpacked.";
				break;

			case UACS::API::PACK_NOT_IN_RNG:
				cargoMsg = "Error: No unpackable cargo in range.";
				break;

			case UACS::API::PACK_FAIL:
				cargoMsg = "Error: The unpacking failed.";
				break;
			}
			cargoTimer = 0;
			return 1;

		case OAPI_KEY_F:
		{
			double reqMass = GetMaxFuelMass() - GetFuelMass();

			auto drainInfo = uacs.DrainGrappledResource("fuel", reqMass);
			if (drainInfo.first != UACS::API::DRIN_SUCCED) drainInfo = uacs.DrainUngrappledResource("fuel", reqMass);
			if (drainInfo.first != UACS::API::DRIN_SUCCED) drainInfo = uacs.DrainStationResource("fuel", reqMass);

			switch (drainInfo.first)
			{
			case UACS::API::DRIN_SUCCED:
				SetFuelMass(GetFuelMass() + drainInfo.second);

				sprintf(buffer, "Success: %g kg drained.", drainInfo.second);
				cargoMsg = _strdup(buffer);
				break;

			case UACS::API::DRIN_NOT_IN_RNG:
				cargoMsg = "Error: No resource cargo grappled or in range.";;
				break;

			case UACS::API::DRIN_FAIL:
				cargoMsg = "Error: The drainage failed.";
				break;
			}

			cargoTimer = 0;
			return 1;
		}
		case OAPI_KEY_D:
			switch (uacs.DeleteCargo())
			{
			case UACS::API::RLES_SUCCED:
				cargoMsg = "Success: Cargo deleted.";
				break;

			case UACS::API::RLES_SLT_EMPTY:
				cargoMsg = "Error: Slot empty.";
				break;

			case UACS::API::RLES_FAIL:
				cargoMsg = "Error: The deletion failed.";
				break;

			default: break;
			}
			cargoTimer = 0;
			return 1;
		}
	}	 

	return 0;
}

bool ShuttlePB::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
{
	// Draw the default HUD (Surface, Orbit, etc...)
	VESSEL4::clbkDrawHUD(mode, hps, skp);

	int x = 10;
	int y = int(0.215 * hps->H);

	sprintf(buffer, "Selected cargo to add: %s", uacs.GetAvailCargoName(cargoIdx).data());
	skp->Text(x, y, buffer, strlen(buffer));
	y += 30;

	skp->Text(x, y, "Alt + Numpad 6/4 = Select cargo to add", 38);
	y += 20;

	skp->Text(x, y, "Left Alt + A = Add selected cargo", 33);
	y += 20;

	skp->Text(x, y, "Left Alt + G = Grapple nearest cargo", 36);
	y += 20;

	skp->Text(x, y, "Left Alt + R = Release grappled cargo", 37);
	y += 20;

	skp->Text(x, y, "Left Alt + P = Pack nearest cargo", 33);
	y += 20;

	skp->Text(x, y, "Left Alt + U = Unpack nearest cargo", 35);
	y += 20;

	skp->Text(x, y, "Left Alt + F = Drain nearest resource", 37);
	y += 20;

	skp->Text(x, y, "Left Alt + D = Delete grappled cargo", 36);

	if (cargoTimer < 5) { y += 30; skp->Text(x, y, cargoMsg, strlen(cargoMsg)); }	

	if (const auto& info = cargoInfo.slots.front().cargoInfo; info)
	{
		y += 30;

		skp->Text(x, y, "Grappled cargo information", 26);
		y += 30;

		sprintf(buffer, "Name: %s", oapiGetVesselInterface((*info).handle)->GetName());
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

		sprintf(buffer, "Mass: %gkg", oapiGetMass((*info).handle));
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

		switch ((*info).type)
		{
		case UACS::API::STATIC:
			skp->Text(x, y, "Type: Static", 12);
			break;

		case UACS::API::UNPACK_ONLY:
			skp->Text(x, y, "Type: Unpackable only", 21);

			y += 20;
			sprintf(buffer, "Breathable: %s", (*info).breathable ? "Yes" : "No");
			skp->Text(x, y, buffer, strlen(buffer));

			break;

		case UACS::API::PACK_UNPACK:
			skp->Text(x, y, "Type: Packacble and unpackable", 30);

			y += 20;
			sprintf(buffer, "Breathable: %s", (*info).breathable ? "Yes" : "No");
			skp->Text(x, y, buffer, strlen(buffer));

			break;
		}

		if ((*info).resource)
		{
			y += 20;

			sprintf(buffer, "Resource: %s", (*(*info).resource).c_str());
			skp->Text(x, y, buffer, strlen(buffer));
		}
	}

	// Determine the screen ratio
    x = hps->W - 10;
	y = int(0.215 * hps->H);
	skp->SetTextAlign(oapi::Sketchpad::RIGHT);

	sprintf(buffer, "Selected astronaut to add: %s", uacs.GetAvailAstrName(astrIdx).data());
	skp->Text(x, y, buffer, strlen(buffer));
	y += 30;

	skp->Text(x, y, "Alt + Numpad 8/2 = Select astronaut to add", 42);
	y += 20;

	skp->Text(x, y, "Right Alt + A = Add selected astronaut", 38);
	y += 20;

	skp->Text(x, y, "Right Alt + E = Egress astronaut", 32);
	y += 20;

	skp->Text(x, y, "Right Alt + T = Transfer astronaut", 34);
	y += 20;

	skp->Text(x, y, "Right Alt + D = Delete astronaut", 32);

	if (astrTimer < 5) { y += 30; skp->Text(x, y, astrMsg, strlen(astrMsg)); }

	if (auto info = astrInfo.stations.front().astrInfo; info)
	{
		y += 30;

		skp->Text(x, y, "Onboard astronaut information", 29);
		y += 36;

		sprintf(buffer, "Name: %s", (*info).name.c_str());
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

		sprintf(buffer, "Role: %s", (*info).role.c_str());
		skp->Text(x, y, buffer, strlen(buffer));
		y += 20;

		sprintf(buffer, "Mass: %gkg", (*info).mass);
		skp->Text(x, y, buffer, strlen(buffer));
	}

	return true;
}

void ShuttlePB::SetStatusLanded()
{
	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);
	status.status = 1;

	SetGroundRotation(status, 1.1);

	DefSetStateEx(&status);
}

// ==============================================================
// Airfoil lift/drag functions
// ==============================================================

void ShuttlePB::vlift (VESSEL *v, double aoa, double M, double Re,
	void *context, double *cl, double *cm, double *cd)
{
	static const double clp[] = {  // lift coefficient from -pi to pi in 10deg steps
		-0.1,-0.5,-0.4,-0.1,0,0,0,0,0,0,0,0,0,0,-0.2,-0.6,-0.6,-0.4,0.2,0.5,0.9,0.8,0.2,0,0,0,0,0,0,0,0,0,0.1,0.4,0.5,0.3,-0.1,-0.5
	};
	static const double aoa_step = 10.0*RAD;
	double a, fidx, saoa = sin(aoa);
	a = modf((aoa+PI)/aoa_step, &fidx);
	int idx = (int)(fidx+0.5);
	*cl = clp[idx]*(1.0-a) + clp[idx+1]*a;     // linear interpolation
	*cm = 0.0; //-0.03*sin(aoa-0.1);
	*cd = 0.03 + 0.4*saoa*saoa;                // profile drag
	*cd += oapiGetInducedDrag (*cl, 1.0, 0.5); // induced drag
	*cd += oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);  // wave drag
}

void ShuttlePB::hlift (VESSEL *v, double aoa, double M, double Re,
	void *context, double *cl, double *cm, double *cd)
{
	static const double clp[] = {  // lift coefficient from -pi to pi in 45deg steps
		0,0.4,0,-0.4,0,0.4,0,-0.4,0,0.4
	};
	static const double aoa_step = 45.0*RAD;
	double a, fidx;
	a = modf((aoa+PI)/aoa_step, &fidx);
	int idx = (int)(fidx+0.5);
	*cl = clp[idx]*(1.0-a) + clp[idx+1]*a;     // linear interpolation
	*cm = 0.0;
	*cd = 0.03;
	*cd += oapiGetInducedDrag (*cl, 1.5, 0.6); // induced drag
	*cd += oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);  // wave drag
}

// ==============================================================
// API callback interface
// ==============================================================

// --------------------------------------------------------------
// Vessel initialisation
// --------------------------------------------------------------
DLLCLBK VESSEL *ovcInit (OBJHANDLE hvessel, int flightmodel) { return new ShuttlePB(hvessel, flightmodel); }

// --------------------------------------------------------------
// Vessel cleanup
// --------------------------------------------------------------
DLLCLBK void ovcExit (VESSEL *vessel) { if (vessel) delete (ShuttlePB*) vessel; }
