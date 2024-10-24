#include <functional>
#include <iostream>
#include <flockmtl/core/functions/prompt_builder.hpp>
#include <flockmtl/common.hpp>
#include <flockmtl/core/functions/scalar.hpp>
#include <flockmtl/core/model_manager/model_manager.hpp>
#include <flockmtl/core/model_manager/openai.hpp>
#include <flockmtl/core/model_manager/tiktoken.hpp>
#include <flockmtl/core/parser/llm_response.hpp>
#include <flockmtl/core/parser/scalar.hpp>
#include <flockmtl_extension.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <templates/llm_complete_json_prompt_template.hpp>

namespace flockmtl {
namespace core {

static void LlmCompleteJsonScalarFunction(DataChunk &args, ExpressionState &state, Vector &result) {
    Connection con(*state.GetContext().db);
    CoreScalarParsers::LlmCompleteJsonScalarParser(args);

    auto model = args.data[1].GetValue(0).ToString();
    auto query_result =
        con.Query("SELECT model, max_tokens FROM flockmtl_config.FLOCKMTL_MODEL_INTERNAL_TABLE WHERE model_name = '" +
                  model + "'");

    if (query_result->RowCount() == 0) {
        throw std::runtime_error("Model not found");
    }

    auto model_name = query_result->GetValue(0, 0).ToString();
    auto model_max_tokens = query_result->GetValue(1, 0).GetValue<int32_t>();

    if (args.ColumnCount() == 2) {
        auto query_result =
            con.Query("SELECT prompt FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE WHERE prompt_name = '" +
                      args.data[0].GetValue(0).ToString() + "'");

        if (query_result->RowCount() == 0) {
            throw std::runtime_error("Prompt not found");
        }

        auto template_str = query_result->GetValue(0, 0).ToString() + "\nThe Ouput should be in JSON format.";
        nlohmann::json settings;
        auto response = ModelManager::CallComplete(template_str, model_name, settings);

        result.SetValue(0, response.dump());
    } else {
        auto tuples = CoreScalarParsers::Struct2Json(args.data[2], args.size());

        auto prompts = ConstructPrompts(tuples, con, args.data[0].GetValue(0).ToString(),
                                        llm_complete_json_prompt_template, model_max_tokens);

        nlohmann::json settings;
        if (args.ColumnCount() == 4) {
            settings = CoreScalarParsers::Struct2Json(args.data[3], 1)[0];
        }

        auto responses = nlohmann::json::array();
        for (const auto &prompt : prompts) {
            // Call ModelManager::CallComplete and get the rows
            auto response = ModelManager::CallComplete(prompt, model_name, settings);

            // Check if the result contains the 'rows' field and push it to the main 'rows'
            if (response.contains("rows")) {
                for (const auto &row : response["rows"]) {
                    responses.push_back(row);
                }
            }
        }

        auto index = 0;
        for (const auto &response : responses) {
            result.SetValue(index++, Value(response.dump()));
        }
    }
}

void CoreScalarFunctions::RegisterLlmCompleteJsonScalarFunction(DatabaseInstance &db) {
    ExtensionUtil::RegisterFunction(db, ScalarFunction("llm_complete_json", {}, LogicalType::JSON(),
                                                       LlmCompleteJsonScalarFunction, nullptr, nullptr, nullptr,
                                                       nullptr, LogicalType::ANY));
}

} // namespace core
} // namespace flockmtl
