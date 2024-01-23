#pragma once
#include "..\API\Astronaut.h"
#include <span>

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

			virtual std::pair<OBJHANDLE, const AstrInfo*> GetAstrInfoByIndex(size_t);

			virtual const AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			virtual const VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			virtual std::optional<NearestAirlock> GetNearestAirlock(double, bool, bool);

			virtual std::pair<OBJHANDLE, VECTOR3> GetNearestBreathable(double);

			virtual std::optional<NearestAction> GetNearestAction(double, bool);

			virtual bool InBreathable(bool);

			virtual IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>);

			virtual IngressResult TriggerAction(OBJHANDLE, std::optional<size_t>);

		private:
			UACS::Astronaut* pAstr;
			bool passCheck{};
		};
	}
}