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

			if (!pCoreVessel) oapiWriteLog("UACS vessel cargo warning: Couldn't load the core DLL");
		}

		VesselImpl::~VesselImpl()
		{
			if (pCoreVessel) pCoreVessel->Destroy();

			if (coreDLL) FreeLibrary(coreDLL);
		}

		bool VesselImpl::ParseScenarioLine(char* line) { return pCoreVessel ? pCoreVessel->ParseScenarioLine(line) : false; }

		void VesselImpl::SaveState(FILEHANDLE scn) { if (pCoreVessel) pCoreVessel->SaveState(scn); }

		const AstrInfo* VesselImpl::GetAstrInfo(OBJHANDLE hVessel) { return pCoreVessel ? pCoreVessel->GetAstrInfo(hVessel) : nullptr; }

		const VslAstrInfo* VesselImpl::GetVslAstrInfo(OBJHANDLE hVessel) { return pCoreVessel ? pCoreVessel->GetVslAstrInfo(hVessel) : nullptr; }

		TransferResult VesselImpl::TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx)
		{
			return pCoreVessel ? pCoreVessel->TransferAstronaut(stationIdx, airlockIdx, tgtStationIdx) : TRNS_FAIL;
		}

		EgressResult VesselImpl::EgressAstronaut(size_t stationIdx, size_t airlockIdx)
		{
			return pCoreVessel ? pCoreVessel->EgressAstronaut(stationIdx, airlockIdx) : EGRS_FAIL;
		}

		size_t VesselImpl::GetScenarioCargoCount() { return pCoreVessel ? pCoreVessel->GetScenarioCargoCount() : 0; }

		CargoInfo VesselImpl::GetCargoInfoByIndex(size_t cargoIdx) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByIndex(cargoIdx) : CargoInfo(); }

		std::optional<CargoInfo> VesselImpl::GetCargoInfoByHandle(OBJHANDLE hVessel) 
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoByHandle(hVessel) : std::optional<CargoInfo>{}; }

		std::optional<CargoInfo> VesselImpl::GetCargoInfoBySlot(size_t slotIdx)
		{ return pCoreVessel ? pCoreVessel->GetCargoInfoBySlot(slotIdx) : std::optional<CargoInfo>{}; }

		double VesselImpl::GetTotalCargoMass() { return pCoreVessel ? pCoreVessel->GetTotalCargoMass() : 0; }

		size_t VesselImpl::GetAvailableCargoCount() { return pCoreVessel ? pCoreVessel->GetAvailableCargoCount() : 0; }

		std::string_view VesselImpl::GetAvailableCargoName(size_t availIdx) { return pCoreVessel ? pCoreVessel->GetAvailableCargoName(availIdx) : std::string_view(); }

		GrappleResult VesselImpl::AddCargo(size_t availIdx, std::optional<size_t> slotIdx) { return pCoreVessel ? pCoreVessel->AddCargo(availIdx, slotIdx) : GRPL_FAIL; }

		ReleaseResult VesselImpl::DeleteCargo(std::optional<size_t> slotIdx) { return pCoreVessel ? pCoreVessel->DeleteCargo(slotIdx) : RLES_FAIL; }

		GrappleResult VesselImpl::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx) { return pCoreVessel ? pCoreVessel->GrappleCargo(hCargo, slotIdx) : GRPL_FAIL; }

		ReleaseResult VesselImpl::ReleaseCargo(std::optional<size_t> slotIdx) { return pCoreVessel ? pCoreVessel->ReleaseCargo(slotIdx) : RLES_FAIL; }

		PackResult VesselImpl::PackCargo(OBJHANDLE hCargo) { return pCoreVessel ? pCoreVessel->PackCargo(hCargo) : PACK_FAIL; }

		PackResult VesselImpl::UnpackCargo(OBJHANDLE hCargo) { return pCoreVessel ? pCoreVessel->UnpackCargo(hCargo) : PACK_FAIL; }

		DrainInfo VesselImpl::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
		{
			return pCoreVessel ? pCoreVessel->DrainGrappledResource(resource, mass, slotIdx) : DrainInfo{ DRIN_FAIL, 0 };
		}

		DrainInfo VesselImpl::DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE cargoHandle)
		{
			return pCoreVessel ? pCoreVessel->DrainUngrappledResource(resource, mass, cargoHandle) : DrainInfo{ DRIN_FAIL, 0 };
		}

		DrainInfo VesselImpl::DrainStationResource(std::string_view resource, double mass, OBJHANDLE stationHandle)
		{
			return pCoreVessel ? pCoreVessel->DrainStationResource(resource, mass, stationHandle) : DrainInfo{ DRIN_FAIL, 0 };
		}

		OBJHANDLE VesselImpl::GetNearestBreathable() { return pCoreVessel ? pCoreVessel->GetNearestBreathable() : nullptr; }

		bool VesselImpl::InBreathableArea() { return pCoreVessel ? pCoreVessel->InBreathableArea() : false; }
	}
}


