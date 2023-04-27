#pragma once
#include "..\..\API\Astronaut.h"
#include "..\..\API\Vessel.h"
#include <forward_list>

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
			inline static bool configLoaded{};
			inline static bool enhancedMovements{ true };
			inline static double nearSearchRange { 60e3 };			
			static void LoadConfig();

			API::AstrInfo astrInfo;
			double suitMass;

			UINT suitMesh, bodyMesh;			
			PROPELLANT_HANDLE hFuel, hOxy;

			std::string buffer;
			bool keyDown{};
			bool suitOn{ true };
			double consumptionRate{ 1 };

			API::Vessel vslAPI;
			API::VslCargoInfo vslCargoInfo;
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

				struct VesselInfo
				{
					const API::VslAstrInfo* info;
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

			void SetDefaultValues();
			void SetValue(ValueInfo& valueInfo, bool setMax, bool setMin, bool setSlow);

			void SetHeadlight(bool active);
			void SetSuit(bool on, bool checkBreath);

			bool InBreathableArea(bool showMessage);
			void Kill();

			void DrawNearHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawVslHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawAstrHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawCargoHUD(int x, int y, oapi::Sketchpad* skp);
			void DrawShort1HUD(int x, int y, oapi::Sketchpad* skp);
			void DrawShort2HUD(int x, int y, oapi::Sketchpad* skp);

			void DrawVslInfo(int x, int& y, oapi::Sketchpad* skp, VECTOR3 relPos);
			void DrawAstrInfo(int x, int& y, oapi::Sketchpad* skp, const API::AstrInfo& astrInfo);
			void DrawCargoInfo(int x, int& y, oapi::Sketchpad* skp, const API::CargoInfo& cargoInfo, bool drawBreathable, bool selectedName = false);

			std::optional<size_t> GetFirstVslIdx();
			std::optional<size_t> GetFirstAstrIdx();
			std::optional<size_t> GetFirstCargoIdx();
			size_t GetFreeCargoCount();
		};
	}
}