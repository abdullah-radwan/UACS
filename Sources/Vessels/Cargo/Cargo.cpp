#pragma once
#include "Cargo.h"
#include "..\..\Common.h"

#include <sstream>
#include <array>

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
				if (!oapiReadItem_float(hConfig, "ContainerMass", containerMass))
					oapiWriteLog("UACS cargo warning: Couldn't read the container mass, will use the default mass (85 kg)");

				if (!oapiReadItem_bool(hConfig, "EnableFocus", enableFocus))
					oapiWriteLog("UACS cargo warning: Couldn't read the focus enabling, will use the default setting (FALSE)");

				oapiCloseFile(hConfig, FILE_IN_ZEROONFAIL);
			}

			else oapiWriteLog("UACS cargo warning: Couldn't load the configurations file, will use the default configurations");
		}

		Cargo::Cargo(OBJHANDLE hVessel, int fModel) : API::Cargo(hVessel, fModel) { if (!configLoaded) LoadConfig(); }

		void Cargo::clbkSetClassCaps(FILEHANDLE cfg)
		{
			char buffer[512];

			if (!oapiReadItem_string(cfg, "PackedMesh", buffer)) WarnAndTerminate("mesh", GetClassNameA(), "cargo");
			packedMesh = buffer;

			if (!oapiReadItem_float(cfg, "CargoMass", netMass)) WarnAndTerminate("mass", GetClassNameA(), "cargo");

			int type;
			if (!oapiReadItem_int(cfg, "CargoType", type)) WarnAndTerminate("type", GetClassNameA(), "cargo");
			cargoInfo.type = API::CargoType(type);

			if (oapiReadItem_string(cfg, "CargoResource", buffer)) { cargoInfo.resource = buffer; CreatePropellantResource(netMass); }

			switch (cargoInfo.type)
			{
			case API::UNPACK_ONLY:
				oapiReadItem_int(cfg, "SpawnCount", spawnCount);
				[[fallthrough]];

			case API::PACK_UNPACK:
				if (!oapiReadItem_int(cfg, "UnpackingType", unpackType)) WarnAndTerminate("unpacking type", GetClassNameA(), "cargo");

				switch (unpackType)
				{
				case UnpackType::MODULE:
					if (!oapiReadItem_string(cfg, "UnpackedMesh", buffer)) WarnAndTerminate("unpacked mesh", GetClassNameA(), "cargo");
					unpackedMesh = buffer;

					if (!oapiReadItem_float(cfg, "UnpackedSize", unpackSize)) WarnAndTerminate("unpacked size", GetClassNameA(), "cargo");

					if (!oapiReadItem_float(cfg, "UnpackedHeight", unpackFrontPos.y))
					{
						if (!oapiReadItem_vec(cfg, "UnpackedFrontPos", unpackFrontPos) || !oapiReadItem_vec(cfg, "UnpackedRightPos", unpackRightPos) ||
							!oapiReadItem_vec(cfg, "UnpackedLeftPos", unpackLeftPos)) WarnAndTerminate("unpacked height", GetClassNameA(), "cargo");
					}

					oapiReadItem_vec(cfg, "UnpackedAttachPos", unpackAttachPos);
					oapiReadItem_vec(cfg, "UnpackedCS", unpackCS);
					oapiReadItem_vec(cfg, "UnpackedPMI", unpackPMI);

					oapiReadItem_float(cfg, "ResourceContainerMass", resContMass);
					oapiReadItem_bool(cfg, "Breathable", cargoInfo.breathable);

					break;

				case UnpackType::VESSEL:
					if (!oapiReadItem_int(cfg, "UnpackingMode", unpackMode)) WarnAndTerminate("unpacking mode", GetClassNameA(), "cargo");

					if (unpackMode == UnpackMode::DELAYED && !oapiReadItem_int(cfg, "UnpackingDelay", unpackDelay)) WarnAndTerminate("unpacking delay", GetClassNameA(), "cargo");

					if (!oapiReadItem_string(cfg, "SpawnName", buffer)) WarnAndTerminate("spawn name", GetClassNameA(), "cargo");
					spawnName = buffer;

					if (!oapiReadItem_string(cfg, "SpawnModule", buffer)) WarnAndTerminate("spawn module", GetClassNameA(), "cargo");
					spawnModule = buffer;

					oapiReadItem_float(cfg, "SpawnHeight", unpackFrontPos.y);

					break;
				}
				break;
			}

			cargoInfo.unpacked = false;

			SetEnableFocus(enableFocus);

			SetPackedCaps(false);
		}

		void Cargo::clbkLoadStateEx(FILEHANDLE scn, void* status)
		{
			char* line;

			while (oapiReadScenario_nextline(scn, line))
			{
				std::istringstream ss;
				ss.str(line);
				std::string data;

				if (ss >> data)
				{
					switch (cargoInfo.type)
					{
					case API::UNPACK_ONLY:
					case API::PACK_UNPACK:
						switch (unpackType)
						{
						case UnpackType::MODULE:
							if (data == "UNPACKED") ss >> cargoInfo.unpacked;
							else ParseScenarioLineEx(line, status);

							if (cargoInfo.unpacked) SetUnpackedCaps(false);
							break;

						case UnpackType::VESSEL:
							if (data == "LANDED") ss >> landed;
							else if (data == "TIMING") ss >> timing;
							else if (data == "TIMER") ss >> timer;
							else ParseScenarioLineEx(line, status);

							break;
						}
						break;

					default:
						ParseScenarioLineEx(line, status);
						break;
					}
				}
				else ParseScenarioLineEx(line, status);
			}
		}

		void Cargo::clbkSaveState(FILEHANDLE scn)
		{
			VESSEL4::clbkSaveState(scn);

			switch (cargoInfo.type)
			{
			case API::UNPACK_ONLY:
			case API::PACK_UNPACK:
				switch (unpackType)
				{
				case UnpackType::MODULE:
					oapiWriteScenario_int(scn, "UNPACKED", cargoInfo.unpacked);
					break;

				case UnpackType::VESSEL:
					oapiWriteScenario_int(scn, "LANDED", landed);
					oapiWriteScenario_int(scn, "TIMING", timing);
					oapiWriteScenario_float(scn, "TIMER", timer);

					break;
				}
				break;
			}
		}

		void Cargo::clbkPreStep(double simt, double simdt, double mjd)
		{
			// If not landed but contacted the ground
			if (!GetFlightStatus() && GroundContact())
			{
				VECTOR3 angAcc; GetAngularAcc(angAcc);

				if (length(angAcc) < 0.1)
				{
					VESSELSTATUS2 status = GetVesselStatus(this);
					status.status = 1;

					SetGroundRotation(status, abs(unpackFrontPos.y));
					DefSetStateEx(&status);
				}
			}

			// Don't continue if the cargo is not unpackable or not Orbiter vessel
			if ((cargoInfo.type != API::PACK_UNPACK && cargoInfo.type != API::UNPACK_ONLY) || unpackType != UnpackType::VESSEL) return;

			const bool attached = GetAttachmentStatus(cargoInfo.hAttach);

			const bool released = this->attached && !attached;

			this->attached = attached;

			// Cancel the landing and timing if attached
			if (attached)
			{
				if (landed) landed = false;
				if (timing) { timer = 0; timing = false; }
			}

			else if (released)
			{
				switch (unpackMode)
				{
				case UnpackMode::RELEASED:
					clbkUnpackCargo();
					break;

				case UnpackMode::DELAYED:
					timing = true;
					break;

				case UnpackMode::LANDED:
					landed = true;
					break;
				}
			}

			if (timing)
			{
				timer += simdt;

				if (timer >= unpackDelay) { timer = 0; timing = false; clbkUnpackCargo(); }
			}

			else if (landed && GroundContact()) { landed = false; clbkUnpackCargo(); }
		}

		const API::Cargo::CargoInfo* Cargo::clbkGetCargoInfo() { return &cargoInfo; }

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

		bool Cargo::UnpackCargo(bool once)
		{
			if (unpackType == UnpackType::VESSEL)
			{
				VESSELSTATUS2 status = GetVesselStatus(this);

				if (status.status) SetGroundRotation(status, abs(unpackFrontPos.y));

				OBJHANDLE hCargo{};

				for (int cargo{}; cargo < spawnCount; ++cargo)
				{
					std::string spawnName = this->spawnName;
					SetSpawnName(spawnName);

					hCargo = oapiCreateVesselEx(spawnName.c_str(), spawnModule.c_str(), &status);

					if (!hCargo) return false;

					if (once) break;
				}

				oapiDeleteVessel(GetHandle(), hCargo);

				return true;
			}

			cargoInfo.unpacked = true;

			SetUnpackedCaps();

			if (once || cargoInfo.type != API::UNPACK_ONLY) return true;

			VESSELSTATUS2 status = GetVesselStatus(this);

			for (int cargo{ 1 }; cargo < spawnCount; ++cargo)
			{
				std::string spawnName = GetClassNameA();

				// Remove UACS\Cargoes\ from the class name
				spawnName.erase(0, 13);
				spawnName.insert(0, "Cargo");

				SetSpawnName(spawnName);

				OBJHANDLE hCargo = oapiCreateVesselEx(spawnName.c_str(), GetClassNameA(), &status);

				if (!hCargo) return false;

				if (!static_cast<Cargo*>(oapiGetVesselInterface(hCargo))->UnpackCargo(true)) return false;
			}

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

				SetAttachmentParams(cargoInfo.hAttach, { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 });
			}
			else cargoInfo.hAttach = CreateAttachment(true, { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, "UACS");

			// Replace the unpacked mesh with the packed mesh
			InsertMesh(packedMesh.c_str(), 0);

			SetEmptyMass((netMass * spawnCount) + resContMass + containerMass);

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
			if (init && status.status) { SetGroundRotation(status, 0.65); DefSetStateEx(&status); }
		}

		void Cargo::SetUnpackedCaps(bool init)
		{
			VESSELSTATUS2 status;

			if (init)
			{
				status = GetVesselStatus(this);

				SetAttachmentParams(cargoInfo.hAttach, unpackAttachPos, { 0, 1, 0 }, { 0, 0, 1 });
			}

			else cargoInfo.hAttach = CreateAttachment(true, unpackAttachPos, { 0, 1, 0 }, { 0, 0, 1 }, "UACS");

			InsertMesh(unpackedMesh.c_str(), 0);

			SetSize(unpackSize);

			SetEmptyMass(cargoInfo.resource ? resContMass : netMass);

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

			SetCrossSections(unpackCS);

			SetPMI(unpackPMI);

			cargoInfo.frontPos = unpackFrontPos;
			cargoInfo.rightPos = unpackRightPos;
			cargoInfo.leftPos = unpackLeftPos;

			if (init && status.status) { SetGroundRotation(status, abs(unpackFrontPos.y)); DefSetStateEx(&status); }
		}
	}
}