#pragma once
#include "..\API\Astronaut.h"

namespace UACS
{
	namespace Core
	{
		class Astronaut
		{
		public:
			Astronaut(UACS::Astronaut* pAstr);
			virtual void Destroy() noexcept;

			virtual std::string_view GetUACSVersion();

			virtual size_t GetScnAstrCount();

			virtual std::pair<OBJHANDLE, const UACS::AstrInfo*> GetAstrInfoByIndex(size_t);

			virtual const UACS::AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			virtual const UACS::VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			virtual std::optional<UACS::NearestAirlock> GetNearestAirlock(double);

			virtual std::pair<OBJHANDLE, VECTOR3> GetNearestBreathable(double);

			virtual bool InBreathableArea();

			virtual UACS::IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>);

		private:
			UACS::Astronaut* pAstr;
			bool passCheck{};
		};
	}
}