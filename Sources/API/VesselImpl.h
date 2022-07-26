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

			std::string_view GetUACSVersion() override;

			bool ParseScenarioLine(char*) override;

			void clbkPostCreation() override;

			void SaveState(FILEHANDLE) override;

			size_t GetScnAstrCount() override;

			std::pair<OBJHANDLE, const AstrInfo*> GetAstrInfoByIndex(size_t) override;

			const AstrInfo* GetAstrInfoByHandle(OBJHANDLE) override;

			const VslAstrInfo* GetVslAstrInfo(OBJHANDLE) override;

			void SetScnAstrInfoByIndex(size_t, AstrInfo) override;

			bool SetScnAstrInfoByHandle(OBJHANDLE, AstrInfo) override;

			size_t GetAvailAstrCount() override;

			std::string_view GetAvailAstrName(size_t) override;

			IngressResult AddAstronaut(size_t, std::optional<size_t>, std::optional<AstrInfo>) override;

			TransferResult TransferAstronaut(size_t, size_t, std::optional<size_t>) override;

			EgressResult EgressAstronaut(size_t, size_t) override;

			size_t GetScnCargoCount() override;

			CargoInfo GetCargoInfoByIndex(size_t) override;

			std::optional<CargoInfo> GetCargoInfoByHandle(OBJHANDLE) override;

			double GetTotalCargoMass() override;

			size_t GetAvailCargoCount() override;

			std::string_view GetAvailCargoName(size_t) override;

			GrappleResult AddCargo(size_t, std::optional<size_t>) override;

			ReleaseResult DeleteCargo(std::optional<size_t>) override;

			GrappleResult GrappleCargo(OBJHANDLE, std::optional<size_t>) override;

			ReleaseResult ReleaseCargo(std::optional<size_t>) override;

			PackResult PackCargo(OBJHANDLE) override;

			PackResult UnpackCargo(OBJHANDLE) override;

			std::pair<DrainResult, double> DrainGrappledResource(std::string_view, double, std::optional<size_t>) override;

			std::pair<DrainResult, double> DrainUngrappledResource(std::string_view, double, OBJHANDLE) override;

			std::pair<DrainResult, double> DrainStationResource(std::string_view, double, OBJHANDLE) override;

			OBJHANDLE GetNearestBreathable() override;

			bool InBreathableArea() override;

		private:
			HINSTANCE coreDLL;
			Core::Vessel* pCoreVessel{};
		};
	}
}

