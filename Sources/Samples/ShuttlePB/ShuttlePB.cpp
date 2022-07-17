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

ShuttlePB::ShuttlePB(OBJHANDLE hVessel, int flightmodel) : VESSEL4(hVessel, flightmodel), uacs{ UACS::API::Vessel::CreateInstance(this, &astrInfo, &cargoInfo) }
{
	//sprintf(buffer, "UCSO version: %s", uacs->GetUCSOVersion());
	//message = _strdup(buffer);
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

	astrInfo.airlocks.push_back({ "UACS", { 2,-1,-1.3 }, true, CreateDock(PB_DOCK_POS, PB_DOCK_DIR, PB_DOCK_ROT) });
	astrInfo.stations.push_back({ "Pilot" });

	cargoInfo.slots.push_back({ CreateAttachment(false, { 0,-1.65,-1.3 }, { 0,-1,0 }, { 0,0,1 }, "UACS"), true });
}

int ShuttlePB::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (!down) return 0;

	if (KEYMOD_SHIFT(kstate)) // If Shift key is pressed
	{ 
		switch (key)
		{
		case OAPI_KEY_A:
			uacs->TransferAstronaut(0, 0);

			/*switch (uacs->AddCargo(cargoIndex))
			{
			case UACS::API::GRPL_SUCCED:
				message = "The selected cargo was added successfully.";
				break;

			case UACS::API::GRPL_SLT_OCCP:
				message = "Couldn't add the selected cargo: the slot is occupied.";
				break;

			case UACS::API::GRPL_NOT_IN_RNG:
				message = "Couldn't add the selected cargo: the index is invalid.";
				break;

			case UACS::API::GRPL_FAIL:
				message = "Couldn't add the selected cargo.";
				break;

			default: break;
			}*/
			timer = 0;
			return 1;

		case OAPI_KEY_G:
			uacs->EgressAstronaut(0, 0);

			/*switch (uacs->GrappleCargo())
			{
			case UACS::API::GRPL_SUCCED:
				message = "The nearest cargo was grappled successfully.";
				break;

			case UACS::API::GRPL_SLT_OCCP:
				message = "Couldn't grapple cargo: the slot is occupied.";
				break;

			case UACS::API::GRPL_NOT_IN_RNG:
				message = "Couldn't grapple cargo: no grappleable cargo in range.";
				break;

			case UACS::API::GRPL_FAIL:
				message = "Couldn't grapple cargo.";
				break;

			default: break;
			}
			timer = 0;*/
			return 1;

		case OAPI_KEY_R:
			switch (uacs->ReleaseCargo())
			{
			case UACS::API::RLES_SUCCED:
				message = "The grappled cargo was released successfully.";
				break;

			case UACS::API::RLES_SLT_EMPTY:
				message = "Couldn't release the grappled cargo: the slot is empty.";
				break;

			case UACS::API::RLES_NO_EMPTY_POS:
				message = "Couldn't release the grappled cargo: no empty position nearby.";
				break;

			case UACS::API::RLES_FAIL:
				message = "Couldn't release the grappled cargo.";
				break;

			default: break;
			}
			timer = 0;
			return 1;

		case OAPI_KEY_P:
			if (uacs->PackCargo()) message = "The nearest cargo was packed successfully.";
			else message = "Couldn't pack the nearest cargo: no packable cargo in range or the packing failed.";

			timer = 0;
			return 1;

		case OAPI_KEY_U:
			if (uacs->UnpackCargo()) message = "The nearest cargo was unpacked successfully.";
			else message = "Couldn't unpack the nearest cargo: no unpackable cargo in range or the unpacking failed.";

			timer = 0;
			return 1;

		case OAPI_KEY_F:
		{
			double requiredMass = GetMaxFuelMass() - GetFuelMass();

			// Drain the required mass to fill the tank, by subtracting the maximum mass from the current mass
			double drainedMass = uacs->DrainUngrappledResource("fuel", requiredMass).mass;

			if (drainedMass > 0)
			{
				SetFuelMass(GetFuelMass() + drainedMass);
				sprintf(buffer, "%g kilograms of fuel was drained", drainedMass);
				message = _strdup(buffer);
			}
			else message = "Couldn't drain fuel.";

			timer = 0;
			return 1;
		}
		case OAPI_KEY_D:
			switch (uacs->DeleteCargo())
			{
			case UACS::API::RLES_SUCCED:
				message = "The grappled cargo was deleted successfully.";
				break;

			case UACS::API::RLES_SLT_EMPTY:
				message = "Couldn't delete the grappled cargo: the slot is empty.";
				break;

			case UACS::API::RLES_FAIL:
				message = "Couldn't delete the grappled cargo.";
				break;

			default: break;
			}
			timer = 0;
			return 1;

		default: break;
		}
	}

	if (key == OAPI_KEY_S)
	{
		// Reset the index if reached the cargo count, otherwise increase the index
		cargoIndex + 1 < uacs->GetAvailableCargoCount() ? cargoIndex++ : cargoIndex = 0;
		return 1;
	}

	return 0;
}

