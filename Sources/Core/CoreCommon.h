#pragma once
#include "CoreDefs.h"

#include <unordered_map>
#include <span>

namespace UACS
{
	namespace Core
	{
		inline std::vector<API::Astronaut*> astrVector;

		inline std::vector<API::Cargo*> cargoVector;

		inline std::unordered_map<OBJHANDLE, API::VslAstrInfo*> vslAstrMap;

		inline std::optional<size_t> GetEmptyStationIndex(std::span<const API::StationInfo> stations)
		{
			static auto predicate = [](const API::StationInfo& stationInfo) { return !stationInfo.astrInfo.has_value(); };

			const auto stationIt = std::ranges::find_if(stations, predicate);

			if (stationIt == stations.end()) return {};

			return stationIt - stations.begin();
		}
	}
}