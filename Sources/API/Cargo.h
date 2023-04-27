#pragma once
#include "Common.h"
#include <VesselAPI.h>

/**
 * @file
 * @brief UACS cargo API header.
*/

namespace UACS
{
	namespace API
	{
		class CargoImpl;

		class Cargo : public VESSEL4
		{
		public:
			/**
			 * @brief The cargo information struct. This is the information the cargo must pass to UACS.
			 * On the other hand, API::CargoInfo is used to pass cargo information from UACS to vessels.
			*/
			struct CargoInfo
			{
				/// The attachment handle used to grapple the cargo.
				ATTACHMENTHANDLE hAttach;

				CargoType type;
				
				/// If the cargo is unpackable only, and cannot be packed again. Applicable only if type is UNPACKABLE.
				bool unpackOnly{ false };

				/**
				 * @brief The cargo unpacking flag.
				 * @note The cargo is responsible for setting the flag correctly
				 * (e.g. loading/saving it to scenario, or when clbkPackCargo or clbkUnpackCargo is called, etc.).
				*/
				bool unpacked{ false };

				/**
				 * @brief The cargo breathablity flag.
				 * @note If the cargo is breathable, this flag should be true even if the cargo is packed.
				 * UACS automatically considers the cargo unbreathable if it's packed.
				*/
				bool breathable{ false };

				/**
				 * @brief The cargo front end position in vessel-relative coordinates.
				 * 
				 * If rightPos and leftPos are passed, all three values are used to properly orienate the cargo on sloped ground.
				 * Otherwise, at least frontPos Y value must be set to the cargo height, which is used when releasing cargo on ground.
				*/
				VECTOR3 frontPos{};

				std::optional<VECTOR3> rightPos{};

				std::optional<VECTOR3> leftPos{};

				/**
				 * @brief The cargo resource. If the cargo isn't a resource, it should be a nullopt.
				 * @note Use the standard resource names specified in UACS manual for better compatibility with vessels.
				*/
				std::optional<std::string> resource{};
			};

			Cargo(OBJHANDLE hVessel, int fModel = 1);
			virtual ~Cargo();

			/**
			 * @brief The cargo information callback. It must be implemented by cargoes.
			 * @return A const pointer to the CargoInfo struct. The struct must live until the cargo is destroyed.
			 * Don't pass the address of a temporary variable.
			*/
			virtual const CargoInfo* clbkGetCargoInfo() = 0;

			/// Optional callback: Called when the cargo is grappled.
			virtual void clbkCargoGrappled();

			/// Optional callback: Called when the cargo is released.
			virtual void clbkCargoReleased();

			/**
			 * @brief For packable cargoes only: Called when the cargo is packed. Implement the packing logic here.
			 * @return True if the packing is successful, false if not.
			*/
			virtual bool clbkPackCargo();

			/**
			 * @brief For unpackable cargoes only: Called when the cargo is unpacked. Implement the unpacking logic here.
			 * @return True if the unpacking is successful, false if not.
			*/
			virtual bool clbkUnpackCargo();

			/**
			 * @brief For resource cargoes only: Called when the cargo resource is drained. Implement the drainage logic here.
			 * @return The drained mass, or 0 if the drainage failed.
			*/
			virtual double clbkDrainResource(double mass);

		private:
			std::unique_ptr<CargoImpl> pCargoImpl;
		};
	}
}