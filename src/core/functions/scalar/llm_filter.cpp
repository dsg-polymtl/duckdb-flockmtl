#include <functional>
#include <iostream>
#include <flockmtl/core/functions/prompt_builder.hpp>
#include <flockmtl/common.hpp>
#include <flockmtl/core/functions/scalar.hpp>
#include <flockmtl/core/model_manager/model_manager.hpp>
#include <flockmtl/core/model_manager/tiktoken.hpp>
#include <flockmtl/core/parser/llm_response.hpp>
#include <flockmtl/core/parser/scalar.hpp>
#include <flockmtl_extension.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <templates/llm_filter_prompt_template.hpp>

namespace flockmtl {
namespace core {

static void LlmFilterScalarFunction(DataChunk &args, ExpressionState &state, Vector &result) {
    Connection con(*state.GetContext().db);
    CoreScalarParsers::LlmFilterScalarParser(args);

    auto model = args.data[1].GetValue(0).ToString();
    auto query_result =
        con.Query("SELECT model, max_tokens FROM flockmtl_config.FLOCKMTL_MODEL_INTERNAL_TABLE WHERE model_name = '" +
                  model + "'");

    if (query_result->RowCount() == 0) {
        throw std::runtime_error("Model not found");
    }

    auto model_name = query_result->GetValue(0, 0).ToString();
    auto model_max_tokens = query_result->GetValue(1, 0).GetValue<int32_t>();

    auto tuples = CoreScalarParsers::Struct2Json(args.data[2], args.size());

    auto prompts = ConstructPrompts(tuples, con, args.data[0].GetValue(0).ToString(), llm_filter_prompt_template,
                                    model_max_tokens);

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
    Vector vec(LogicalType::VARCHAR, args.size());
    UnaryExecutor::Execute<string_t, string_t>(vec, result, args.size(), [&](string_t _) {
        return StringVector::AddString(result, responses[index++].dump());
    });
}

void CoreScalarFunctions::RegisterLlmFilterScalarFunction(DatabaseInstance &db) {
    ExtensionUtil::RegisterFunction(db, ScalarFunction("llm_filter", {}, LogicalType::VARCHAR, LlmFilterScalarFunction,
                                                       nullptr, nullptr, nullptr, nullptr, LogicalType::ANY));
}

} // namespace core
} // namespace flockmtl
