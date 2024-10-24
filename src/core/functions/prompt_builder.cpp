#include <inja/inja.hpp>
#include <flockmtl/core/functions/prompt_builder.hpp>

namespace flockmtl {
namespace core {

nlohmann::json GetMaxLengthValues(const std::vector<nlohmann::json> &params) {
    nlohmann::json attr_to_max_token_length;

    for (const auto &json_obj : params) {
        for (const auto &item : json_obj.items()) {
            auto key = item.key();
            auto value_str = static_cast<string>(item.value());
            int length = value_str.length();

            if (attr_to_max_token_length.contains(key)) {
                auto current_max_value_str = static_cast<string>(attr_to_max_token_length[key]);
                if (current_max_value_str.length() < length) {
                    attr_to_max_token_length[key] = item.value();
                }
            } else {
                attr_to_max_token_length[key] = item.value();
            }
        }
    }

    return attr_to_max_token_length;
}

std::string CombineValues(const nlohmann::json &json_obj) {
    std::string combined;
    for (const auto &item : json_obj.items()) {
        combined += static_cast<string>(item.value()) + " ";
    }

    if (!combined.empty()) {
        combined.pop_back();
    }
    return combined;
}

std::vector<std::string> ConstructPrompts(std::vector<nlohmann::json> &unique_rows, Connection &con,
                                          std::string user_prompt_name, const std::string &llm_template,
                                          int model_max_tokens) {
    inja::Environment env;

    auto query_result =
        con.Query("SELECT prompt FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE WHERE prompt_name = '" +
                  user_prompt_name + "'");

    if (query_result->RowCount() == 0) {
        throw std::runtime_error("Prompt not found");
    }

    auto user_prompt = query_result->GetValue(0, 0).ToString();
    int row_tokens = 0;
    row_tokens += Tiktoken::GetNumTokens(user_prompt);
    auto max_length_values = GetMaxLengthValues(unique_rows);
    auto combined_values = CombineValues(max_length_values);
    row_tokens += Tiktoken::GetNumTokens(combined_values);

    std::vector<std::string> prompts;

    if (row_tokens > model_max_tokens) {
        throw std::runtime_error("The total number of tokens in the prompt exceeds the model's maximum token limit");
    } else {
        auto template_tokens = Tiktoken::GetNumTokens(llm_template);
        auto max_tokens_for_rows = model_max_tokens - template_tokens;
        auto max_chunk_size = max_tokens_for_rows / row_tokens;
        auto chunk_size = max_chunk_size > static_cast<int>(unique_rows.size()) ? static_cast<int>(unique_rows.size())
                                                                                : max_chunk_size;
        auto num_chunks = static_cast<int>(std::ceil(static_cast<double>(unique_rows.size()) / chunk_size));

        for (int i = 0; i < num_chunks; ++i) {
            nlohmann::json data;
            data["prompts"] = user_prompt;

            for (int j = 0; j < chunk_size; ++j) {
                data["rows"].push_back(unique_rows[i + j]);
            }

            auto prompt = env.render(llm_template, data);
            prompts.push_back(prompt);
        }
    }

    return prompts;
}

} // namespace core
} // namespace flockmtl