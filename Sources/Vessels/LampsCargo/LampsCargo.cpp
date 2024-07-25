#include "LampsCargo.h"
#include "../../BaseCommon.h"

#include <sstream>
#include <array>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel) { return new UACS::Vessel::LampsCargo(hvessel, flightmodel); }

DLLCLBK void ovcExit(VESSEL* vessel) { if (vessel) delete static_cast<UACS::Vessel::LampsCargo*>(vessel); }

namespace UACS
{
	namespace Vessel
	{
		LampsCargo::LampsCargo(OBJHANDLE hVessel, int flightmodel) : UACS::Cargo(hVessel, flightmodel)
		{
			cargoInfo.type = UACS::UNPACKABLE;
			cargoInfo.unpackOnly = true;
		}

		void LampsCargo::clbkSetClassCaps(FILEHANDLE cfg)
		{
			SetEnableFocus(false);

			AddBeacon(&beaconStruct.beaconSpec);

			spotLight = static_cast<SpotLight*>(AddSpotLight(spotStruct.pos, spotStruct.dir, spotStruct.range,
				spotStruct.att0, spotStruct.att1, spotStruct.att2, spotStruct.umbra, spotStruct.penumbra,
				spotStruct.diffuse, spotStruct.specular, spotStruct.ambient));

			SetPackedCaps();
		}

		void LampsCargo::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			char* line;

			while (oapiReadScenario_nextline(scn, line))
			{
				std::istringstream ss{ line };
				std::string data;

				if (ss >> data && data == "UNPACKED")
				{
					ss >> cargoInfo.unpacked;
					if (cargoInfo.unpacked) SetUnpackedCaps(false);
				}
				else ParseScenarioLineEx(line, status);
			}
		}

		void LampsCargo::clbkPreStep(double simt, double simdt, double mjd)
		{
			if (!GetFlightStatus() && GroundContact())
			{
				VECTOR3 angAcc; GetAngularAcc(angAcc);

				if (length(angAcc) < 0.5)
				{
					VESSELSTATUS2 status = GetVesselStatus(this);
					status.status = 1;

					UACS::SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);
					DefSetStateEx(&status);
				}
			}
		}

		void LampsCargo::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);

			oapiWriteScenario_int(scn, "UNPACKED", cargoInfo.unpacked);
		}

		const UACS::Cargo::CargoInfo* LampsCargo::clbkGetCargoInfo() { return &cargoInfo; }

		bool LampsCargo::clbkUnpackCargo() { return UnpackCargo(); }

		bool LampsCargo::UnpackCargo(bool firstUnpack)
		{
			cargoInfo.unpacked = true;

			SetUnpackedCaps();

			if (!firstUnpack) return true;

			VESSELSTATUS2 status = GetVesselStatus(this);

			for (size_t cargo{}; cargo < UNPACK_COUNT; ++cargo)
			{
				std::string spawnName = "Lamp";
				UACS::SetSpawnName(spawnName);

				OBJHANDLE hCargo = oapiCreateVesselEx(spawnName.c_str(), GetClassNameA(), &status);

				if (!hCargo || !static_cast<LampsCargo*>(oapiGetVesselInterface(hCargo))->UnpackCargo(false)) return false;
			}

			oapiDeleteVessel(GetHandle());

			return true;
		}

		void LampsCargo::SetPackedCaps()
		{
			if (cargoInfo.unpacked) return;

			spotLight->Activate(false);

			InsertMesh("UACS/Container4", 0);

			SetEmptyMass(PACK_MASS);

			SetSize(0.65);

			cargoInfo.hAttach = CreateAttachment(true, { 0, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, "UACS");

			SetPMI({ 0.28, 0.28, 0.28 });

			SetCrossSections({ 1.69, 1.69, 1.69 });

			cargoInfo.frontPos = { 0, -0.65, 0.65 };
			cargoInfo.rightPos = { 0.65, -0.65, -0.65 };
			cargoInfo.leftPos = { -0.65, -0.65, -0.65 };

			static double stiffness = 100 * PACK_MASS;
			static double damping = 2 * sqrt(PACK_MASS * stiffness);

			static std::array<TOUCHDOWNVTX, 7> tdVtx
			{ {
			{{ 0, -0.65, 0.65 }, stiffness, damping, 3, 3},
			{{ -0.65, -0.65, -0.65 }, stiffness, damping, 3, 3},
			{{ 0.65, -0.65, -0.65 }, stiffness, damping, 3, 3},
			{{ -0.65, 0.65, -0.65 }, stiffness, damping, 3, 3},
			{{ 0.65, 0.65, -0.65 }, stiffness, damping, 3, 3},
			{{ -0.65, -0.65, 0.65 }, stiffness, damping, 3, 3},
			{{ 0.65, -0.65, 0.65 }, stiffness, damping, 3, 3}
			} };

			SetTouchdownPoints(tdVtx.data(), tdVtx.size());
		}

		void LampsCargo::SetUnpackedCaps(bool init)
		{
			VESSELSTATUS2 status;

			if (init) status = GetVesselStatus(this);

			beaconStruct.beaconSpec.active = true;
			spotLight->Activate(true);

			InsertMesh("UACS/Lamp", 0);

			SetSize(4);

			SetEmptyMass(UNPACK_MASS);

			SetPMI({ 1.44, 0.2, 1.56 });

			SetCrossSections({ 0.58, 0.24, 0.6 });

			cargoInfo.frontPos = { -0.014652, -2.90275, 0.551933 };
			cargoInfo.rightPos = { 0.520547, -2.90275, -0.49769 };
			cargoInfo.leftPos = { -0.521277, -2.90275, -0.49769 };

			SetAttachmentParams(cargoInfo.hAttach, { 0.013923, -2.90275, -0.49769 }, { 0, -1, 0 }, { 0, 0, -1 });

			static double stiffness = 100 * UNPACK_MASS;
			static double damping = 2 * sqrt(UNPACK_MASS * stiffness);

			static std::array<TOUCHDOWNVTX, 4> tdVtx
			{ {
			{ { 3.4641, -2.9, -2}, stiffness, damping, 3, 3},
			{ { 0, -2.9, 4 }, stiffness, damping, 3, 3},
			{ { -3.4641, -2.9, -2 }, stiffness, damping, 3, 3},
			{ { 0, 15 * 4, 0 }, stiffness, damping, 3, 3}
			} };

			SetTouchdownPoints(tdVtx.data(), tdVtx.size());

			if (init && status.status)
			{
				UACS::SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);

				DefSetStateEx(&status);
			}
		}
	}
}