#include "Common.h"

DLLCLBK UACS::Core::Cargo* CreateCargo(UACS::Cargo* pCargo) { return new UACS::Core::Cargo(pCargo); }

namespace UACS
{
	namespace Core
	{
		Cargo::Cargo(UACS::Cargo* pCargo) : pCargo(pCargo) { cargoVector.push_back(pCargo); }

		void Cargo::Destroy() noexcept
		{
			std::erase(cargoVector, pCargo);
			delete this;
		}

		std::string_view Cargo::GetUACSVersion() { return Core::GetUACSVersion(); }
	}
}