#pragma once
#include <nlohmann/json.hpp>
#include <flockmtl/common.hpp>
#include <flockmtl/core/model_manager/tiktoken.hpp>

namespace flockmtl {
namespace core {

nlohmann::json GetMaxLengthValues(const std::vector<nlohmann::json> &params);

std::string CombineValues(const nlohmann::json &json_obj);

std::vector<std::string> ConstructPrompts(std::vector<nlohmann::json> &unique_rows, Connection &con,
                                          std::string user_prompt_name, const std::string &llm_template,
                                          int model_max_tokens = 4096);

} // namespace core
} // namespace flockmtl