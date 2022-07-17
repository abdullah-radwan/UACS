#pragma once
#include "Common.h"
#include <VesselAPI.h>

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
			 * On the other hand, API::CargoInfo is used to pass information from UACS to vessels.
			*/
			struct CargoInfo
			{
				/// The attachment handle used to grapple the cargo.
				ATTACHMENTHANDLE hAttach;

				CargoType type;

				/**
				 * @brief The cargo resource. If the cargo isn't a resource, it should be an empty string.
				 * @note Use the standard resource names specified in UACS manual for better compatibility with vessels.
				*/
				std::string resource{};

				/**
				 * @brief The cargo unpacking flag. True if unpacked, false if not.
				 * @note The cargo is responsible for setting the flag correctly (e.g. loading/saving it to scenario, or when clbkPackCargo or clbkUnpackCargo is called, etc.).
				*/
				bool unpacked{ false };

				/// The front position of the cargo. This is used to properly orient the cargo when released on sloped ground.
				VECTOR3 frontPos{};

				/// The right position of the cargo. This is used to properly orient the cargo when released on sloped ground.
				VECTOR3 rightPos{};

				/// The left position of the cargo. This is used to properly orient the cargo when released on sloped ground.
				VECTOR3 leftPos{};

				/**
				 * @brief The cargo breathablity flag. True if breathable, false if not.
				 * @note If the cargo is breathable, this flag should be true even if the cargo is packed. UACS automatically considers the cargo as unbreathable if it's packed.
				*/
				bool breathable{ false };
			};

			Cargo(OBJHANDLE hVessel, int fModel = 1);
			virtual ~Cargo();

			/**
			 * @brief Cargo information callback. Must be implemented by cargoes.
			 * @return A const pointer to the cargo information struct.
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
			 * @return The drained mass, or 0 if the drainge failed.
			*/
			virtual double clbkDrainResource(double mass);

		private:
			std::unique_ptr<CargoImpl> cargoImpl;
		};
	}
}