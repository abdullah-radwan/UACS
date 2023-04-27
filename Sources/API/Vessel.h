#pragma once
#include "Common.h"

/**
 * @file
 * @brief UACS vessel API header.
*/

namespace UACS
{
	namespace Core { class Vessel; }

	namespace API
	{
		struct CargoInfo
		{
			OBJHANDLE handle;
			bool attached;
			CargoType type;
			bool unpacked;

			/**
			 * @brief While the cargo is breathable only if it's unpacked, this flag will be true even if the cargo is packed.
			 * 
			 * The cargo is considered breathable in its current state if both unpacked and breathable flags are true.
			*/
			bool breathable;

			std::optional<std::string> resource;
		};

		struct SlotInfo
		{
			ATTACHMENTHANDLE hAttach;

			bool open{ true };

			/// The cargo release velocity (if released in space) in meters per second.
			double relVel{};

			/// The slot ground release information.
			GroundInfo gndInfo{};

			std::optional<CargoInfo> cargoInfo{};
		};

		struct VslCargoInfo
		{
			std::vector<SlotInfo> slots;

			/**
			 * @brief The astronaut mode flag.
			 * 
			 * In astronaut mode, the following happens:
			 * 1. Unpacked cargo can be grappled.
			 * 2. No release distance is added in front of the vessel.
			 * 3. If a cargo is already in front of the vessel, the grappled cargo cannot be released. In normal mode, cargoes will be released in rows.
			*/
			bool astrMode{ false };

			/// The cargo grapple range in meters.
			double grappleRange{ 50 };

			/// The search range for packable and unpackable cargoes in meters.
			double packRange{ 50 };

			/// The search range for draining resources in meters.
			double drainRange{ 100 };

			/// The maximum single cargo mass in kilograms.
			std::optional<double> maxCargoMass;

			/// The maximum total cargo mass in kilograms.
			std::optional<double> maxTotalCargoMass;
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

			/// The passed target station (or all target stations if tgtStationIdx is nullopt) is invalid.
			TRNS_TGT_STN_UNDEF,

			/// The passed target station (or all target stations if tgtStationIdx is nullopt) is occupied.
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

			/// No empty position to egress the astronaut on the ground.
			EGRS_NO_EMPTY_POS,

			EGRS_FAIL
		};

		enum GrappleResult
		{
			GRPL_SUCCED,

			/// The passed slot (or all slots if slotIdx is nullopt) is closed.
			GRPL_SLT_CLSD,

			/// The passed slot (or all slots if slotIdx is nullopt) is occupied.
			GRPL_SLT_OCCP,

			/// The passed cargo (or all grappable cargoes if hCargo is nullptr) is outside the grapple range.
			GRPL_NOT_IN_RNG,

			/// The passed cargo can't be grappled because it is unpacked. Not applicable with the astronaut mode activated.
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

			/// The passed slot (or all slots if slotIdx is nullopt) is closed.
			RLES_SLT_CLSD,

			/// The passed slot (or all slots if slotIdx is nullopt) is empty.
			RLES_SLT_EMPTY,

			/// No empty position to release the cargo on ground.
			RLES_NO_EMPTY_POS,

			RLES_FAIL
		};

		enum PackResult
		{
			PACK_SUCCED,

			/// The passed cargo (or all packable cargoes if hCargo is nullptr) is outside the packing range.
			PACK_NOT_IN_RNG,

			/// The passed cargo is already packed. Returned only by PackCargo method.
			PACK_CRG_PCKD,

			/// The passed cargo is already unpacked. Returned only by UnpackCargo method.
			PACK_CRG_UNPCKD,

			/// The passed cargo is not packable if PackCargo method is called, or not unpackable if UnpackCargo method is called.
			PACK_CRG_NOT_PCKABL,

			/// The passed cargo is attached to another vessel.
			PACK_CRG_ATCHD,

			PACK_FAIL
		};

		enum DrainResult
		{
			DRIN_SUCCED,

			/// The passed slot (or all slots if slotIdx is nullopt) is empty. Returned only by DrainGrappledResource method.
			DRIN_SLT_EMPTY,

			/**
			 * @brief The passed vessel (or all drainable cargoes or stations if hCargo or hStation is nullptr) is outside the drainage range.
			 * NOT returned by DrainGrappledResource method.
			*/
			DRIN_NOT_IN_RNG,

			/// The passed cargo or vessel resource (if any) doesn't match the passed resource.
			DRIN_RES_NOMATCH,

			DRIN_FAIL
		};

		/// UACS vessel API.
		class Vessel
		{
		public:
			/**
			 * @brief Creates an instance from the vessel API. It can be called from anywhere.
			 * @param pVessel A pointer to the vessel class.
			 * @param pVslAstrInfo A pointer to the vessel astronaut information. If nullptr is passed, astronaut methods shouldn't be called.
			 * @param pVslCargoInfo A pointer to vessel cargo information. If nullptr is passed, cargo methods shouldn't be called.
			 * @note All pointers must live until the vessel API instance is destroyed. Don't pass the address of temporary variables.
			 * @return An instance of the vessel API even if UACS isn't installed. To know if UACS isn't installed, use GetUACSVersion.
			*/
			Vessel(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo);
			~Vessel();

