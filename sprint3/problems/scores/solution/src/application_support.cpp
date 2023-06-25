#include "application_support.h"



namespace app_support	{

	std::optional<std::shared_ptr<loottypes_for_maps>> FrontendExtraDataMap::GetExtraData(std::string Id)
	{
		try{
			return extra_data_.at(Id);
		}
		catch(...)
		{
			return std::nullopt;
		}
	}

	void FrontendExtraDataMap::AddExtraData(std::string Id, std::shared_ptr<loottypes_for_maps> local_loot)
	{
		extra_data_.emplace(Id, local_loot);
	}


}