#pragma once
#include "..\..\API\Astronaut.h"
#include "..\..\API\Module.h"
#include <forward_list>
#include <map>

namespace UACS
{
	namespace Vessel
	{
		class Astronaut : public UACS::Astronaut
		{
		public:
			Astronaut(OBJHANDLE hVessel, int fModel);
			~Astronaut();

			void clbkSetClassCaps(FILEHANDLE cfg) override;

			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkSaveState(FILEHANDLE scn) override;

			void clbkPostCreation() override;

			bool clbkSetAstrInfo(const UACS::AstrInfo& astrInfo) override;
			const UACS::AstrInfo* clbkGetAstrInfo() override;

			int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate) override;
			int clbkConsumeDirectKey(char* kstate) override;

			void clbkPreStep(double simt, double simdt, double mjd) override;
			bool clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp) override;

		private:
			inline static bool configLoaded{};
			inline static bool enhancedMovements{ true };
			inline static double searchRange{ 60e3 };
			static void LoadConfig();

			UACS::AstrInfo astrInfo;
			VECTOR3 suitHoldDir, bodyHoldDir;
			double suitMass;

			UINT suitMesh, bodyMesh;
			double bodyHeight;
			PROPELLANT_HANDLE hFuel, hOxy;

			std::string buffer;
			bool keyDown{};
			bool suitOn{ true };
			double consumptionRate{ 1 };

			UACS::Module mdlAPI;
			UACS::VslCargoInfo vslCargoInfo;
			SpotLight* spotLight1{};
			SpotLight* spotLight2{};

			enum { HUD_NST = 1, HUD_VSL, HUD_AST, HUD_CRG, HUD_SRT1, HUD_SRT2 };

			struct HudInfo
			{
				size_t mode{ HUD_NONE };
				oapi::Font* deadFont{};

				std::string message;
				double timer{};

				std::string modeMsg;
				double modeTimer{ 5 };

				size_t availIdx{};
				bool drainFuel{ true };

				size_t vslIdx{};
				OBJHANDLE hVessel{};

				std::map<size_t, OBJHANDLE> vslMap, astrMap, cargoMap;

				struct VesselInfo
				{
					const UACS::VslAstrInfo* info;
					std::optional<std::string> resources{};
					size_t arlckIdx{};
					size_t statIdx{};
				} vslInfo;

				int rightX, startY;
				int smallSpace, space, largeSpace;
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

			struct HeadlightInfo
			{
				struct
				{
					VECTOR3 pos;
					VECTOR3 color{ 1,1,1 };
					BEACONLIGHTSPEC spec{ BEACONSHAPE_DIFFUSE, &pos, &color, 0.02, 0.2, 0, 0, 0, false };
				} beaconInfo;

				struct
				{
					VECTOR3 pos;
					VECTOR3 dir;
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

				SpotLight* spotLight;
			}; std::forward_list<HeadlightInfo> headlights;

			void InitPropellant();
			void SetOxygenConsumption(double simdt);
			void SetLandedStatus();

			void SetSurfaceRef();
			void SetGroundMovement(double simdt);
			void SetLngLatHdg(double distance, double hdgOffset, VESSELSTATUS2& status);

			void SetDefaultValues();
			void SetValue(ValueInfo& valueInfo, bool setMax, bool setMin, bool setSlow);

			void SetHeadlight(bool active);
			void SetSuit(bool on, bool checkBreath);
			void Kill();

			void DrawNearHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawVslHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawAstrHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawCargoHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawShort1HUD(int x, int y, oapi::Sketchpad* skp);
			void DrawShort2HUD(int x, int y, oapi::Sketchpad* skp);

			void DrawVslInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos);
			void DrawAstrInfo(int x, int& y, oapi::Sketchpad* skp, const UACS::AstrInfo& astrInfo);
			void DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const UACS::CargoInfo& cargoInfo, bool drawBreathable, bool selectedName = false);

			void SetVslMap();
			void SetAstrMap();
			void SetCargoMap();
			void SetMapIdx(const std::map<size_t, OBJHANDLE>& map, bool increase);
		};
	}
}