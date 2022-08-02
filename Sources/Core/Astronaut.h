#pragma once
#include "..\API\Astronaut.h"

namespace UACS
{
	namespace Core
	{
		class Astronaut
		{
		public:
			Astronaut(API::Astronaut* pAstr);
			virtual void Destroy() noexcept;

			virtual std::string_view GetUACSVersion();

			virtual size_t GetScnAstrCount();

			virtual std::pair<OBJHANDLE, const API::AstrInfo*> GetAstrInfoByIndex(size_t);

			virtual const API::AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			virtual const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			virtual void SetScnAstrInfoByIndex(size_t, API::AstrInfo);

			virtual bool SetScnAstrInfoByHandle(OBJHANDLE, API::AstrInfo);

			virtual std::optional<API::NearestAirlock> GetNearestAirlock(double);

			virtual std::pair<OBJHANDLE, VECTOR3> GetNearestBreathable(double);

			virtual bool InBreathableArea();

			virtual API::IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>);

		private:
			API::Astronaut* pAstr;
			bool passCheck{};
		};
	}
}
