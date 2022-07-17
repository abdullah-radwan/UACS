#pragma once
#include "Common.h"

namespace UACS
{
	namespace API
	{
		struct SlotInfo
		{
			ATTACHMENTHANDLE hAttach;
			bool open;
		};

		struct VslCargoInfo
		{
			std::vector<SlotInfo> slots;

			/**
			 * @brief The astronaut mode flag. True if activated, false if not.
			 * 
			 * In astronaut mode, the following happens:
			 * 1. Unpacked cargo can be grappled.
			 * 2. No release distance is added in front of the vessel.
			 * 3. If a cargo is already in front of the vessel, the grappled cargo cannot be released. In normal mode, cargoes will be released in rows.
			*/
			bool astrMode{ false };

			/// The cargo grapple range in meters.
			double grappleRange = 50;

			/// The cargo release velocity if released in space, in meters.
			double relVel = 0.05;

			/// The number of cargoes in a single row when released on ground. Not applicable when astronaut mode is activated.
			int relRowCount = 4;

			/// The search range for packable and unpackable cargoes in meters.
			double packRange = 50;

			/// The search range for draining resources in meters.
			double drainRange = 100;

			/// The search range for breathable cargoes in meters.
			double breathableRange = 100;

			/// The maximum single cargo mass in kilograms.
			std::optional<double> maxCargoMass;

			/// The maximum total cargo mass in kilograms.
			std::optional<double> maxTotalCargoMass;
		};

		struct CargoInfo
		{
			OBJHANDLE handle;

			bool attached;

			/// The cargo name in the scenario.
			std::string name;

			/// The cargo mass in kilograms.
			double mass;

			CargoType type;

			/// The cargo resource. If cargo isn't a resource, the string will be empty.
			std::string resource;

			bool unpacked;

			/**
			 * @note The cargo is breathable only if it's unpacked. This flag will be true even if the cargo is packed.
			*/
			bool breathable;
		};

		enum TransferResult
		{
			TRNS_SUCCEDED,

			TRNS_STN_EMPTY,

			TRNS_ARLCK_CLSD,

			/// The passed airlock has no docking port.
			TRNS_DOCK_UNDEF,

			/// The passed airlock docking port is empty.
			TRNS_DOCK_EMPTY,

			/// The target vessel has no airlocks associated with the docking port.
			TRNS_TGT_ARLCK_UNDEF,

			/// The target airlock is closed.
			TRNS_TGT_ARLCK_CLSD,

			/// The target vessel has no stations if tgtStationIdx is not passed, or the passed target station is invalid.
			TRNS_TGT_STN_UNDEF,

			/// All of the target vessel stations are occupied if tgtStationIdx is not passed, or the passed target station is occupied.
			TRNS_TGT_STN_OCCP,

			TRNS_FAIL
		};

		enum EgressResult
		{
			EGRS_SUCCEDED,

			EGRS_STN_EMPTY,

			EGRS_ARLCK_CLSD,

			/// A vessel is docked to the docking port associated with the passed airlock.
			EGRS_ARLCK_DCKD,

			/// No empty position to egress the astronaut on ground.
			EGRS_NO_EMPTY_POS,

			EGRS_FAIL
		};

		enum GrappleResult
		{
			GRPL_SUCCED,

			/// All slots are closed if slotIdx is not passed, or the passed slot is closed.
			GRPL_SLT_CLSD,

			/// All slots are occupied if slotIdx is not passed, or the passed slot is occupied.
			GRPL_SLT_OCCP,

			/// No grappable cargo in the grapple range if hCargo is nullptr, or the passed cargo is outside the grapple range.
			GRPL_NOT_IN_RNG,

			/**
			 * @brief The passed cargo is unpacked so it can't be grappled.
			 * @note Not applicable with the astronaut mode activated.
			*/
			GRPL_CRG_UNPCKD,

			/// The passed cargo is attached to another vessel.
			GRPL_CRG_ATCHD,

			GRPL_MAX_MASS_EXCD,

			GRPL_MAX_TTLMASS_EXCD,

			GRPL_FAIL
		};

		enum ReleaseResult
		{
			RLES_SUCCED,

			/// All slots are closed if slotIdx is not passed, or the passed slot is closed.
			RLES_SLT_CLSD,

			/// All slots are empty if slotIdx is not passed, or the passed slot is empty.
			RLES_SLT_EMPTY,

			/// No empty position to release the cargo on ground.
			RLES_NO_EMPTY_POS,

			RLES_FAIL
		};

		enum PackResult
		{
			PACK_SUCCED,

			/// No packable cargo in the packing range if hCargo is nullptr, or the passed cargo is outside the packing range.
			PACK_NOT_IN_RNG,

			/**
			 * @brief The passed cargo is already packed
			 * @note Returned only by PackCargo method.
			*/
			PACK_CRG_PCKD,

			/**
			 * @brief The passed cargo is already unpacked
			 * @note Returned only by UnpackCargo method.
			*/
			PACK_CRG_UNPCKD,

