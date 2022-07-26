#include "AstronautImpl.h"

namespace UACS
{
	namespace API
	{
		Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : VESSEL4(hVessel, fModel), pAstrImpl(new AstronautImpl(this)) {}

		Astronaut::~Astronaut() = default;

		std::string_view Astronaut::GetUACSVersion() { return pAstrImpl->GetUACSVersion(); }

		size_t Astronaut::GetScnAstrCount() { return pAstrImpl->GetScnAstrCount(); }

		std::pair<OBJHANDLE, const AstrInfo*> Astronaut::GetAstrInfoByIndex(size_t astrIdx) { return pAstrImpl->GetAstrInfoByIndex(astrIdx); }

		const AstrInfo* Astronaut::GetAstrInfoByHandle(OBJHANDLE hAstr) { return pAstrImpl->GetAstrInfoByHandle(hAstr); }

		const VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return pAstrImpl->GetVslAstrInfo(hVessel); }

		void Astronaut::SetScnAstrInfoByIndex(size_t astrIdx, AstrInfo astrInfo) { pAstrImpl->SetScnAstrInfoByIndex(astrIdx, astrInfo); }

		bool Astronaut::SetScnAstrInfoByHandle(OBJHANDLE hAstr, AstrInfo astrInfo) { return pAstrImpl->SetScnAstrInfoByHandle(hAstr, astrInfo);}

		std::optional<NearestAirlock> Astronaut::GetNearestAirlock(double range) { return pAstrImpl->GetNearestAirlock(range); }

		IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx) 
		{ return pAstrImpl->Ingress(hVessel, airlockIdx, stationIdx); }

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

		std::string_view AstronautImpl::GetUACSVersion() { return pCoreAstr->GetUACSVersion(); }

		size_t AstronautImpl::GetScnAstrCount() { return pCoreAstr->GetScnAstrCount(); }

		std::pair<OBJHANDLE, const AstrInfo*> AstronautImpl::GetAstrInfoByIndex(size_t astrIdx) { return pCoreAstr->GetAstrInfoByIndex(astrIdx); }

		const AstrInfo* AstronautImpl::GetAstrInfoByHandle(OBJHANDLE hAstr) { return pCoreAstr->GetAstrInfoByHandle(hAstr); }

		const VslAstrInfo* AstronautImpl::GetVslAstrInfo(OBJHANDLE hVessel) { return pCoreAstr->GetVslAstrInfo(hVessel); }

		void AstronautImpl::SetScnAstrInfoByIndex(size_t astrIdx, AstrInfo astrInfo) { pCoreAstr->SetScnAstrInfoByIndex(astrIdx, astrInfo); }

		bool AstronautImpl::SetScnAstrInfoByHandle(OBJHANDLE hAstr, AstrInfo astrInfo) { return pCoreAstr->SetScnAstrInfoByHandle(hAstr, astrInfo); }

		std::optional<NearestAirlock> AstronautImpl::GetNearestAirlock(double range) { return pCoreAstr->GetNearestAirlock(range); }

		IngressResult AstronautImpl::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
		{ return pCoreAstr->Ingress(hVessel, airlockIdx, stationIdx); }
	}
}