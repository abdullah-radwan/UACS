#include "Vessel.h"
#include "..\Core\CoreDefs.h"

namespace UACS
{
	namespace API
	{
		Vessel::Vessel(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo) : coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
		{
			if (coreDLL)
			{
				auto CreateVessel = reinterpret_cast<Core::CreateVessel>(GetProcAddress(coreDLL, "CreateVessel"));

				if (CreateVessel) pCoreVessel = CreateVessel(pVessel, pVslAstrInfo, pVslCargoInfo);
			}

			if (!pCoreVessel) oapiWriteLog("UACS vessel warning: Couldn't load the core DLL");
		}

		Vessel::~Vessel()
		{
			if (pCoreVessel) pCoreVessel->Destroy();

			if (coreDLL) FreeLibrary(coreDLL);
		}

		std::string_view Vessel::GetUACSVersion()
		{ return pCoreVessel ? pCoreVessel->GetUACSVersion() : std::string_view(); }

		bool Vessel::ParseScenarioLine(char* line) { return pCoreVessel ? pCoreVessel->ParseScenarioLine(line) : false; }
		
		void Vessel::clbkPostCreation() { if (pCoreVessel) pCoreVessel->clbkPostCreation(); }

		void Vessel::clbkSaveState(FILEHANDLE scn) { if (pCoreVessel) pCoreVessel->clbkSaveState(scn); }

		size_t Vessel::GetScnAstrCount() { return pCoreVessel ? pCoreVessel->GetScnAstrCount() : 0; }

		std::pair<OBJHANDLE, const AstrInfo*> Vessel::GetAstrInfoByIndex(size_t astrIdx)
		{ return pCoreVessel ? pCoreVessel->GetAstrInfoByIndex(astrIdx) : std::pair<OBJHANDLE, const AstrInfo*>{}; }

		const AstrInfo* Vessel::GetAstrInfoByHandle(OBJHANDLE hAstr)
		{ return pCoreVessel ? pCoreVessel->GetAstrInfoByHandle(hAstr) : nullptr; }

		const VslAstrInfo* Vessel::GetVslAstrInfo(OBJHANDLE hVessel)
		{ return pCoreVessel ? pCoreVessel->GetVslAstrInfo(hVessel) : nullptr; }

		size_t Vessel::GetAvailAstrCount() { return pCoreVessel ? pCoreVessel->GetAvailAstrCount() : 0; }

		std::string_view Vessel::GetAvailAstrName(size_t availIdx)
		{ return pCoreVessel ? pCoreVessel->GetAvailAstrName(availIdx) : std::string_view(); }

		IngressResult Vessel::AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx, std::optional<AstrInfo> astrInfo)
		{
			return pCoreVessel ? pCoreVessel->AddAstronaut(availIdx, stationIdx, astrInfo) : INGRS_FAIL;
		}

		TransferResult Vessel::TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx)
		{
			return pCoreVessel ? pCoreVessel->TransferAstronaut(stationIdx, airlockIdx, tgtStationIdx) : TRNS_FAIL;
		}

		EgressResult Vessel::EgressAstronaut(size_t stationIdx, size_t airlockIdx)
		{
			return pCoreVessel ? pCoreVessel->EgressAstronaut(stationIdx, airlockIdx) : EGRS_FAIL;
		}

		size_t Vessel::GetScnCargoCount() { return pCoreVessel ? pCoreVessel->GetScnCargoCount() : 0; }

		CargoInfo Vessel::GetCargoInfoByIndex(size_t cargoIdx) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByIndex(cargoIdx) : CargoInfo(); }

		std::optional<CargoInfo> Vessel::GetCargoInfoByHandle(OBJHANDLE hCargo) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByHandle(hCargo) : std::nullopt; }

		double Vessel::GetTotalCargoMass() { return pCoreVessel ? pCoreVessel->GetTotalCargoMass() : 0; }

		size_t Vessel::GetAvailCargoCount() { return pCoreVessel ? pCoreVessel->GetAvailCargoCount() : 0; }

		std::string_view Vessel::GetAvailCargoName(size_t availIdx)
		{ return pCoreVessel ? pCoreVessel->GetAvailCargoName(availIdx) : std::string_view(); }

		GrappleResult Vessel::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->AddCargo(availIdx, slotIdx) : GRPL_FAIL; }

		ReleaseResult Vessel::DeleteCargo(std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->DeleteCargo(slotIdx) : RLES_FAIL; }

		GrappleResult Vessel::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->GrappleCargo(hCargo, slotIdx) : GRPL_FAIL; }

		ReleaseResult Vessel::ReleaseCargo(std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->ReleaseCargo(slotIdx) : RLES_FAIL; }

		PackResult Vessel::PackCargo(OBJHANDLE hCargo)
		{ return pCoreVessel ? pCoreVessel->PackCargo(hCargo) : PACK_FAIL; }

		PackResult Vessel::UnpackCargo(OBJHANDLE hCargo)
		{ return pCoreVessel ? pCoreVessel->UnpackCargo(hCargo) : PACK_FAIL; }

		std::pair<DrainResult, double> Vessel::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->DrainGrappledResource(resource, mass, slotIdx) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 }; }

		std::pair<DrainResult, double> Vessel::DrainScenarioResource(std::string_view resource, double mass, OBJHANDLE hCargo)
		{
			return pCoreVessel ? pCoreVessel->DrainScenarioResource(resource, mass, hCargo) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 };
		}

		std::pair<DrainResult, double> Vessel::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
		{
			return pCoreVessel ? pCoreVessel->DrainStationResource(resource, mass, hStation) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 };
		}
	}
}


