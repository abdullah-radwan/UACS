#pragma once
#include "Vessel.h"
#include "..\Core\CoreDefs.h"

namespace UACS
{
	namespace API
	{
		class VesselImpl : public API::Vessel
		{
		public:
			VesselImpl(VESSEL*, VslAstrInfo*, VslCargoInfo*);
			~VesselImpl();

			bool ParseScenarioLine(char*) override;

			void SaveState(FILEHANDLE) override;

			const API::AstrInfo* GetAstrInfo(OBJHANDLE) override;

			const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE) override;

			API::TransferResult TransferAstronaut(size_t, size_t, std::optional<size_t>) override;

			API::EgressResult EgressAstronaut(size_t, size_t) override;

			size_t GetScenarioCargoCount() override;

			API::CargoInfo GetCargoInfoByIndex(size_t) override;

			std::optional<API::CargoInfo> GetCargoInfoByHandle(OBJHANDLE) override;

			std::optional<API::CargoInfo> GetCargoInfoBySlot(size_t) override;

			double GetTotalCargoMass() override;

			int GetAvailableCargoCount() override;

			std::string_view GetAvailableCargoName(size_t) override;

			API::GrappleResult AddCargo(size_t, std::optional<size_t>) override;

			API::ReleaseResult DeleteCargo(std::optional<size_t>) override;

			API::GrappleResult GrappleCargo(OBJHANDLE, std::optional<size_t>) override;

			API::ReleaseResult ReleaseCargo(std::optional<size_t>) override;

			API::PackResult PackCargo(OBJHANDLE) override;

			API::PackResult UnpackCargo(OBJHANDLE) override;

			API::DrainInfo DrainGrappledResource(std::string_view, double, std::optional<size_t>) override;

			API::DrainInfo DrainUngrappledResource(std::string_view, double, OBJHANDLE) override;

			API::DrainInfo DrainStationResource(std::string_view, double, OBJHANDLE) override;

			OBJHANDLE GetNearestBreathable() override;

			bool InBreathableArea() override;

		private:
			HINSTANCE coreDLL;
			Core::Vessel* pCoreVessel{};
		};
	}
}

