#pragma once
#include "..\..\API\Cargo.h"

namespace UACS
{
	namespace Vessel
	{
		class LampCargo : public UACS::Cargo
		{
		public:
			LampCargo(OBJHANDLE hVessel, int flightmodel);

			void clbkSetClassCaps(FILEHANDLE cfg) override;
			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkPreStep(double simt, double simdt, double mjd) override;
			void clbkSaveState(FILEHANDLE scn) override;

			const UACS::Cargo::CargoInfo* clbkGetCargoInfo() override;
			bool clbkUnpackCargo() override;
			bool UnpackCargo(bool firstUnpack = true);

		private:
			constexpr static inline size_t UNPACK_COUNT = 4;
			constexpr static inline double PACK_MASS = 500;
			constexpr static inline double UNPACK_MASS = 100;

			const static inline struct
			{
				const VECTOR3 pos = { 0, 2.55, 0.5 };
				const double tilt = -20 * RAD;
				const VECTOR3 dir = { 0, sin(tilt), cos(tilt) };
				const double range = 1e3;
				const double att0 = 0.001;
				const double att1 = 0;
				const double att2 = 0.005;
				const double umbra = 45 * RAD;
				const double penumbra = PI05;
				const COLOUR4 diffuse = { 1,1,1,0 };
				const COLOUR4 specular = { 1,1,1,0 };
				const COLOUR4 ambient = { 0,0,0,0 };
			} spotStruct;

			struct
			{
				VECTOR3 pos = { 0, 2.55, 0.65 };
				VECTOR3 color = { 1,1,1 };
				BEACONLIGHTSPEC beaconSpec = { BEACONSHAPE_DIFFUSE, &pos, &color, 2, 0.2, 0, 0, 0, false };
			} beaconStruct;

			SpotLight* spotLight{};

			UACS::Cargo::CargoInfo cargoInfo;

			void SetPackedCaps();
			void SetUnpackedCaps(bool init = true);
		};
	}
}