#include "Astronaut.h"
#include "..\Core\Defs.h"

namespace UACS
{
	Astronaut::Astronaut(OBJHANDLE hVessel, int fModel) : VESSEL4(hVessel, fModel), coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
	{
		if (coreDLL)
		{
			auto CreateAstronaut = reinterpret_cast<Core::CreateAstronaut>(GetProcAddress(coreDLL, "CreateAstronaut"));
			if (CreateAstronaut) pCoreAstr = CreateAstronaut(this);
		}

		if (!pCoreAstr)
		{
			if (coreDLL) FreeLibrary(coreDLL);

			oapiWriteLog("UACS astronaut fatal error: Couldn't load the core DLL");
			std::terminate();
		}
	}

	Astronaut::~Astronaut() { pCoreAstr->Destroy(); FreeLibrary(coreDLL); }

	std::string_view Astronaut::GetUACSVersion() { return pCoreAstr->GetUACSVersion(); }

	size_t Astronaut::GetScnAstrCount() { return pCoreAstr->GetScnAstrCount(); }

	std::pair<OBJHANDLE, const AstrInfo*> Astronaut::GetAstrInfoByIndex(size_t astrIdx) { return pCoreAstr->GetAstrInfoByIndex(astrIdx); }

	const AstrInfo* Astronaut::GetAstrInfoByHandle(OBJHANDLE hAstr) { return pCoreAstr->GetAstrInfoByHandle(hAstr); }

	const VslAstrInfo* Astronaut::GetVslAstrInfo(OBJHANDLE hVessel) { return pCoreAstr->GetVslAstrInfo(hVessel); }

	std::optional<NearestAirlock> Astronaut::GetNearestAirlock(double range) { return pCoreAstr->GetNearestAirlock(range); }

	std::pair<OBJHANDLE, VECTOR3> Astronaut::GetNearestBreathable(double range) { return pCoreAstr->GetNearestBreathable(range); }

	bool Astronaut::InBreathableArea() { return pCoreAstr->InBreathableArea(); }

	IngressResult Astronaut::Ingress(OBJHANDLE hVessel, std::optional<size_t> airlockIdx, std::optional<size_t> stationIdx)
	{ return pCoreAstr->Ingress(hVessel, airlockIdx, stationIdx); }
}