#include "VesselImpl.h"

namespace UACS
{
	namespace API
	{
		std::unique_ptr<Vessel> Vessel::CreateInstance(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo) 
		{ return std::make_unique<VesselImpl>(pVessel, pVslAstrInfo, pVslCargoInfo); }

		VesselImpl::VesselImpl(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo) : coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
		{
			if (coreDLL)
			{
				auto CreateVessel = reinterpret_cast<Core::CreateVessel>(GetProcAddress(coreDLL, "CreateVessel"));

				if (CreateVessel) pCoreVessel = CreateVessel(pVessel, pVslAstrInfo, pVslCargoInfo);
			}

			if (!pCoreVessel) oapiWriteLog("UACS vessel warning: Couldn't load the core DLL");
		}

		VesselImpl::~VesselImpl()
		{
			if (pCoreVessel) pCoreVessel->Destroy();

			if (coreDLL) FreeLibrary(coreDLL);
		}

		std::string_view VesselImpl::GetUACSVersion()
		{ return pCoreVessel ? pCoreVessel->GetUACSVersion() : std::string_view(); }

		bool VesselImpl::ParseScenarioLine(char* line) { return pCoreVessel ? pCoreVessel->ParseScenarioLine(line) : false; }
		
		void VesselImpl::clbkPostCreation() { if (pCoreVessel) pCoreVessel->clbkPostCreation(); }

		void VesselImpl::SaveState(FILEHANDLE scn) { if (pCoreVessel) pCoreVessel->SaveState(scn); }

		size_t VesselImpl::GetScnAstrCount() { return pCoreVessel ? pCoreVessel->GetScnAstrCount() : 0; }

		std::pair<OBJHANDLE, const AstrInfo*> VesselImpl::GetAstrInfoByIndex(size_t astrIdx)
		{ return pCoreVessel ? pCoreVessel->GetAstrInfoByIndex(astrIdx) : std::pair<OBJHANDLE, const AstrInfo*>{}; }

		const AstrInfo* VesselImpl::GetAstrInfoByHandle(OBJHANDLE hAstr)
		{ return pCoreVessel ? pCoreVessel->GetAstrInfoByHandle(hAstr) : nullptr; }

		const VslAstrInfo* VesselImpl::GetVslAstrInfo(OBJHANDLE hVessel)
		{ return pCoreVessel ? pCoreVessel->GetVslAstrInfo(hVessel) : nullptr; }

		void VesselImpl::SetScnAstrInfoByIndex(size_t astrIdx, AstrInfo astrInfo)
		{ if (pCoreVessel) pCoreVessel->SetScnAstrInfoByIndex(astrIdx, astrInfo); }

		bool VesselImpl::SetScnAstrInfoByHandle(OBJHANDLE hAstr, AstrInfo astrInfo)
		{ return pCoreVessel ? pCoreVessel->SetScnAstrInfoByHandle(hAstr, astrInfo) : false; }

		size_t VesselImpl::GetAvailAstrCount() { return pCoreVessel ? pCoreVessel->GetAvailAstrCount() : 0; }

		std::string_view VesselImpl::GetAvailAstrName(size_t availIdx)
		{ return pCoreVessel ? pCoreVessel->GetAvailAstrName(availIdx) : std::string_view(); }

		IngressResult VesselImpl::AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx, std::optional<AstrInfo> astrInfo)
		{
			return pCoreVessel ? pCoreVessel->AddAstronaut(availIdx, stationIdx, astrInfo) : INGRS_FAIL;
		}

		TransferResult VesselImpl::TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx)
		{
			return pCoreVessel ? pCoreVessel->TransferAstronaut(stationIdx, airlockIdx, tgtStationIdx) : TRNS_FAIL;
		}

		EgressResult VesselImpl::EgressAstronaut(size_t stationIdx, size_t airlockIdx)
		{
			return pCoreVessel ? pCoreVessel->EgressAstronaut(stationIdx, airlockIdx) : EGRS_FAIL;
		}

		size_t VesselImpl::GetScnCargoCount() { return pCoreVessel ? pCoreVessel->GetScnCargoCount() : 0; }

		CargoInfo VesselImpl::GetCargoInfoByIndex(size_t cargoIdx) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByIndex(cargoIdx) : CargoInfo(); }

		std::optional<CargoInfo> VesselImpl::GetCargoInfoByHandle(OBJHANDLE hCargo) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByHandle(hCargo) : std::nullopt; }

		double VesselImpl::GetTotalCargoMass() { return pCoreVessel ? pCoreVessel->GetTotalCargoMass() : 0; }

		size_t VesselImpl::GetAvailCargoCount() { return pCoreVessel ? pCoreVessel->GetAvailCargoCount() : 0; }

		std::string_view VesselImpl::GetAvailCargoName(size_t availIdx)
		{ return pCoreVessel ? pCoreVessel->GetAvailCargoName(availIdx) : std::string_view(); }

		GrappleResult VesselImpl::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->AddCargo(availIdx, slotIdx) : GRPL_FAIL; }

		ReleaseResult VesselImpl::DeleteCargo(std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->DeleteCargo(slotIdx) : RLES_FAIL; }

		GrappleResult VesselImpl::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->GrappleCargo(hCargo, slotIdx) : GRPL_FAIL; }

		ReleaseResult VesselImpl::ReleaseCargo(std::optional<size_t> slotIdx)
		{ return pCoreVessel ? pCoreVessel->ReleaseCargo(slotIdx) : RLES_FAIL; }

		PackResult VesselImpl::PackCargo(OBJHANDLE hCargo)
		{ return pCoreVessel ? pCoreVessel->PackCargo(hCargo) : PACK_FAIL; }

		PackResult VesselImpl::UnpackCargo(OBJHANDLE hCargo)
		{ return pCoreVessel ? pCoreVessel->UnpackCargo(hCargo) : PACK_FAIL; }

		std::pair<DrainResult, double> VesselImpl::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{
			return pCoreVessel ? pCoreVessel->DrainGrappledResource(resource, mass, slotIdx) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 };
		}

		std::pair<DrainResult, double> VesselImpl::DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE hCargo)
		{
			return pCoreVessel ? pCoreVessel->DrainUngrappledResource(resource, mass, hCargo) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 };
		}

		std::pair<DrainResult, double> VesselImpl::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
		{
			return pCoreVessel ? pCoreVessel->DrainStationResource(resource, mass, hStation) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 };
		}

		OBJHANDLE VesselImpl::GetNearestBreathable() { return pCoreVessel ? pCoreVessel->GetNearestBreathable() : nullptr; }

		bool VesselImpl::InBreathableArea() { return pCoreVessel ? pCoreVessel->InBreathableArea() : false; }
	}
}


