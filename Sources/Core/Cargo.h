#pragma once
#include "..\API\Cargo.h"

namespace UACS
{
	namespace Core
	{
		class Cargo
		{
		public:
			Cargo(UACS::Cargo* pCargo);
			virtual void Destroy() noexcept;

			virtual std::string_view GetUACSVersion();

		private:
			UACS::Cargo* pCargo;
		};
	}
}