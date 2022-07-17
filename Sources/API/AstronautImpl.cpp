#include "AstronautImpl.h"

namespace UACS
{
	namespace API
	{
		Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : VESSEL4(hVessel, fModel), astrImpl(new AstronautImpl(this)) {}

		Astronaut::~Astronaut() = default;

		std::optional<NearestAirlock> Astronaut::GetNearestAirlock(double range) { return astrImpl->GetNearestAirlock(range); }

		const VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return astrImpl->GetVslAstrInfo(hVessel); }

		const AstrInfo* Astronaut::GetAstrInfo(OBJHANDLE hVessel) { return astrImpl->GetAstrInfo(hVessel); }

		IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx) 
		{ return astrImpl->Ingress(hVessel, airlockIdx, stationIdx); }

		AstronautImpl::AstronautImpl(Astronaut* pAstr) : coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
		{
			if (coreDLL)
			{
				auto CreateAstronaut = reinterpret_cast<Core::CreateAstronaut>(GetProcAddress(coreDLL, "CreateAstronaut"));

				if (CreateAstronaut) pCoreAstr = CreateAstronaut(pAstr);
			}

			if (!pCoreAstr)
			{
				if (coreDLL) FreeLibrary(coreDLL);

				oapiWriteLog("UACS astronaut fatal error: Couldn't load the core DLL");

				std::terminate();
			}
		}

		AstronautImpl::~AstronautImpl() { pCoreAstr->Destroy(); FreeLibrary(coreDLL); }

		std::optional<NearestAirlock> AstronautImpl::GetNearestAirlock(double range) { return pCoreAstr->GetNearestAirlock(range); }

		const VslAstrInfo* AstronautImpl::GetVslAstrInfo(OBJHANDLE hVessel) { return pCoreAstr->GetVslAstrInfo(hVessel); }

		const AstrInfo* AstronautImpl::GetAstrInfo(OBJHANDLE hVessel) { return pCoreAstr->GetAstrInfo(hVessel); }

		IngressResult AstronautImpl::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
		{
			return pCoreAstr->Ingress(hVessel, airlockIdx, stationIdx);
		}
	}
}