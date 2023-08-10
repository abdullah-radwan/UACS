#pragma once
#include "..\API\Vessel.h"
#include "..\API\Cargo.h"

#include <span>

namespace UACS
{
	namespace Core
	{
		class Vessel
		{
		public:
			Vessel(VESSEL*, API::VslAstrInfo*, API::VslCargoInfo*);
			virtual void Destroy() noexcept;

			virtual std::string_view GetUACSVersion();

			virtual bool ParseScenarioLine(char*);

			virtual void clbkPostCreation();

			virtual void clbkSaveState(FILEHANDLE);

			virtual size_t GetScnAstrCount();

			virtual std::pair<OBJHANDLE, const API::AstrInfo*> GetAstrInfoByIndex(size_t);

			virtual const API::AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			virtual const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			virtual size_t GetAvailAstrCount();

			virtual std::string_view GetAvailAstrName(size_t);

			virtual API::IngressResult AddAstronaut(size_t, std::optional<size_t>, std::optional<API::AstrInfo>);

			virtual API::TransferResult TransferAstronaut(size_t, size_t, std::optional<size_t>);

			virtual API::EgressResult EgressAstronaut(size_t, size_t);

			virtual size_t GetScnCargoCount();

			virtual API::CargoInfo GetCargoInfoByIndex(size_t);

			virtual std::optional<API::CargoInfo> GetCargoInfoByHandle(OBJHANDLE);

			virtual double GetTotalCargoMass();

			virtual size_t GetAvailCargoCount();

			virtual std::string_view GetAvailCargoName(size_t);

			virtual API::GrappleResult AddCargo(size_t, std::optional<size_t>);

			virtual API::ReleaseResult DeleteCargo(std::optional<size_t>);

			virtual API::GrappleResult GrappleCargo(OBJHANDLE, std::optional<size_t>);

			virtual API::ReleaseResult ReleaseCargo(std::optional<size_t>);

			virtual API::PackResult PackCargo(OBJHANDLE);

			virtual API::PackResult UnpackCargo(OBJHANDLE);

			virtual std::pair<API::DrainResult, double> DrainGrappledResource(std::string_view, double, std::optional<size_t>);

			virtual std::pair<API::DrainResult, double> DrainScenarioResource(std::string_view, double, OBJHANDLE);

			virtual std::pair<API::DrainResult, double> DrainStationResource(std::string_view, double, OBJHANDLE);

		private:
			inline static std::vector<std::string> availCargoVector, availAstrVector;
			static void InitAvailCargo();
			static void InitAvailAstr();

			VESSEL* pVessel;
			API::VslAstrInfo* pVslAstrInfo;
			API::VslCargoInfo* pVslCargoInfo;

			API::AstrInfo* pLoadAstrInfo{};
			bool passCheck{};

			template<typename T>
			bool SetGroundPos(const VESSELSTATUS2& vslStatus, VECTOR3& initPos, API::GroundInfo gndInfo, std::span<T*> objSpan, const T* pOrgObj = nullptr);
			void SetGroundDir(VECTOR3& dir);

			API::CargoInfo SetCargoInfo(API::Cargo* pCargo);
			bool StationHasResource(VESSEL* pStation, std::string_view resource);

			API::SlotInfo& GetEmptySlot(bool mustBeOpen);
			API::SlotInfo& GetOccupiedSlot(bool mustBeOpen);
		};
	}
}