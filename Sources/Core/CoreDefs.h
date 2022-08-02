#pragma once
#include "Vessel.h"
#include "Astronaut.h"

namespace UACS
{
	namespace Core
	{
		using AstronautFunc = void (*)(API::Astronaut*);

		using CargoFunc = void (*)(API::Cargo*);

		using CreateAstronaut = Astronaut* (*)(API::Astronaut*);

		using CreateVessel = Vessel* (*)(VESSEL*, API::VslAstrInfo*, API::VslCargoInfo*);
	}
}