bool ShuttlePB::clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp)
{
	// Draw the default HUD (Surface, Orbit, etc...)
	VESSEL4::clbkDrawHUD(mode, hps, skp);

	// Determine the screen ratio
	int s = hps->H;
	double d = s * 0.00130208;
	int sw = hps->W;
	int lw = static_cast<int>(16 * sw / 1024);
	int x = 0;
	if (s / sw < 0.7284) x = (lw * 10) + 10;
	int y = static_cast<int>((168 * d) + (-88 * d));

	sprintf(buffer, "Selected cargo to add: %s", uacs->GetAvailableCargoName(cargoIndex).data());
	skp->Text(x, y, buffer, strlen(buffer));
	y += 36;

	skp->Text(x, y, "S = Select a cargo to add", 25);
	y += 20;

	skp->Text(x, y, "Shift + A = Add the selected cargo", 34);
	y += 20;

	skp->Text(x, y, "Shift + G = Grapple the nearest cargo", 37);
	y += 20;

	skp->Text(x, y, "Shift + R = Release the grappled cargo", 38);
	y += 20;

	skp->Text(x, y, "Shift + P = Pack the nearest cargo", 34);
	y += 20;

	skp->Text(x, y, "Shift + U = Unpack the nearest cargo", 36);
	y += 20;

	skp->Text(x, y, "Shift + F = Drain the nearest fuel resource", 43);
	y += 20;

	skp->Text(x, y, "Shift + D = Delete the grappled cargo", 37);

	// Display the message if the timer is below 5
	//if (timer < 5) { y += 36; skp->Text(x, y, message, strlen(message)); }

	/*VESSEL* cargoVessel = uacs->GetCargoInfo(0);
	if (!cargoVessel) return true;

	y += 36;
	skp->Text(x, y, "Grappled cargo information", 26);
	y += 40;

	sprintf(buffer, "Name: %s", cargoVessel->GetName());
	skp->Text(x, y, buffer, strlen(buffer));
	y += 20;

	sprintf(buffer, "Mass: %gkg", cargoVessel->GetMass());
	skp->Text(x, y, buffer, strlen(buffer));*/

	return true;
}

void ShuttlePB::clbkPreStep(double simt, double simdt, double mjd)
{
	if (timer < 5) timer += simdt;

	/*if (GroundContact() && !(GetFlightStatus() & 1) && GetGroundspeed() <= 0.2)
	{
		VECTOR3 thrustVector;

		if(!GetThrustVector(thrustVector)) SetStatusLanded();
	}*/
}

void ShuttlePB::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;

	while (oapiReadScenario_nextline(scn, line)) if (!uacs->ParseScenarioLine(line)) ParseScenarioLineEx(line, status);
}

void ShuttlePB::clbkSaveState(FILEHANDLE scn)
{
	VESSEL4::clbkSaveState(scn);

	uacs->SaveState(scn);
}

void ShuttlePB::SetStatusLanded()
{
	VESSELSTATUS2 status;
	memset(&status, 0, sizeof(status));
	status.version = 2;
	GetStatusEx(&status);
	status.status = 1;

	

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
