#include "CargoImpl.h"

namespace UACS
{
	namespace API
	{
		Cargo::Cargo(OBJHANDLE hVessel, int fModel) : VESSEL4(hVessel, fModel), cargoImpl(new CargoImpl(this)) {}

		Cargo::~Cargo() = default;

		void Cargo::clbkCargoGrappled() { }

		void Cargo::clbkCargoReleased() { }

		bool Cargo::clbkPackCargo() { return false; }

		bool Cargo::clbkUnpackCargo() { return false; }

		double Cargo::clbkDrainResource(double mass) { return 0; }

		CargoImpl::CargoImpl(Cargo* cargo) : cargo(cargo), coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
		{
			if (coreDLL)
			{
				AddCargo = reinterpret_cast<Core::CargoFunc>(GetProcAddress(coreDLL, "AddCargo"));

				DeleteCargo = reinterpret_cast<Core::CargoFunc>(GetProcAddress(coreDLL, "DeleteCargo"));
			}

			if (AddCargo && DeleteCargo) AddCargo(cargo);

			else
			{
				if (coreDLL) FreeLibrary(coreDLL);

				oapiWriteLog("UACS cargo fatal error: Couldn't load the core DLL");

				std::terminate();
			}
		}

		CargoImpl::~CargoImpl() { DeleteCargo(cargo); FreeLibrary(coreDLL); }
	}
}