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

		inline std::string_view GetUACSVersion() { return "1.0.0"; }

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

		inline size_t GetEmptyStationIndex(std::span<const UACS::StationInfo> stations)
		{
			for (size_t idx{}; idx < stations.size(); ++idx) if (!stations[idx].astrInfo) return idx;
			
			return 0;
		}
	}
}