#pragma once
#include "Cargo.h"
#include "..\Core\CoreDefs.h"

namespace UACS
{
	namespace API
	{
		class CargoImpl
		{
		public:
			CargoImpl(Cargo*);
			~CargoImpl();

		private:
			Cargo* cargo;

			HINSTANCE coreDLL;
			Core::CargoFunc AddCargo{};
			Core::CargoFunc DeleteCargo{};
		};
	}
}