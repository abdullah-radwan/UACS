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
			VESSEL* pVessel;
			API::VslAstrInfo* pVslAstrInfo;
			API::VslCargoInfo* pVslCargoInfo;

			API::AstrInfo* pLoadAstrInfo{};

			std::vector<VECTOR3> GetNearbyAstrs(const VECTOR3& airlockPos);
			bool GetNearestAstrEmptyPos(VECTOR3& initialPos);
			double GetAstrHeight(std::string_view className);

			bool passRangeCheck{};

			API::CargoInfo SetCargoInfo(API::Cargo* pCargo);
			OBJHANDLE GetSlotCargoHandle(size_t slotIdx);

			struct SlotResult
			{
				size_t slotIdx;
				bool open;
				OBJHANDLE hCargo;
			};

			std::optional<SlotResult> GetEmptySlot(bool mustBeOpen);
			std::optional<SlotResult> GetOccupiedSlot(bool mustBeOpen);

			std::vector<VECTOR3> GetNearbyCargoes(const VECTOR3& slotPos);
			bool GetNearestCargoEmptyPos(VECTOR3& initialPos);
		};
	}
}
