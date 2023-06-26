#pragma once

#include <filesystem>

#include <boost/json.hpp>

#include "model.h"

#include "application_support.h"

namespace json_loader {

	model::Game LoadGame(const std::filesystem::path& json_path, app_support::FrontendExtraDataMap& extra);

}  // namespace json_loader
