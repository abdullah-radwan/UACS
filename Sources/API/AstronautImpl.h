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

			std::string_view GetUACSVersion();

			size_t GetScnAstrCount();

			std::pair<OBJHANDLE, const AstrInfo*> GetAstrInfoByIndex(size_t);

			const AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			const VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			void SetScnAstrInfoByIndex(size_t, AstrInfo);

			bool SetScnAstrInfoByHandle(OBJHANDLE, AstrInfo);

			std::optional<NearestAirlock> GetNearestAirlock(double);

			IngressResult Ingress(OBJHANDLE, std::optional<size_t>, std::optional<size_t>);

		private:
			HINSTANCE coreDLL;
			Core::Astronaut* pCoreAstr{};
		};
	}
}