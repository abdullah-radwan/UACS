#pragma once
#include "..\API\Astronaut.h"
#include "..\API\Cargo.h"
#include "..\API\Vessel.h"

namespace UACS
{
	namespace Core
	{
		using AstronautFunc = void (*)(API::Astronaut*);

		using CargoFunc = void (*)(API::Cargo*);

		class Astronaut
		{
		public:
			virtual void Destroy() noexcept = 0;

			virtual std::optional<API::NearestAirlock> GetNearestAirlock(double) = 0;

			virtual const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE) = 0;

			virtual const API::AstrInfo* GetAstrInfo(OBJHANDLE) = 0;

			virtual API::IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>) = 0;
		};

		using CreateAstronaut = Astronaut* (*)(API::Astronaut*);

		class Vessel : public API::Vessel { public: virtual void Destroy() noexcept = 0; };
		using CreateVessel = Vessel* (*)(VESSEL*, API::VslAstrInfo*, API::VslCargoInfo*);
	}
}