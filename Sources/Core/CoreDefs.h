#pragma once
#include "Astronaut.h"
#include "Cargo.h"
#include "Vessel.h"

namespace UACS
{
	namespace Core
	{
		using AstronautFunc = void (*)(API::Astronaut*);

		using CargoFunc = void (*)(API::Cargo*);

		using CreateAstronaut = Astronaut* (*)(API::Astronaut*);

		using CreateCargo = Cargo* (*)(API::Cargo*);

		using CreateVessel = Vessel* (*)(VESSEL*, API::VslAstrInfo*, API::VslCargoInfo*);
	}
}