#pragma once
#include "..\..\API\Cargo.h"

namespace UACS
{
	namespace Vessel
	{
		class Cargo : public UACS::Cargo
		{
		public:
			Cargo(OBJHANDLE hVessel, int fModel);

			void clbkSetClassCaps(FILEHANDLE cfg) override;

			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkSaveState(FILEHANDLE scn) override;

			void clbkPreStep(double simt, double simdt, double mjd) override;

			const UACS::Cargo::CargoInfo* clbkGetCargoInfo() override;
			double clbkDrainResource(double mass) override;

			bool clbkPackCargo() override;
			bool clbkUnpackCargo() override;

		private:
			struct UnpackType { enum { MODULE, VESSEL }; };
			struct UnpackMode { enum { MANUAL, RELEASED, DELAYED, LANDED }; };

			inline static bool configLoaded{};
			inline static bool enableFocus{};
			static void LoadConfig();

			UACS::Cargo::CargoInfo cargoInfo;
			std::string packedMesh, unpackedMesh;

			double payloadMass, contMass;

			int unpackType, unpackMode{};
			int unpackDelay; double unpackTimer{};
			int unpackedCount{ 1 };

			std::string unpackVslName, unpackVslModule;
			double unpackSize;

			VECTOR3 unpackFrontPos{};
			VECTOR3 unpackRightPos{};
			VECTOR3 unpackLeftPos{};

			VECTOR3 unpackAttachPos{};
			VECTOR3 unpackPMI{ -1,-1,-1 };
			VECTOR3 unpackCS{ 20,20,20 };

			bool UnpackCargo(bool firstUnpack = true);
			void SetPackedCaps(bool init = true);
			void SetUnpackedCaps(bool init = true);
		};
	}
}