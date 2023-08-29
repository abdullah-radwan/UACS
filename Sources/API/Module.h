#pragma once
#include "Common.h"

/**
 * @file
 * @brief UACS module API header.
*/

namespace UACS
{
	namespace Core { class Module; }

	struct CargoInfo
	{
		OBJHANDLE handle;

		/// If the cargo is attached to another vessel.
		bool attached;

		CargoType type;

		/// If the cargo is unpackable only and can't be packed again. Applicable only if type is UNPACKABLE.
		bool unpackOnly;

		bool unpacked;

		/**
		 * @brief While the cargo is breathable only if it's unpacked, this flag is true even if the cargo is packed.
		 *
		 * The cargo is considered breathable in its current state if both unpacked and breathable flags are true.
		*/
		bool breathable;

		std::optional<std::string> resource;
	};

	struct SlotInfo
	{
		ATTACHMENTHANDLE hAttach;

		/**
		 * @brief The slot cargo holding direction. It's used to position the grappled cargo properly (see UACS developer manual).
		 *
		 * If the cargo is held from below, the Y value is -1. From above, Y value is 1.
		 * From right, X value is 1. From left, X value is -1. From front, Z value is 1. From rear, Z value is -1.
		 * All other values is set to 0.
		*/
		VECTOR3 holdDir;

		bool open{ true };

		/// The cargo release velocity (if released in space) in meters per second.
		double relVel{ 0 };

		/// The slot ground release information. If astronaut mode is on, it has no effect.
		GroundInfo gndInfo{};

		std::optional<CargoInfo> cargoInfo{};
	};

	struct VslCargoInfo
	{
		std::vector<SlotInfo> slots;

		/**
		 * @brief The astronaut mode flag.
		 *
		 * If on:
		 * 1. Unpacked cargoes can be grappled.
		 * 2. On ground, cargoes are released in a single position (slot attachment point position or ground information position), not in a table.
		 * All of the slot ground information except position is ignored. If a cargo exists in that position, the grappled cargo can't be released.
		 * 3. On ground, cargo heading once released is the same as its heading when grappled. If astronaut mode is off, the heading is set to the vessel heading.
		*/
		bool astrMode{ false };

		/// The cargo grapple range in meters. It's the max distance between the slot attachment point position and the cargo attachment point position.
		double grappleRange{ 50 };

		/// The search range for packable and unpackable cargoes in meters. It's the max distance between the vessel radius (size) and the cargo radius.
		double packRange{ 50 };

		/// The search range for draining resources in meters. It's the max distance between the vessel radius (size) and the cargo/station radius.
		double drainRange{ 100 };

		/// The maximum single cargo mass in kilograms.
		std::optional<double> maxCargoMass;

		/// The maximum total cargo mass in kilograms.
		std::optional<double> maxTotalCargoMass;
	};

	enum TransferResult
	{
		TRNS_SUCCED,
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
		EGRS_SUCCED,
		EGRS_STN_EMPTY,
		EGRS_ARLCK_CLSD,

		/// A vessel is docked to the passed airlock docking port.
		EGRS_ARLCK_DCKD,

		/// No empty position to egress the astronaut on the ground.
		EGRS_NO_EMPTY_POS,

		/// The astronaut was egressed but its information couldn't be set.
		EGRS_INFO_NOT_SET,

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

		/// The passed cargo can't be grappled because it is unpacked. Not applicable if grappleUnpacked is true.
		GRPL_CRG_UNPCKD,

		/// The passed cargo is attached to another vessel.
		GRPL_CRG_ATCHD,

		GRPL_MASS_EXCD,
		GRPL_TTL_MASS_EXCD,
		GRPL_FAIL
	};

	enum ReleaseResult
	{
		RLES_SUCCED,

		/// The passed slot (or all slots if slotIdx is nullopt) is closed.
		RLES_SLT_CLSD,

		/// The passed slot (or all slots if slotIdx is nullopt) is empty.
		RLES_SLT_EMPTY,

		/// No empty position to release the cargo on the ground.
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

		/// The passed vessel (or all resource cargoes or stations if hCargo or hStation is nullptr) is outside the drainage range. Not returned by DrainGrappledResource method.
		DRIN_NOT_IN_RNG,

		/// The passed cargo or station (or all resource cargoes or stations if hCargo or hStation is nullptr) isn't a resource.
		DRIN_VSL_NOT_RES,

		/// The passed cargo or station doesn't contain the passed resource.
		DRIN_RES_NOT_FND,

		/// The passed cargo is attached to another vessel. Returned only by DrainScenarioResource.
		DRIN_RES_ATCHD,

		DRIN_FAIL
	};

