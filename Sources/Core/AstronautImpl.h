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

			std::string_view GetUACSVersion() override;

			size_t GetScnAstrCount() override;

			std::pair<OBJHANDLE, const API::AstrInfo*> GetAstrInfoByIndex(size_t) override;

			const API::AstrInfo* GetAstrInfoByHandle(OBJHANDLE) override;

			const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE) override;

			void SetScnAstrInfoByIndex(size_t, API::AstrInfo) override;

			bool SetScnAstrInfoByHandle(OBJHANDLE, API::AstrInfo) override;

			std::optional<API::NearestAirlock> GetNearestAirlock(double) override;

			API::IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>) override;

		private:
			API::Astronaut* pAstr;	
		};
	}
}