			/// The passed cargo is not packable for PackCargo method or not unpackable for UnpackCargo method.
			PACK_CRG_NOT_PCKABL,

			/// The passed cargo is attached to another vessel.
			PACK_CRG_ATCHD,

			PACK_FAIL
		};

		enum DrainResult
		{
			DRIN_SUCCED,

			/**
			 * @brief All slots are empty if slotIdx is not passed, or the passed slot is empty.
			 * @note Returned by DrainGrappledResource method only.
			*/
			DRIN_SLT_EMPTY,

			/**
			 * @brief No drainable cargo or station in the drainage range if hCargo or hStation is nullptr, or the passed vessel is outside the drainage range.
			 * @note Not returned by DrainGrappledResource method.
			*/
			DRIN_NOT_IN_RNG,

			/// The passed cargo is not a resource.
			DRIN_NOT_RES,

			/// The passed cargo resource doesn't match the requested resource.
			DRIN_RES_NOMATCH,

			DRIN_FAIL
		};

		struct DrainInfo
		{
			DrainResult result;

			/// The drained mass in kilograms. 0 if no resource was drained.
			double mass;
		};

		class Vessel
		{
		public:
			/**
			 * @brief Creates an instance from the vessel API. Can be called from anywhere.
			 * @param pVessel Pointer to the vessel class.
			 * @param pVslAstrInfo Vessel astronaut information as the VslAstrInfo struct. If nullptr is passed, astronaut methods shouldn't be called.
			 * @param pVslCargoInfo Vessel cargo information as the VslCargoInfo struct. If nullptr is passed, cargo methods shouldn't be called.
			 * @return An instance of the vessel API.
			*/
			static std::unique_ptr<Vessel> CreateInstance(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo);
			virtual ~Vessel() = default;
			
			/**
			 * @brief Parses the scenario file to parse UACS data. It must be called in the vessel ParseScenarioLine method.
			 * 
			 * Currently, this is only needed to load astronaut data.
			 * @param line The scenario line as passed from the vessel ParseScenarioLine method.
			 * @return True if UACS data was parsed, false if not.
			*/
			virtual bool ParseScenarioLine(char* line) = 0;

			/**
			 * @brief Saves UACS data to the scenario file. It must be callsed in the vessel clbkSaveState method.
			 * 
			 * Currently, this is only needed to save astronaut data.
			 * @param scn The scenario file.
			*/
			virtual void SaveState(FILEHANDLE scn) = 0;

			/**
			 * @brief Gets astronaut information from an astronaut vessel.
			 * @param hVessel The astronaut vessel handle.
			 * @return A pointer to the astronaut information struct, or nullptr if hVessel is invalid or isn't an astronaut.
			*/
			virtual const AstrInfo* GetAstrInfo(OBJHANDLE hVessel) = 0;

			/**
			 * @brief Gets vessel astronaut information from a vessel.
			 * @param hVessel The vessel handle.
			 * @return A pointer to the vessel astronaut information struct, or nullptr if hVessel is invalid or isn't an astronaut.
			*/
			virtual const VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel) = 0;

			/**
			 * @brief Transfers the astronaut in the passed station to the vessel docked to the docking port associated with the passed airlock.
			 * @param stationIdx The astronaut station index.
			 * @param airlockIdx The airlock index. A vessel with UACS astronaut implementation must be docked to the docking port associated with the airlock.
			 * @param tgtStationIdx The target vessel station index to transfer the astronaut into. If not passed, the first empty station will be used.
			 * @return The transfer result as the TransferResult enum.
			*/
			virtual TransferResult TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx = {}) = 0;

			/**
			 * @brief Egresses the astronaut in the passed station via the passed airlock.
			 * 
			 * In space, the astronaut is egressed directly in front of the airlock. On ground, the astronaut is egressed at the closest location to the airlock position.
			 * @param stationIdx The astronaut station index.
			 * @param airlockIdx The airlock index. 
			 * @return The egress result as the EgressResult enum.
			*/
			virtual EgressResult EgressAstronaut(size_t stationIdx, size_t airlockIdx) = 0;



			/**
			 * @brief Gets total cargo count in the scenario.
			 * @return Total cargo count in the scenario.
			*/
			virtual size_t GetScenarioCargoCount() = 0;

			/**
			 * @brief Gets cargo information by the cargo index.
			 * @param cargoIdx The cargo index. Must be less than the total cargo count in the scenario.
			 * @return The cargo information as the CargoInfo struct.
			*/
			virtual CargoInfo GetCargoInfoByIndex(size_t cargoIdx) = 0;

			/**
			 * @brief Gets cargo information by the cargo handle.
			 * @param hVessel The cargo handle.
			 * @return The cargo information as the CargoInfo struct if the handle is a cargo.
			*/
			virtual std::optional<CargoInfo> GetCargoInfoByHandle(OBJHANDLE hVessel) = 0;

