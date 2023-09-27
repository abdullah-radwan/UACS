#include "Module.h"
#include "..\Core\Defs.h"

namespace UACS
{
	Module::Module(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo) : coreDLL(LoadLibraryA("Modules/UACS/Core.dll"))
	{
		if (coreDLL)
		{
			auto CreateModule = reinterpret_cast<Core::CreateModule>(GetProcAddress(coreDLL, "CreateModule"));

			if (CreateModule) pCoreModule = CreateModule(pVessel, pVslAstrInfo, pVslCargoInfo);
		}

		if (!pCoreModule) oapiWriteLog("UACS module warning: Couldn't load the core DLL");
	}

	Module::~Module()
	{
		if (pCoreModule) pCoreModule->Destroy();

		if (coreDLL) FreeLibrary(coreDLL);
	}

	std::string_view Module::GetUACSVersion() { return pCoreModule ? pCoreModule->GetUACSVersion() : std::string_view(); }

	bool Module::ParseScenarioLine(char* line) { return pCoreModule ? pCoreModule->ParseScenarioLine(line) : false; }

	void Module::clbkPostCreation() { if (pCoreModule) pCoreModule->clbkPostCreation(); }

	void Module::clbkSaveState(FILEHANDLE scn) { if (pCoreModule) pCoreModule->clbkSaveState(scn); }

	size_t Module::GetScnAstrCount() { return pCoreModule ? pCoreModule->GetScnAstrCount() : 0; }

	std::pair<OBJHANDLE, const AstrInfo*> Module::GetAstrInfoByIndex(size_t astrIdx)
	{ return pCoreModule ? pCoreModule->GetAstrInfoByIndex(astrIdx) : std::pair<OBJHANDLE, const AstrInfo*>{}; }

	const AstrInfo* Module::GetAstrInfoByHandle(OBJHANDLE hAstr)
	{ return pCoreModule ? pCoreModule->GetAstrInfoByHandle(hAstr) : nullptr; }

	const VslAstrInfo* Module::GetVslAstrInfo(OBJHANDLE hVessel)
	{ return pCoreModule ? pCoreModule->GetVslAstrInfo(hVessel) : nullptr; }

	bool Module::SetAstrInfoByIndex(size_t astrIdx, const AstrInfo& astrInfo)
	{ return pCoreModule ? pCoreModule->SetAstrInfoByIndex(astrIdx, astrInfo) : false; }

	bool Module::SetAstrInfoByHandle(OBJHANDLE hAstr, const AstrInfo& astrInfo)
	{ return pCoreModule ? pCoreModule->SetAstrInfoByHandle(hAstr, astrInfo) : false; }

	size_t Module::GetAvailAstrCount() { return pCoreModule ? pCoreModule->GetAvailAstrCount() : 0; }

	std::string_view Module::GetAvailAstrName(size_t availIdx)
	{ return pCoreModule ? pCoreModule->GetAvailAstrName(availIdx) : std::string_view(); }

	double Module::GetTotalAstrMass() { return pCoreModule ? pCoreModule->GetTotalAstrMass() : 0; }

	IngressResult Module::AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx, std::optional<AstrInfo> astrInfo)
	{ return pCoreModule ? pCoreModule->AddAstronaut(availIdx, stationIdx, astrInfo) : INGRS_FAIL; }

	TransferResult Module::TransferAstronaut(std::optional<size_t> stationIdx, std::optional<size_t> airlockIdx, std::optional<size_t> tgtStationIdx)
	{ return pCoreModule ? pCoreModule->TransferAstronaut(stationIdx, airlockIdx, tgtStationIdx) : TRNS_FAIL; }

	EgressResult Module::EgressAstronaut(std::optional<size_t> stationIdx, std::optional<size_t> airlockIdx)
	{ return pCoreModule ? pCoreModule->EgressAstronaut(stationIdx, airlockIdx) : EGRS_FAIL; }

	size_t Module::GetScnCargoCount() { return pCoreModule ? pCoreModule->GetScnCargoCount() : 0; }

	CargoInfo Module::GetCargoInfoByIndex(size_t cargoIdx)
	{ return pCoreModule ? pCoreModule->GetCargoInfoByIndex(cargoIdx) : CargoInfo(); }

	std::optional<CargoInfo> Module::GetCargoInfoByHandle(OBJHANDLE hCargo)
	{ return pCoreModule ? pCoreModule->GetCargoInfoByHandle(hCargo) : std::nullopt; }

	std::optional<std::vector<std::string>> Module::GetStationResources(OBJHANDLE hStation)
	{ return pCoreModule ? pCoreModule->GetStationResources(hStation) : std::nullopt; }

	size_t Module::GetAvailCargoCount() { return pCoreModule ? pCoreModule->GetAvailCargoCount() : 0; }

	std::string_view Module::GetAvailCargoName(size_t availIdx)
	{ return pCoreModule ? pCoreModule->GetAvailCargoName(availIdx) : std::string_view(); }

	double Module::GetTotalCargoMass() { return pCoreModule ? pCoreModule->GetTotalCargoMass() : 0; }

	GrappleResult Module::AddCargo(size_t availIdx, std::optional<size_t> slotIdx)
	{ return pCoreModule ? pCoreModule->AddCargo(availIdx, slotIdx) : GRPL_FAIL; }

	ReleaseResult Module::DeleteCargo(std::optional<size_t> slotIdx)
	{ return pCoreModule ? pCoreModule->DeleteCargo(slotIdx) : RLES_FAIL; }

	GrappleResult Module::GrappleCargo(OBJHANDLE hCargo, std::optional<size_t> slotIdx)
	{ return pCoreModule ? pCoreModule->GrappleCargo(hCargo, slotIdx) : GRPL_FAIL; }

	ReleaseResult Module::ReleaseCargo(std::optional<size_t> slotIdx)
	{ return pCoreModule ? pCoreModule->ReleaseCargo(slotIdx) : RLES_FAIL; }

	PackResult Module::PackCargo(OBJHANDLE hCargo)
	{ return pCoreModule ? pCoreModule->PackCargo(hCargo) : PACK_FAIL; }

	PackResult Module::UnpackCargo(OBJHANDLE hCargo)
	{ return pCoreModule ? pCoreModule->UnpackCargo(hCargo) : PACK_FAIL; }

	std::pair<DrainResult, double> Module::DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx)
	{ return pCoreModule ? pCoreModule->DrainGrappledResource(resource, mass, slotIdx) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 }; }

	std::pair<DrainResult, double> Module::DrainScenarioResource(std::string_view resource, double mass, OBJHANDLE hCargo)
	{ return pCoreModule ? pCoreModule->DrainScenarioResource(resource, mass, hCargo) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 }; }

	std::pair<DrainResult, double> Module::DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation)
	{ return pCoreModule ? pCoreModule->DrainStationResource(resource, mass, hStation) : std::pair<DrainResult, double>{ DRIN_FAIL, 0 }; }
}