	/// UACS module API.
	class Module
	{
	public:
		/**
		 * @brief Creates an instance from the module API. It can be called anywhere.
		 * @param pVessel A pointer to the vessel class. If nullptr is passed, astronaut station and airlock and cargo slot specific methods can't be called.
		 * pVslAstrInfo and pVslCargoInfo must be set nullptr as well.
		 * @param pVslAstrInfo A pointer to the vessel astronaut information. If nullptr is passed, astronaut station and airlock specific methods can't be called.
		 * @param pVslCargoInfo A pointer to vessel cargo information. If nullptr is passed, cargo slot specific methods can't be called.
		 * @note All pointers must live until the module API instance is destroyed. Don't pass the address of temporary variables.
		 * @return An instance of the module API even if UACS isn't installed. To find out whether UACS is installed or not, use GetUACSVersion.
		*/
		Module(VESSEL* pVessel, VslAstrInfo* pVslAstrInfo, VslCargoInfo* pVslCargoInfo);
		~Module();

		/**
		 * @brief Gets UACS version. It can be used to find out if UACS is installed.
		 * @return UACS version if UACS is installed, or an empty string view if not.
		*/
		std::string_view GetUACSVersion();

		// The 3 methods below must be called if pVessel is defined.

		/**
		 * @brief Parses the scenario file to load UACS information. It must be called in the vessel clbkLoadStateEx method.
		 * @param line The scenario line from the vessel clbkLoadStateEx method.
		 * @return True if UACS information was loaded, false if not.
		*/
		bool ParseScenarioLine(char* line);

		/**
		 * @brief Finishes UACS initialization.
		 * It must be called once from the vessel clbkPostCreation method, after defining all airlocks, stations, and slots.
		*/
		void clbkPostCreation();

		/**
		 * @brief Saves UACS information to the scenario file. It must be called in the vessel clbkSaveState method.
		 * @param scn The scenario file.
		*/
		void clbkSaveState(FILEHANDLE scn);

		// Astronaut methods.

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
		 * @brief Sets an astronaut information by the astronaut index.
		 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
		 * @param astrInfo The astronaut information. Avoid making an instance of the AstrInfo struct. Rather, use GetAstrInfoByIndex and change the information as required.
		 * @return True if the astronaut information is set, false if not.
		*/
		bool SetAstrInfoByIndex(size_t astrIdx, const AstrInfo& astrInfo);

		/**
		 * @brief Sets an astronaut information by the astronaut handle.
		 * @param astrIdx The astronaut index. It must be less than GetScnAstrCount.
		 * @param astrInfo The astronaut information. Avoid making an instance of the AstrInfo struct. Rather, use GetAstrInfoByHandle and change the information as required.
		 * @return True if the astronaut information is set, false if not.
		*/
		bool SetAstrInfoByHandle(OBJHANDLE hAstr, const AstrInfo& astrInfo);

		/**
		 * @brief Gets the available astronaut count.
		 *
		 * It's the count of astronauts that can be added to the scenario, which is the config file count in 'Config\Vessels\UACS\Astronauts'.
		 * @return The available cargo count.
		*/
		size_t GetAvailAstrCount();

		/**
		 * @brief Gets the name of an available astronaut, which is the filename of the astronaut config file.
		 * @param availIdx The available astronaut index. It must be less than GetAvailAstrCount.
		 * @return The available astronaut name.
		*/
		std::string_view GetAvailAstrName(size_t availIdx);

		// Astronaut methods below can only be used only if pVessel and pVslAstrInfo are defined.

		/**
		 * @brief Gets the mass of onboard astronauts.
		 * @return The mass of onboard astronauts.
		*/
		double GetTotalAstrMass();

		/**
		 * @brief Adds an astronaut with the passed information to the passed station.
		 * @param availIdx The available astronaut index. It must be less than GetAvailAstrCount.
		 * @param stationIdx The station index. If nullopt is passed, the first empty station is used.
		 * @param astrInfo The astronaut information. The class name is set by UACS. If nullopt is passed, the astronaut default information is used.
		 * @return The addition result as the IngressResult enum.
		*/
		IngressResult AddAstronaut(size_t availIdx, std::optional<size_t> stationIdx = {}, std::optional<AstrInfo> astrInfo = {});

		/**
		 * @brief Transfers the astronaut in the passed station to the vessel docked to the docking port associated with the passed airlock.
		 * @param stationIdx The astronaut station index.
		 * @param airlockIdx The airlock index. A vessel with UACS astronaut implementation must be docked to the docking port associated with the airlock.
		 * @param tgtStationIdx The target vessel station index to transfer the astronaut into it. If nullopt is passed, the first empty station is used.
		 * @return The transfer result.
		*/
		TransferResult TransferAstronaut(size_t stationIdx, size_t airlockIdx, std::optional<size_t> tgtStationIdx = {});

		/**
		 * @brief Egresses the astronaut in the passed station via the passed airlock.
		 * @param stationIdx The astronaut station index.
		 * @param airlockIdx The airlock index.
		 * @return The egress result.
		*/
		EgressResult EgressAstronaut(size_t stationIdx, size_t airlockIdx);

