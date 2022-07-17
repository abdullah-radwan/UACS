#pragma once
#include "Astronaut.h"
#include "..\Core\CoreDefs.h"

namespace UACS
{
	namespace API
	{
		class AstronautImpl
		{
		public:
			AstronautImpl(Astronaut*);
			~AstronautImpl();

			std::optional<NearestAirlock> GetNearestAirlock(double);

			const VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			const AstrInfo* GetAstrInfo(OBJHANDLE);

			IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>);

		private:
			HINSTANCE coreDLL;
			Core::Astronaut* pCoreAstr{};
		};
	}
}