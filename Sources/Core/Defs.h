#pragma once
#include "Astronaut.h"
#include "Cargo.h"
#include "Module.h"

namespace UACS
{
	namespace Core
	{
		using AstronautFunc = void (*)(UACS::Astronaut*);

		using CargoFunc = void (*)(UACS::Cargo*);

		using CreateAstronaut = Astronaut* (*)(UACS::Astronaut*);

		using CreateCargo = Cargo* (*)(UACS::Cargo*);

		using CreateModule = Module* (*)(VESSEL*, UACS::VslAstrInfo*, UACS::VslCargoInfo*);
	}
}