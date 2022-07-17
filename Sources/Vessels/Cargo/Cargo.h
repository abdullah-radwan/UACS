#pragma once
#include "..\..\API\Cargo.h"

namespace UACS
{
	namespace Vessel
	{
		class Cargo : public API::Cargo
		{
		public:
			Cargo(OBJHANDLE hVessel, int fModel);

			void clbkSetClassCaps(FILEHANDLE cfg) override;

			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkSaveState(FILEHANDLE scn) override;

			void clbkPreStep(double simt, double simdt, double mjd) override;

			const API::Cargo::CargoInfo* clbkGetCargoInfo() override;
			double clbkDrainResource(double mass) override;

			bool clbkPackCargo() override;
			bool clbkUnpackCargo() override;

		private:
			struct UnpackType { enum { MODULE, VESSEL }; };

			struct UnpackMode { enum { RELEASED, DELAYED, LANDED, MANUAL }; };

			API::Cargo::CargoInfo cargoInfo;

			std::string packedMesh, unpackedMesh;

			double netMass;
			double resContMass{};

			int unpackType;
			int spawnCount{ 1 };

			std::string spawnName, spawnModule;
			int unpackMode, unpackDelay;
			double unpackSize;

			VECTOR3 unpackFrontPos{};
			VECTOR3 unpackRightPos{};
			VECTOR3 unpackLeftPos{};

			VECTOR3 unpackAttachPos{};
			VECTOR3 unpackPMI{ -1,-1,-1 };
			VECTOR3 unpackCS{ 20,20,20 };

			double timer{};
			bool landed{};
			bool timing{};
			bool attached{};

			bool UnpackCargo(bool once = false);
			void SetPackedCaps(bool init = true);
			void SetUnpackedCaps(bool init = true);
		};
	}
}

