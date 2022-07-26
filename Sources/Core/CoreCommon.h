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

		inline std::string_view GetUACSVersion() { return "Alpha 4"; }

		inline size_t GetScnAstrCount() { return astrVector.size(); }

		inline std::pair<OBJHANDLE, const API::AstrInfo*> GetAstrInfoByIndex(size_t astrIdx)
		{
			API::Astronaut* pAstr = astrVector.at(astrIdx);
			return { pAstr->GetHandle(), pAstr->clbkGetAstrInfo() };
		}

		inline const API::AstrInfo* GetAstrInfoByHandle(OBJHANDLE hAstr)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hAstr](API::Astronaut* pAstr) { return pAstr->GetHandle() == hAstr; });

			return astrIt == astrVector.end() ? nullptr : (*astrIt)->clbkGetAstrInfo();
		}

		inline const API::VslAstrInfo* GetVslAstrInfo(OBJHANDLE hVessel)
		{
			const auto& vslPair = vslAstrMap.find(hVessel);

			return vslPair == vslAstrMap.end() ? nullptr : vslPair->second;
		}

		inline void SetScnAstrInfoByIndex(size_t astrIdx, API::AstrInfo astrInfo) { astrVector.at(astrIdx)->clbkSetAstrInfo(astrInfo); }

		inline bool SetScnAstrInfoByHandle(OBJHANDLE hAstr, API::AstrInfo astrInfo)
		{
			auto astrIt = std::ranges::find_if(astrVector, [hAstr](API::Astronaut* pAstr) { return pAstr->GetHandle() == hAstr; });

			if (astrIt == astrVector.end()) return false;

			(*astrIt)->clbkSetAstrInfo(astrInfo);

			return true;
		}

		inline std::optional<size_t> GetEmptyStationIndex(std::span<const API::StationInfo> stations)
		{
			static auto predicate = [](const API::StationInfo& stationInfo) { return !stationInfo.astrInfo.has_value(); };

			const auto stationIt = std::ranges::find_if(stations, predicate);

			if (stationIt == stations.end()) return {};

			return stationIt - stations.begin();
		}
	}
}