			/**
			 * @brief Gets cargo information by the slot index.
			 * @param slotIdx The slot index.
			 * @return The cargo information as the CargoInfo struct if the index slot contains a cargo.
			*/
			virtual std::optional<CargoInfo> GetCargoInfoBySlot(size_t slotIdx) = 0;

			/**
			 * @brief Gets the total mass of all grappled cargoes.
			 * @return The total mass of all grappled cargoes.
			*/
			virtual double GetTotalCargoMass() = 0;

			/**
			 * @brief Gets the available cargo count, which is the count of cargoes that can be added.
			 * 
			 * It's the number of config files in 'Config\Vessels\UACS\Cargoes'.
			 * @return The count of cargoes that can be added.
			*/
			virtual int GetAvailableCargoCount() = 0;

			/**
			 * @brief Gets the name of an available cargo.
			 * @param availIdx The available cargo index. Must be greater than 0 and less than the available cargo count.
			 * @return The available cargo name.
			*/
			virtual std::string_view GetAvailableCargoName(size_t availIdx) = 0;

			/**
			 * @brief Adds the passed available cargo to the passed slot.
			 * @param availIdx The available cargo index. Must be less than the available cargo count.
			 * @param slotIdx The slot index. If not passed, the first empty slot is used.
			 * @return The addition result as the GrappleResult enum.
			*/
			virtual GrappleResult AddCargo(size_t availIdx, std::optional<size_t> slotIdx = {}) = 0;

			/**
			 * @brief Deletes the cargo in the passed slot.
			 * @param slotIdx The slot index. If not passed, the first occupied slot is used.
			 * @return The delete result as the ReleaseResult enum.
			*/
			virtual ReleaseResult DeleteCargo(std::optional<size_t> slotIdx = {}) = 0;

			/**
			 * @brief Grapples the passed cargo into the passed slot.
			 * @param hCargo The cargo handle. If nullptr is passed, the nearest cargo in the grapple range is used.
			 * @param slotIdx The slot index. If not passed, the first empty slot is used.
			 * @return The grapple result as the GrappleResult enum.
			*/
			virtual GrappleResult GrappleCargo(OBJHANDLE hCargo = nullptr, std::optional<size_t> slotIdx = {}) = 0;

			/**
			 * @brief Releases the cargo in the passed slot.
			 * 
			 * If releasing in space, the cargo is released at the slot position with the specified release velocity in the vessel cargo information struct.
			 * On ground, the cargo is released at the closest location to the slot position.
			 * @param slotIdx The slot index. If not passed, the first occupied slot will be used.
			 * @return The release result as the ReleaseResult enum. 
			*/
			virtual ReleaseResult ReleaseCargo(std::optional<size_t> slotIdx = {}) = 0;

			/**
			 * @brief Packs the passed cargo.
			 * @param hCargo The cargo handle. If nullptr is passed, the nearest packable cargo in the packing range is used.
			 * @return The packing result as the PackResult enum.
			*/
			virtual PackResult PackCargo(OBJHANDLE hCargo = nullptr) = 0;

			/**
			 * @brief Unpacks the passed cargo.
			 * @param hCargo The cargo handle. If nullptr is passed, the nearest unpackable cargo in the packing range is used.
			 * @return The unpacking result as the PackResult enum.
			*/
			virtual PackResult UnpackCargo(OBJHANDLE hCargo = nullptr) = 0;

			/**
			 * @brief Drains the passed resource from the cargo in the passed slot.
			 * @param resource The resource name. See the UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param slotIdx The cargo slot index. If not passed, the first suitable cargo is used.
			 * @return The drainage result as the DrainInfo struct.
			*/
			virtual DrainInfo DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx = {}) = 0;

			/**
			 * @brief Drains the passed resource from the passed cargo.
			 * @param resource The resource name. See the UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param hCargo The cargo handle. If nullptr is passed, the nearest suitable cargo in the drainage range is used.
			 * @return The drainage result as the DrainInfo struct.
			*/
			virtual DrainInfo DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE hCargo = nullptr) = 0;

			/**
			 * @brief Drains the passed resource from the passed station.
			 * @param resource The resource name. See the UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param hStation The station handle. If nullptr is passed, the nearest suitable station in the drainage range is used.
			 * @return The drainage result as the DrainInfo struct.
			*/
			virtual DrainInfo DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation = nullptr) = 0;

			/**
			 * @brief Gets the nearest breathable cargo in the breathable search range.
			 * @return The nearest breathable cargo.
			*/
			virtual OBJHANDLE GetNearestBreathable() = 0;

			/**
			 * @brief Determines whether the vessel is in a breathable area.
			 * 
			 * The vessel is in a breathable area if the distance between the vessel and the nearest breathable cargo is less than the breathable cargo size.
			 * @return True if the vessel is in a breathable area, false if not.
			*/
			virtual bool InBreathableArea() = 0;

		protected:
			/// Protected to prevent incorrect initialization. Use CreateInstance instead.
			Vessel() = default;
		};
	};
}