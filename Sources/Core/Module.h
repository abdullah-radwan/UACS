#pragma once
#include "..\API\Module.h"
#include "..\API\Cargo.h"

#include <span>

namespace UACS
{
	namespace Core
	{
		class Module
		{
		public:
			Module(VESSEL*, UACS::VslAstrInfo*, UACS::VslCargoInfo*);
			virtual void Destroy() noexcept;

			virtual std::string_view GetUACSVersion();

			virtual bool ParseScenarioLine(char*);

			virtual void clbkPostCreation();

			virtual void clbkSaveState(FILEHANDLE);

			virtual size_t GetScnAstrCount();

			virtual std::pair<OBJHANDLE, const UACS::AstrInfo*> GetAstrInfoByIndex(size_t);

			virtual const UACS::AstrInfo* GetAstrInfoByHandle(OBJHANDLE);

			virtual const UACS::VslAstrInfo* GetVslAstrInfo(OBJHANDLE);

			virtual bool SetAstrInfoByIndex(size_t, const UACS::AstrInfo&);

			virtual bool SetAstrInfoByHandle(OBJHANDLE, const UACS::AstrInfo&);

			virtual size_t GetAvailAstrCount();

			virtual std::string_view GetAvailAstrName(size_t);

			virtual double GetTotalAstrMass();

			virtual UACS::IngressResult AddAstronaut(size_t, std::optional<size_t>, std::optional<UACS::AstrInfo>);

			virtual UACS::TransferResult TransferAstronaut(size_t, size_t, std::optional<size_t>);

			virtual UACS::EgressResult EgressAstronaut(size_t, size_t);

			virtual size_t GetScnCargoCount();

			virtual UACS::CargoInfo GetCargoInfoByIndex(size_t);

			virtual std::optional<UACS::CargoInfo> GetCargoInfoByHandle(OBJHANDLE);

			virtual std::optional<std::vector<std::string>> GetStationResources(OBJHANDLE);

			virtual size_t GetAvailCargoCount();

			virtual std::string_view GetAvailCargoName(size_t);

			virtual double GetTotalCargoMass();

			virtual UACS::GrappleResult AddCargo(size_t, std::optional<size_t>);

			virtual UACS::ReleaseResult DeleteCargo(std::optional<size_t>);

			virtual UACS::GrappleResult GrappleCargo(OBJHANDLE, std::optional<size_t>);

			virtual UACS::ReleaseResult ReleaseCargo(std::optional<size_t>);

			virtual UACS::PackResult PackCargo(OBJHANDLE);

			virtual UACS::PackResult UnpackCargo(OBJHANDLE);

			virtual std::pair<UACS::DrainResult, double> DrainGrappledResource(std::string_view, double, std::optional<size_t>);

			virtual std::pair<UACS::DrainResult, double> DrainScenarioResource(std::string_view, double, OBJHANDLE);

			virtual std::pair<UACS::DrainResult, double> DrainStationResource(std::string_view, double, OBJHANDLE);

		private:
			inline static std::vector<std::string> availCargoVector, availAstrVector;
			static void InitAvailCargo();
			static void InitAvailAstr();

			VESSEL* pVessel;
			UACS::VslAstrInfo* pVslAstrInfo;
			UACS::VslCargoInfo* pVslCargoInfo;

			UACS::AstrInfo* pLoadAstrInfo{};
			bool passCheck{};

			double GetTgtVslDist(VESSEL* pTgtVsl) const;
			void SetAttachPos(bool attach, bool unpacked, const UACS::SlotInfo& slotInfo);			

			template<typename T>
			bool SetGroundPos(const VESSELSTATUS2& vslStatus, VECTOR3& initPos, UACS::GroundInfo gndInfo, std::span<T*> objSpan, const T* pOrgObj = nullptr);
			void SetGroundDir(VECTOR3& dir);

			UACS::CargoInfo SetCargoInfo(UACS::Cargo* pCargo);
			UACS::SlotInfo& GetEmptySlot(bool mustBeOpen);
			UACS::SlotInfo& GetOccupiedSlot(bool mustBeOpen);
		};
	}
}