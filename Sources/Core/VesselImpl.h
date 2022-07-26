#pragma once
#include "CoreCommon.h"

namespace UACS
{
	namespace Core
	{
		class VesselImpl : public Vessel
		{
		public:
			VesselImpl(VESSEL*, API::VslAstrInfo*, API::VslCargoInfo*);
			void Destroy() noexcept override;

			std::string_view GetUACSVersion() override;

			bool ParseScenarioLine(char*) override;

			void clbkPostCreation() override;

			void SaveState(FILEHANDLE) override;

			size_t GetScnAstrCount() override;

			std::pair<OBJHANDLE, const API::AstrInfo*> GetAstrInfoByIndex(size_t) override;

			const API::AstrInfo* GetAstrInfoByHandle(OBJHANDLE) override;

			const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE) override;

			void SetScnAstrInfoByIndex(size_t, API::AstrInfo) override;

			bool SetScnAstrInfoByHandle(OBJHANDLE, API::AstrInfo) override;

			size_t GetAvailAstrCount() override;

			std::string_view GetAvailAstrName(size_t) override;

			API::IngressResult AddAstronaut(size_t, std::optional<size_t>, std::optional<API::AstrInfo>) override;

			API::TransferResult TransferAstronaut(size_t, size_t, std::optional<size_t>) override;

			API::EgressResult EgressAstronaut(size_t, size_t) override;

			size_t GetScnCargoCount() override;

			API::CargoInfo GetCargoInfoByIndex(size_t) override;

			std::optional<API::CargoInfo> GetCargoInfoByHandle(OBJHANDLE) override;

			double GetTotalCargoMass() override;

			size_t GetAvailCargoCount() override;

			std::string_view GetAvailCargoName(size_t) override;

			API::GrappleResult AddCargo(size_t, std::optional<size_t>) override;

			API::ReleaseResult DeleteCargo(std::optional<size_t>) override;

			API::GrappleResult GrappleCargo(OBJHANDLE, std::optional<size_t>) override;

			API::ReleaseResult ReleaseCargo(std::optional<size_t>) override;

			API::PackResult PackCargo(OBJHANDLE) override;

			API::PackResult UnpackCargo(OBJHANDLE) override;

			std::pair<API::DrainResult, double> DrainGrappledResource(std::string_view, double, std::optional<size_t>) override;

			std::pair<API::DrainResult, double> DrainUngrappledResource(std::string_view, double, OBJHANDLE) override;

			std::pair<API::DrainResult, double> DrainStationResource(std::string_view, double, OBJHANDLE) override;

			OBJHANDLE GetNearestBreathable() override;

			bool InBreathableArea() override;

		private:
			inline static std::vector<std::string> availCargoVector, availAstrVector;
			static void InitAvailCargo();
			static void InitAvailAstr();

			VESSEL* pVessel;
			API::VslAstrInfo* pVslAstrInfo;
			API::VslCargoInfo* pVslCargoInfo;

			API::AstrInfo* pLoadAstrInfo{};

			std::vector<VECTOR3> GetNearbyAstrs(const VECTOR3& airlockPos);
			bool GetNearestAstrEmptyPos(VECTOR3& initialPos);
			double GetAstrHeight(std::string_view className);

			bool passRangeCheck{};

			API::CargoInfo SetCargoInfo(API::Cargo* pCargo);

			API::SlotInfo& GetEmptySlot(bool mustBeOpen);
			API::SlotInfo& GetOccupiedSlot(bool mustBeOpen);

			std::vector<VECTOR3> GetNearbyCargoes(const VECTOR3& slotPos);
			bool GetNearestCargoEmptyPos(VECTOR3& initialPos);
		};
	}
}
