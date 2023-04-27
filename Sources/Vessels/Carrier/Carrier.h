#pragma once
#include "../../Common.h"
#include "../../API/Vessel.h"

namespace UACS
{
	namespace Vessel
	{
		class Carrier : public VESSEL4
		{
		public:
			Carrier(OBJHANDLE hVessel, int flightmodel);
			void clbkSetClassCaps(FILEHANDLE cfg) override;

			void clbkLoadStateEx(FILEHANDLE scn, void* status) override;
			void clbkSaveState(FILEHANDLE scn) override;

			void clbkPostCreation() override;
			int clbkGeneric(int msgid, int prm, void* context) override;
			int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate) override;			
	
			void clbkPreStep(double simt, double simdt, double mjd) override;
			bool clbkDrawHUD(int mode, const HUDPAINTSPEC* hps, oapi::Sketchpad* skp) override;

		private:
			API::Vessel vslAPI;
			API::VslAstrInfo vslAstrInfo;
			API::VslCargoInfo vslCargoInfo;

			enum { HUD_OP = 1, HUD_SRT };
			size_t hudMode { HUD_NONE };

			struct HudInfo
			{
				size_t idx{};
				std::string msg;
				double timer{ 5 };
			} astrHUD, cargoHUD;
			std::string buffer;

			static void vlift(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd);
			static void hlift(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd);

			// animation transformation definitions
			inline static UINT GRP_LWING = 2;
			inline static UINT GRP_RWING = 3;
			inline static VECTOR3 LWING_REF = { -1.3,-0.725,-1.5 };
			inline static VECTOR3 LWING_AXIS = { -0.9619,-0.2735,0 };
			inline static VECTOR3 RWING_REF = { 1.3,-0.725,-1.5 };
			inline static VECTOR3 RWING_AXIS = { 0.9619,-0.2735,0 };
			inline static float AILERON_RANGE = (float)(20.0 * RAD);
			inline static float ELEVATOR_RANGE = (float)(30.0 * RAD);
			inline static MGROUP_ROTATE trans_Laileron = { 0, &GRP_LWING, 1, LWING_REF, LWING_AXIS, AILERON_RANGE };
			inline static MGROUP_ROTATE trans_Raileron = { 0, &GRP_RWING, 1, RWING_REF, RWING_AXIS, AILERON_RANGE };
			inline static MGROUP_ROTATE trans_Lelevator = { 0, &GRP_LWING, 1, LWING_REF, LWING_AXIS, -ELEVATOR_RANGE };
			inline static MGROUP_ROTATE trans_Relevator = { 0, &GRP_RWING, 1, RWING_REF, RWING_AXIS, ELEVATOR_RANGE };
		};
	}
}
