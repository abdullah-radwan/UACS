#pragma once
#include "Cargo.h"
#include "..\..\BaseCommon.h"

#include <sstream>
#include <array>
#include <filesystem>

DLLCLBK VESSEL* ovcInit(OBJHANDLE hVessel, int fModel) { return new UACS::Vessel::Cargo(hVessel, fModel); }

DLLCLBK void ovcExit(VESSEL* pVessel) { if (pVessel) delete static_cast<UACS::Vessel::Cargo*>(pVessel); }

namespace UACS
{
	namespace Vessel
	{
		void Cargo::LoadConfig()
		{
			configLoaded = true;

			FILEHANDLE hConfig = oapiOpenFile("UACS.cfg", FILE_IN_ZEROONFAIL, CONFIG);

			if (hConfig)
			{
				if (!oapiReadItem_bool(hConfig, "DisableFocus", disableFocus))
					oapiWriteLog("UACS warning: Couldn't read DisableFocus option from config file, will use default value (TRUE)");

				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
			}

			else oapiWriteLog("UACS warning: Couldn't load config file, will use default config");
		}

		Cargo::Cargo(OBJHANDLE hVessel, int fModel) : UACS::Cargo(hVessel, fModel) { if (!configLoaded) LoadConfig(); }

		void Cargo::clbkSetClassCaps(FILEHANDLE cfg)
		{
			char buffer[512];

			if (!oapiReadItem_string(cfg, "PackedMesh", buffer)) LogTerminate("PackedMesh", GetClassNameA());
			packedMesh = buffer;

			if (!oapiReadItem_float(cfg, "PayloadMass", payloadMass)) LogTerminate("PayloadMass", GetClassNameA());

			if (!oapiReadItem_float(cfg, "ContainerMass", contMass)) LogTerminate("ContainerMass", GetClassNameA());

			int type;
			if (!oapiReadItem_int(cfg, "CargoType", type)) LogTerminate("CargoType", GetClassNameA());
			cargoInfo.type = UACS::CargoType(type);

			if (oapiReadItem_string(cfg, "CargoResource", buffer)) { cargoInfo.resource = buffer; CreatePropellantResource(payloadMass); }

			switch (cargoInfo.type)
			{
			case UACS::UNPACKABLE:
				oapiReadItem_bool(cfg, "UnpackOnly", cargoInfo.unpackOnly);

				if (!oapiReadItem_int(cfg, "UnpackingType", unpackType)) LogTerminate("UnpackingType", GetClassNameA());
				if (unpackType == UnpackType::VESSEL) cargoInfo.unpackOnly = true;

				oapiReadItem_int(cfg, "UnpackingMode", unpackMode);

				if (oapiReadItem_int(cfg, "UnpackedCount", unpackedCount)) cargoInfo.unpackOnly = true;

				if (unpackMode == UnpackMode::DELAYED && !oapiReadItem_int(cfg, "UnpackingDelay", unpackDelay))
					LogTerminate("UnpackingDelay", GetClassNameA()); 

				if (!oapiReadItem_float(cfg, "UnpackedHeight", unpackFrontPos.y))
				{
					if (!oapiReadItem_vec(cfg, "UnpackedFrontPos", unpackFrontPos) || !oapiReadItem_vec(cfg, "UnpackedRightPos", unpackRightPos) ||
						!oapiReadItem_vec(cfg, "UnpackedLeftPos", unpackLeftPos)) LogTerminate("UnpackedHeight", GetClassNameA());
				}

				switch (unpackType)
				{
				case UnpackType::MODULE:
					if (!oapiReadItem_string(cfg, "UnpackedMesh", buffer)) LogTerminate("UnpackedMesh", GetClassNameA());
					unpackedMesh = buffer;

					if (!oapiReadItem_float(cfg, "UnpackedSize", unpackSize)) LogTerminate("UnpackedSize", GetClassNameA());

					oapiReadItem_vec(cfg, "UnpackedAttachPos", unpackAttachPos);

					VECTOR3 cs;
					if (oapiReadItem_vec(cfg, "UnpackedCrossSections", cs)) unpackCS = cs;

					VECTOR3 pmi;
					if (oapiReadItem_vec(cfg, "UnpackedInertia", pmi)) unpackPMI = pmi;

					oapiReadItem_bool(cfg, "UnpackedBreathable", cargoInfo.breathable);

					break;

				case UnpackType::VESSEL:
					if (!oapiReadItem_string(cfg, "UnpackedVesselName", buffer)) LogTerminate("UnpackedVesselName", GetClassNameA());
					unpackVslName = buffer;

					if (!oapiReadItem_string(cfg, "UnpackedVesselModule", buffer)) LogTerminate("UnpackedVesselModule", GetClassNameA());
					unpackVslModule = buffer;

					break;
				}
				break;
			}

			cargoInfo.unpacked = false;

			SetEnableFocus(!disableFocus);

			SetPackedCaps(false);
		}

