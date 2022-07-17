#pragma once
#include "CoreCommon.h"

namespace UACS
{
	namespace Core
	{
		class AstronautImpl : public Astronaut
		{
		public:
			AstronautImpl(API::Astronaut*);
			void Destroy() noexcept override;

			std::optional<API::NearestAirlock> GetNearestAirlock(double) override;

			const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE) override;

			const API::AstrInfo* GetAstrInfo(OBJHANDLE) override;

			API::IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>) override;

		private:
			API::Astronaut* pAstr;	
		};
	}
}