			/**
			 * @brief Gets UACS version. It can be used to know if UACS is installed.
			 * @return UACS version if UACS is installed, or an empty string view if not.
			*/
			std::string_view GetUACSVersion();
			
			/**
			 * @brief Parses the scenario file to load UACS information. It must be called in the vessel ParseScenarioLine method.
			 * 
			 * Currently, this is used to load astronaut information.
			 * @param line The scenario line from the vessel ParseScenarioLine method.
			 * @return True if UACS information was loaded, false if not.
			*/
			bool ParseScenarioLine(char* line);

			/**
			 * @brief Finishes UACS initialization.
			 * It must be called once from the vessel clbkPostCreation method, after defining all airlocks, stations, and slots.
			 * 
			 * Currently, this is used to link grappled cargoes to their slots after loading the scenario.
			*/
			void clbkPostCreation();

			/**
			 * @brief Saves UACS information to the scenario file. It must be called in the vessel clbkSaveState method.
			 * 
			 * Currently, this is used to save astronaut information.
			 * @param scn The scenario file.
			*/
			void SaveState(FILEHANDLE scn);

			/**
			 * @brief Gets the astronaut count in the scenario.
			 * @return The astronaut count in the scenario.
			*/
			size_t GetScnAstrCount();

			/**
			 * @brief Gets an astronaut information by the astronaut index.
			 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
			 * @return A pair of the astronaut vessel handle and a pointer to the astronaut AstrInfo struct.
			*/
		    std::pair<OBJHANDLE, const AstrInfo*> GetAstrInfoByIndex(size_t astrIdx);

			/**
			 * @brief Gets an astronaut information by the astronaut handle.
			 * @param hAstr The astronaut vessel handle.
			 * @return A pointer to the astronaut AstrInfo struct, or nullptr if hAstr is invalid or not an astronaut.
			*/
			const AstrInfo* GetAstrInfoByHandle(OBJHANDLE hAstr);

			/**
			 * @brief Gets a vessel astronaut information by the vessel handle.
			 * @param hVessel The vessel handle.
			 * @return A pointer to the vessel VslAstrInfo struct, or nullptr if hVessel is invalid or not an astronaut.
			*/
			const VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel);

			/**
			 * @brief Sets a scenario astronaut information by the astronaut index.
			 * @note This is used to change a scenario astronaut information.
			 * To change a vessel astronaut information, change the information in the VslAstrInfo struct directly.
			 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
			 * @param astrInfo The astronaut information. The astronaut class name must not be changed.
			*/
			void SetScnAstrInfoByIndex(size_t astrIdx, AstrInfo astrInfo);

			/**
			 * @brief Sets a scenario astronaut information by the astronaut handle.
			 * @note This is used to change a scenario astronaut information.
			 * To change a vessel astronaut information, change the information in the VslAstrInfo struct directly.
			 * @param hAstr The astronaut vessel handle.
			 * @param astrInfo The astronaut information. The astronaut class name must not be changed.
			 * @return True if hAstr is an astronaut, false if not.
			*/
			bool SetScnAstrInfoByHandle(OBJHANDLE hAstr, AstrInfo astrInfo);

			/**
			 * @brief Gets the available astronaut count, which is the count of astronauts that can be added.
			 * 
			 * It's the config file count in 'Config\Vessels\UACS\Astronauts'.
			 * @return The available cargo count.
			*/
			size_t GetAvailAstrCount();

			/**
			 * @brief Gets the name of an available astronaut.
			 * @param availIdx The available astronaut index. It must be less than GetAvailAstrCount.
			 * @return The available astronaut name.
			*/
			std::string_view GetAvailAstrName(size_t availIdx);

			/**
			 * @brief Adds an astronaut with the passed information to the passed station.
			 * @param availIdx The available astronaut index. It must be less than GetAvailAstrCount.
			 * @param stationIdx The station index. If nullopt is passed, the first empty station will be used.
			 * @param astrInfo The astronaut information. If nullopt is passed, the astronaut default information in its config file will be used.
			 * @return The addition result as the IngressResult enum.
			*/
			IngressResult AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx = {}, std::optional<AstrInfo> astrInfo = {});

