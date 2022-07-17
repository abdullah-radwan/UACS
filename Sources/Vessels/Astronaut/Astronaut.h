#pragma once
#include "..\..\API\Astronaut.h"
#include "..\..\API\Vessel.h"

namespace UACS
{
	namespace Vessel
	{
		class Astronaut : public API::Astronaut
		{
		public:
			Astronaut(OBJHANDLE hVessel, int fModel);
			~Astronaut();

			void clbkSetClassCaps(FILEHANDLE cfg) override;

			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkSaveState(FILEHANDLE scn) override;

			void clbkPostCreation() override;

			void clbkSetAstrInfo(const API::AstrInfo& astrInfo) override;
			const API::AstrInfo* clbkGetAstrInfo() override;

			int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate) override;
			int clbkConsumeDirectKey(char* kstate) override;
				
			void clbkPreStep(double simt, double simdt, double mjd) override;
			bool clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp) override;

		private:
			API::AstrInfo astrInfo;
			std::string mesh;
			PROPELLANT_HANDLE hFuel, hOxy;
			UINT suitMesh, astrMesh;

			std::string buffer;
			bool keyDown{};
			bool suitOn{ true };
			double consumptionRate{ 1 };

			std::unique_ptr<API::Vessel> pVslAPI;
			API::VslCargoInfo cargoInfo;
			SpotLight* spotLight1{};
			SpotLight* spotLight2{};

			enum { HUD_NEAREST = 1, HUD_VESSEL, HUD_CARGO };

			struct HudInfo
			{
				int mode = HUD_NONE;
				oapi::Font* deadFont{};

				std::string message;
				double timer{ 5 };

				std::string modeMsg;
				double modeTimer{ 5 };

				size_t availIdx{};
				bool drainFuel{ true };

				size_t vslIdx{};
				OBJHANDLE hVessel{};

				struct VesselInfo
				{
					const API::VslAstrInfo* info;
					size_t arlckIdx{};
					size_t statIdx{};
				} vslInfo;
			} hudInfo;

			struct SurfaceInfo
			{
				OBJHANDLE ref{};
				double gravityAccel{};
				double steerRatio{};
			} surfInfo;

			struct ValueInfo
			{
				double value{};
				double maxLimit, minLimit, maxSlowLimit, minSlowLimit;
				double maxMinRate, returnRate;
				double maxMinRateConst;
			} lonSpeed, latSpeed, steerAngle;

			double totalRunDist{};

			struct BeaconInfo
			{
				VECTOR3 pos{ 0.1156, -0.0759, 0.1494 };
				VECTOR3 color{ 1,1,1 };
				BEACONLIGHTSPEC spec{ BEACONSHAPE_DIFFUSE, &pos, &color, 0.02, 0.2, 0, 0, 0, false };
			} beacon1Info, beacon2Info;

			struct SpotStruct
			{
				VECTOR3 pos{ 0.1156, -0.0759, 0.1494 };
				const VECTOR3 dir{ 0, 0, 1 };
				const double range{ 400 };
				const double att0{ 0.01 };
				const double att1{};
				const double att2{ 0.05 };
				const double umbra{ 30 * RAD };
				const double penumbra{ PI05 };
				const COLOUR4 diffuse{ 1,1,1,0 };
				const COLOUR4 specular{ 1,1,1,0 };
				const COLOUR4 ambient{ 0,0,0,0 };
			} spotInfo;

			void SetOxygenConsumption(double simdt);
			void SetLandedStatus();
			void SetSurfaceRef();
			void SetGroundMovement(double simdt);

			void SetDefaultValues();
			void SetValue(ValueInfo& valueInfo, bool setMax, bool setMin, bool setSlow);

			void SetHeadlight(bool active);
			void SetSuit(bool on, bool checkBreath);

			bool InBreathableArea(bool showMessage);
			void Kill();

			void DrawNearestHUD(int& x, int& y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp);
			void DrawVesselHUD(int& x, int& y, oapi::Sketchpad* skp);
			void DrawCargoHUD(int& x, int& y, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp);

			void DrawVesselInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos);
			void DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const API::CargoInfo& cargoInfo, bool drawBreathable);

			std::optional<size_t> GetFirstVesselIndex();
			std::optional<size_t> GetFirstCargoIndex();
		};
	}
}