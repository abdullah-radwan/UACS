#include "Cargo.h"
#include "..\Core\CoreDefs.h"

namespace UACS
{
	namespace API
	{
		Cargo::Cargo(OBJHANDLE hVessel, int fModel) : VESSEL4(hVessel, fModel), coreDLL(LoadLibraryA("Modules/UACS/Core.dll")) 
		{
			if (coreDLL)
			{
				auto CreateCargo = reinterpret_cast<Core::CreateCargo>(GetProcAddress(coreDLL, "CreateCargo"));
				if (CreateCargo) pCoreCargo = CreateCargo(this);
			}

			if (!pCoreCargo)
			{
				if (coreDLL) FreeLibrary(coreDLL);

				oapiWriteLog("UACS cargo fatal error: Couldn't load the core DLL");
				std::terminate();
			}
		}

		Cargo::~Cargo() { pCoreCargo->Destroy(); FreeLibrary(coreDLL); }

		std::string_view Cargo::GetUACSVersion() { return pCoreCargo->GetUACSVersion(); }

		void Cargo::clbkCargoGrappled() { }

		void Cargo::clbkCargoReleased() { }

		bool Cargo::clbkPackCargo() { return false; }

		bool Cargo::clbkUnpackCargo() { return false; }

		double Cargo::clbkDrainResource(double mass) { return 0; }
	}
}