#pragma once
#include "Defs.h"

#include <unordered_map>
#include <span>

namespace UACS
{
	namespace Core
	{
		inline std::vector<UACS::Astronaut*> astrVector;

		inline std::vector<UACS::Cargo*> cargoVector;

		inline std::unordered_map<OBJHANDLE, UACS::VslAstrInfo*> vslAstrMap;

		inline std::string_view GetUACSVersion() { return "v1.0 Pre-release 3"; }

		inline std::pair<OBJHANDLE, const UACS::AstrInfo*> GetAstrInfoByIndex(size_t astrIdx)
		{
			UACS::Astronaut* pAstr = astrVector.at(astrIdx);
			return { pAstr->GetHandle(), pAstr->clbkGetAstrInfo() };
		}

		inline const UACS::AstrInfo* GetAstrInfoByHandle(OBJHANDLE hAstr)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hAstr](UACS::Astronaut* pAstr) { return pAstr->GetHandle() == hAstr; });

			return (astrIt == astrVector.end()) ? nullptr : (*astrIt)->clbkGetAstrInfo();
		}

		inline const UACS::VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel)
		{
			const auto& vslPair = vslAstrMap.find(hVessel);

			return (vslPair == vslAstrMap.end()) ? nullptr : vslPair->second;
		}

		inline std::optional<size_t> GetEmptyStationIndex(std::span<const UACS::StationInfo> stations)
		{
			static auto predicate = [](const UACS::StationInfo& stationInfo) { return !stationInfo.astrInfo.has_value(); };

			const auto stationIt = std::ranges::find_if(stations, predicate);

			if (stationIt == stations.end()) return {};

			return std::distance(stations.begin(), stationIt);
		}
	}
}