		void Cargo::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			if (cargoInfo.type != UACS::UNPACKABLE) { VESSEL4::clbkLoadStateEx(scn, status); return; }

			char* line;

			while (oapiReadScenario_nextline(scn, line))
			{
				std::istringstream ss(line);
				std::string data;

				bool read{};

				if (ss >> data)
				{
					if (read = data == "UNPACKED") ss >> cargoInfo.unpacked;

					if (cargoInfo.unpacked) SetUnpackedCaps(false);
					else if (unpackMode == UnpackMode::DELAYED && (read = data == "UNPACK_TIMER")) ss >> unpackTimer;
				}

				if (!read) ParseScenarioLineEx(line, status);
			}
		}

		void Cargo::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);

			if (cargoInfo.type != UACS::UNPACKABLE) return;

			oapiWriteScenario_int(scn, "UNPACKED", cargoInfo.unpacked);

			if (!cargoInfo.unpacked && unpackMode == UnpackMode::DELAYED && !GetAttachmentStatus(cargoInfo.hAttach))
				oapiWriteScenario_float(scn, "UNPACK_TIMER", unpackTimer);
		}

		void Cargo::clbkPreStep(double simt, double simdt, double mjd)
		{
			if (!GetFlightStatus() && GroundContact())
			{
				VECTOR3 angAcc; GetAngularAcc(angAcc);

				if (length(angAcc) < 0.5)
				{
					VESSELSTATUS2 status = GetVesselStatus(this);
					status.status = 1;

					SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);
					DefSetStateEx(&status);
				}
			}

			if (cargoInfo.type != UACS::UNPACKABLE || cargoInfo.unpacked || unpackMode == UnpackMode::MANUAL) return;

			if (GetAttachmentStatus(cargoInfo.hAttach))
			{
				if (unpackMode == UnpackMode::DELAYED) unpackTimer = 0;
			}
			else
			{
				switch (unpackMode)
				{
				case UnpackMode::RELEASED:
					clbkUnpackCargo();
					break;

				case UnpackMode::DELAYED:
					unpackTimer += simdt;
					if (unpackTimer >= unpackDelay) { unpackTimer = 0; clbkUnpackCargo(); }
					break;

				case UnpackMode::LANDED:
					if (GroundContact()) clbkUnpackCargo();
					break;
				}
			}
		}

		const UACS::Cargo::CargoInfo* Cargo::clbkGetCargoInfo() { return &cargoInfo; }

		double Cargo::clbkDrainResource(double mass)
		{
			double fuelMass = GetFuelMass();

			if (!fuelMass) return 0;

			double drainedMass;

			// If the cargo net mass is lower than or equal to the required mass
			if (fuelMass - mass >= 0) drainedMass = mass;

			// If the required mass is higher than the available mass, use the full mass
			else drainedMass = fuelMass;

			SetFuelMass(fuelMass - drainedMass);

			return drainedMass;
		}

		bool Cargo::clbkPackCargo()
		{
			cargoInfo.unpacked = false;

			SetPackedCaps();

			return true;
		}

		bool Cargo::clbkUnpackCargo() { return UnpackCargo(); }

		bool Cargo::UnpackCargo(bool firstUnpack)
		{
			if (unpackType == UnpackType::VESSEL)
			{
				VESSELSTATUS2 status = GetVesselStatus(this);

				if (status.status) SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);

				OBJHANDLE hCargo{};

				for (int cargo{}; cargo < unpackedCount; ++cargo)
				{
					std::string spawnName = unpackVslName;
					SetSpawnName(spawnName);

					hCargo = oapiCreateVesselEx(spawnName.c_str(), unpackVslModule.c_str(), &status);

					if (!hCargo) return false;
				}

				oapiDeleteVessel(GetHandle(), hCargo);

				return true;
			}

			cargoInfo.unpacked = true;

			SetUnpackedCaps();

			if (!firstUnpack || !cargoInfo.unpackOnly) return true;

			VESSELSTATUS2 status = GetVesselStatus(this);

			for (int cargo{}; cargo < unpackedCount; ++cargo)
			{
				std::string spawnName = std::filesystem::path(GetClassNameA()).filename().string();
				SetSpawnName(spawnName);

				OBJHANDLE hCargo = oapiCreateVesselEx(spawnName.c_str(), GetClassNameA(), &status);

				if (!hCargo || !static_cast<Cargo*>(oapiGetVesselInterface(hCargo))->UnpackCargo(false)) return false;
			}

			oapiDeleteVessel(GetHandle());

			return true;
		}

		void Cargo::SetPackedCaps(bool init)
		{
			if (cargoInfo.unpacked) return;

			VESSELSTATUS2 status;

			// If Orbiter is initiated, which means the cargo was unpacked (not loaded from scenario)
			if (init)
			{
				status = GetVesselStatus(this);

				SetAttachmentParams(cargoInfo.hAttach, { 0, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 });
			}
			else cargoInfo.hAttach = CreateAttachment(true, { 0, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, "UACS");

			// Replace the unpacked mesh with the packed mesh
			InsertMesh(packedMesh.c_str(), 0);

			if (cargoInfo.resource) SetEmptyMass(contMass);
			else SetEmptyMass((payloadMass * unpackedCount) + contMass);

			SetSize(0.65);

			SetCrossSections({ 1.69, 1.69, 1.69 });

			SetPMI({ 0.28, 0.28, 0.28 });

			cargoInfo.frontPos = { 0, -0.65, 0.65 };
			cargoInfo.rightPos = { 0.65, -0.65, -0.65 };
			cargoInfo.leftPos = { -0.65, -0.65, -0.65 };

			double stiffness = 100 * GetMass();
			double damping = 2 * sqrt(GetMass() * stiffness);

			std::array<TOUCHDOWNVTX, 7> tdVtx
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

			// If the cargo was unpacked and it is landed
			if (init && status.status) 
			{
				SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);
				DefSetStateEx(&status);
			}
		}

		void Cargo::SetUnpackedCaps(bool init)
		{
			VESSELSTATUS2 status;

			if (init)
			{
				status = GetVesselStatus(this);

				SetAttachmentParams(cargoInfo.hAttach, unpackAttachPos, { 0, -1, 0 }, { 0, 0, 1 });
			}

			else cargoInfo.hAttach = CreateAttachment(true, unpackAttachPos, { 0, -1, 0 }, { 0, 0, 1 }, "UACS");

			InsertMesh(unpackedMesh.c_str(), 0);

			SetSize(unpackSize);

			SetEmptyMass(cargoInfo.resource ? 0 : payloadMass);

			double stiffness = 100 * GetMass();
			double damping = 2 * sqrt(GetMass() * stiffness);

			if (unpackFrontPos.z || unpackRightPos.x || unpackLeftPos.x)
			{
				std::array<TOUCHDOWNVTX, 4> tdVtx
				{ {
				{ unpackFrontPos, stiffness, damping, 3, 3},
				{ unpackLeftPos, stiffness, damping, 3, 3},
				{ unpackRightPos, stiffness, damping, 3, 3},
				{ { 0, unpackSize, 0 }, stiffness, damping, 3, 3}
				} };

				SetTouchdownPoints(tdVtx.data(), tdVtx.size());
			}

			else
			{
				double sizeSin = -sin(30 * RAD) * unpackSize;
				double sizeCos = cos(30 * RAD) * unpackSize;

				std::array<TOUCHDOWNVTX, 4> tdVtx
				{ {
				{ { sizeCos, -abs(unpackFrontPos.y), sizeSin}, stiffness, damping, 3, 3},
				{ { 0, -abs(unpackFrontPos.y), unpackSize }, stiffness, damping, 3, 3},
				{ { -sizeCos, -abs(unpackFrontPos.y), sizeSin }, stiffness, damping, 3, 3},
				{ { 0, 15 * unpackSize, 0 }, stiffness, damping, 3, 3}
				} };

				SetTouchdownPoints(tdVtx.data(), tdVtx.size());
			}

			if (unpackCS) SetCrossSections(*unpackCS);
			if (unpackPMI) SetPMI(*unpackPMI);

			cargoInfo.frontPos = unpackFrontPos;
			cargoInfo.rightPos = unpackRightPos;
			cargoInfo.leftPos = unpackLeftPos;

			if (init && status.status) 
			{ 
				SetGroundRotation(status, cargoInfo.frontPos, cargoInfo.rightPos, cargoInfo.leftPos);				
				DefSetStateEx(&status);
			}
		}
	}
}