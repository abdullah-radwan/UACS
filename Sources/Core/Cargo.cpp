#include "CoreCommon.h"

DLLCLBK UACS::Core::Cargo* CreateCargo(UACS::API::Cargo* pCargo) { return new UACS::Core::Cargo(pCargo); }

namespace UACS
{
	namespace Core
	{
		Cargo::Cargo(API::Cargo* pCargo) : pCargo(pCargo) { cargoVector.push_back(pCargo); }

		void Cargo::Destroy() noexcept
		{
			std::erase(cargoVector, pCargo);
			delete this;
		}

		std::string_view Cargo::GetUACSVersion() { return Core::GetUACSVersion(); }
	}
}
