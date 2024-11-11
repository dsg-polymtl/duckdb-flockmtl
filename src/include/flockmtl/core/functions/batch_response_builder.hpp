#pragma once
#include <nlohmann/json.hpp>
#include <flockmtl/common.hpp>
#include <flockmtl/core/model_manager/model_manager.hpp>
#include <templates/llm_scalar_prompt_templates.hpp>

namespace flockmtl {
namespace core {

std::vector<nlohmann::json> CastVectorOfStructsToJson(Vector &struct_vector, int size);

struct PromptDetails {
    std::string prompt_name;
    std::string prompt;
};

std::string ConstructMarkdownHeader(const nlohmann::json &tuple);

std::string ConstructMarkdownSingleTuple(const nlohmann::json &tuple);

std::string ConstructMarkdownArrayTuples(const nlohmann::json &tuples);

PromptDetails CreatePromptDetails(Connection &con, const nlohmann::json prompt_details_json);

nlohmann::json Complete(const std::string &tuples, const std::string &user_prompt, const std::string &llm_template,
                        const ModelDetails &model_details);

nlohmann::json BatchAndComplete(std::vector<nlohmann::json> &tuples, Connection &con, std::string user_prompt_name,
                                ScalarPromptType prompt_type, const ModelDetails &model_details);

} // namespace core
} // namespace flockmtl