		// Cargo methods.

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
		 * @brief Gets a station resources by the station handle. The method is expensive, so the result should be cached.
		 * @param hStation The station vessel handle.
		 * @return The station resources, or nullopt is hCargo is invalid or not a cargo. The vector is empty if the station supports all resources.
		*/
		std::optional<std::vector<std::string>> GetStationResources(OBJHANDLE hStation);

		/**
		 * @brief Gets the available cargo count.
		 *
		 * It's the count of cargoes that can be added to the scenario, which is the config file count in 'Config\Vessels\UACS\Cargoes'.
		 * @return The available cargo count.
		*/
		size_t GetAvailCargoCount();

		/**
		 * @brief Gets the name of an available cargo, which is the filename of the cargo config file.
		 * @param availIdx The available cargo index. It must be less than GetAvailCargoCount.
		 * @return The available cargo name.
		*/
		std::string_view GetAvailCargoName(size_t availIdx);

		// Cargo methods below can only be used only if pVessel and pVslCargoInfo are defined.

		/**
		 * @brief Gets the mass of all grappled cargoes.
		 * @return The mass of all grappled cargoes.
		*/
		double GetTotalCargoMass();

		/**
		 * @brief Adds the passed available cargo to the passed slot.
		 * @param availIdx The available cargo index. It must be less than GetAvailCargoCount.
		 * @param slotIdx The slot index. If nullopt is passed, the first empty slot is used.
		 * @return The addition result as the GrappleResult enum.
		*/
		GrappleResult AddCargo(size_t availIdx, std::optional<size_t> slotIdx = {});

		/**
		 * @brief Deletes the cargo in the passed slot.
		 * @param slotIdx The slot index. If nullopt is passed, the first occupied slot is used.
		 * @return The delete result as the ReleaseResult enum.
		*/
		ReleaseResult DeleteCargo(std::optional<size_t> slotIdx = {});

		/**
		 * @brief Grapples the passed cargo into the passed slot.
		 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest cargo in the grapple range is used.
		 * @param slotIdx The slot index. If nullopt is passed, the first empty slot is used.
		 * @return The grapple result.
		*/
		GrappleResult GrappleCargo(OBJHANDLE hCargo = nullptr, std::optional<size_t> slotIdx = {});

		/**
		 * @brief Releases the cargo in the passed slot.
		 * @param slotIdx The slot index. If nullopt is passed, the first occupied slot is used.
		 * @return The release result.
		*/
		ReleaseResult ReleaseCargo(std::optional<size_t> slotIdx = {});

		/**
		 * @brief Packs the passed cargo.
		 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest packable cargo in the packing range is used.
		 * @return The packing result.
		*/
		PackResult PackCargo(OBJHANDLE hCargo = nullptr);

		/**
		 * @brief Unpacks the passed cargo.
		 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest unpackable cargo in the packing range is used.
		 * @return The unpacking result as the PackResult enum.
		*/
		PackResult UnpackCargo(OBJHANDLE hCargo = nullptr);

		/**
		 * @brief Drains the passed resource from the cargo in the passed slot.
		 * @param resource The resource name. Use standard resource names (see UACS developer manual).
		 * @param mass The requested resource mass in kilograms.
		 * @param slotIdx The slot index. If nullopt is passed, the first suitable cargo is used.
		 * @return A pair of the drainage result and drained mass. The drained mass is 0 if no resource was drained.
		*/
		std::pair<DrainResult, double> DrainGrappledResource(std::string_view resource, double mass, std::optional<size_t> slotIdx = {});

		/**
		 * @brief Drains the passed resource from the passed cargo.
		 * @param resource The resource name. Use standard resource names (see UACS developer manual).
		 * @param mass The requested resource mass in kilograms.
		 * @param hCargo The cargo vessel handle. If nullptr is passed, the nearest suitable cargo in the drainage range is used.
		 * @return A pair of the drainage result and drained mass. The drained mass is 0 if no resource was drained.
		*/
		std::pair<DrainResult, double> DrainScenarioResource(std::string_view resource, double mass, OBJHANDLE hCargo = nullptr);

		/**
		 * @brief Drains the passed resource from the passed station.
		 * @param resource The resource name. Use standard resource names (see UACS developer manual).
		 * @param mass The requested resource mass in kilograms.
		 * @param hStation The station vessel handle. If nullptr is passed, the nearest suitable station in the drainage range is used.
		 * @return A pair of the drainage result and drained mass. The drained mass is 0 if no resource was drained.
		*/
		std::pair<DrainResult, double> DrainStationResource(std::string_view resource, double mass, OBJHANDLE hStation = nullptr);

	private:
		HINSTANCE coreDLL;
		Core::Module* pCoreModule{};
	};
}