			/**
			 * @brief Transfers the astronaut in the passed station to the vessel docked to the docking port associated with the passed airlock.
			 * @param stationIdx The astronaut station index.
			 * @param airlockIdx The airlock index. A vessel with UACS astronaut implementation should be docked to the docking port associated with the airlock.
			 * @param tgtStationIdx The target vessel station index to transfer the astronaut into it. If nullopt is passed, the first empty station will be used.
			 * @return The transfer result.
			*/
			TransferResult TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx = {});

			/**
			 * @brief Egresses the astronaut in the passed station via the passed airlock.
			 * 
			 * In space, the astronaut is egressed directly in front of the airlock. On the ground, the astronaut is egressed at the closest location to the airlock position.
			 * @param stationIdx The astronaut station index.
			 * @param airlockIdx The airlock index. 
			 * @return The egress result.
			*/
			EgressResult EgressAstronaut(size_t stationIdx, size_t airlockIdx);

			/**
			 * @brief Gets cargo count in the scenario.
			 * @return The cargo count in the scenario.
			*/
			size_t GetScnCargoCount();

			/**
			 * @brief Gets a cargo information by the cargo index.
			 * @param cargoIdx The cargo index. It must be less than GetScnCargoCount.
			 * @return The cargo information.
			*/
			CargoInfo GetCargoInfoByIndex(size_t cargoIdx);

			/**
			 * @brief Gets a cargo information by the cargo handle.
			 * @param hCargo The cargo vessel handle.
			 * @return The cargo information, or nullopt is hCargo is invalid or not a cargo.
			*/
			std::optional<CargoInfo> GetCargoInfoByHandle(OBJHANDLE hCargo);

			/**
			 * @brief Gets the mass of all grappled cargoes.
			 * @return The mass of all grappled cargoes.
			*/
			double GetTotalCargoMass();

			/**
			 * @brief Gets the available cargo count, which is the count of cargoes that can be added.
			 * 
			 * It's the config file count in 'Config\Vessels\UACS\Cargoes'.
			 * @return The available cargo count.
			*/
			size_t GetAvailCargoCount();

			/**
			 * @brief Gets the name of an available cargo.
			 * @param availIdx The available cargo index. It must be less than GetAvailCargoCount.
			 * @return The available cargo name.
			*/
			std::string_view GetAvailCargoName(size_t availIdx);

			/**
			 * @brief Adds the passed available cargo to the passed slot.
			 * @param availIdx The available cargo index. It must be less than GetAvailCargoCount.
			 * @param slotIdx The slot index. If nullopt is passed, the first empty slot will be used.
			 * @return The addition result as the GrappleResult enum.
			*/
			GrappleResult AddCargo(size_t availIdx, std::optional<size_t> slotIdx = {});

			/**
			 * @brief Deletes the cargo in the passed slot.
			 * @param slotIdx The slot index. If nullopt is passed, the first occupied slot will be used.
			 * @return The delete result as the ReleaseResult enum.
			*/
			ReleaseResult DeleteCargo(std::optional<size_t> slotIdx = {});

			/**
			 * @brief Grapples the passed cargo into the passed slot.
			 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest cargo in the grapple range will be used.
			 * @param slotIdx The slot index. If nullopt is passed, the first empty slot will be used.
			 * @return The grapple result.
			*/
			GrappleResult GrappleCargo(OBJHANDLE hCargo = nullptr, std::optional<size_t> slotIdx = {});

			/**
			 * @brief Releases the cargo in the passed slot.
			 * 
			 * If releasing in space, the cargo will be released at the slot position at the release velocity.
			 * On the ground, the cargo will be released at the closest location to the slot position.
			 * @param slotIdx The slot index. If nullopt is passed, the first occupied slot will be used.
			 * @return The release result. 
			*/
			ReleaseResult ReleaseCargo(std::optional<size_t> slotIdx = {});

			/**
			 * @brief Packs the passed cargo.
			 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest packable cargo in the packing range will be used.
			 * @return The packing result.
			*/
			PackResult PackCargo(OBJHANDLE hCargo = nullptr);

			/**
			 * @brief Unpacks the passed cargo.
			 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest unpackable cargo in the packing range will be used.
			 * @return The unpacking result as the PackResult enum.
			*/
			PackResult UnpackCargo(OBJHANDLE hCargo = nullptr);

			/**
			 * @brief Drains the passed resource from the cargo in the passed slot.
			 * @param resource The resource name. See UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param slotIdx The slot index. If nullopt is passed, the first suitable cargo will be used.
			 * @return A pair of the drainage result and drained mass. The drained mass will be 0 if no resource was drained.
			*/
			std::pair<DrainResult, double> DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx = {});

			/**
			 * @brief Drains the passed resource from the passed cargo.
			 * @param resource The resource name. See UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest suitable cargo in the drainage range will be used.
			 * @return A pair of the drainage result and drained mass. The drained mass will be 0 if no resource was drained.
			*/
			std::pair<DrainResult, double> DrainUngrappledResource(std::string_view resource, double mass, OBJHANDLE hCargo = nullptr);

			/**
			 * @brief Drains the passed resource from the passed station.
			 * @param resource The resource name. See UACS manual for the standard resource names.
			 * @param mass The requested resource mass in kilograms.
			 * @param hStation The station vessel handle. If nullptr is passed, the nearest suitable station in the drainage range will be used.
			 * @return A pair of the drainage result and drained mass. The drained mass will be 0 if no resource was drained.
			*/
			std::pair<DrainResult, double> DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation = nullptr);

		private:
			HINSTANCE coreDLL;
			Core::Vessel* pCoreVessel{};
		};
	};
}