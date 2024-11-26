#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

int LlmReduce::GetAvailableTokens() {
    int num_tokens_meta_and_reduce_query = 0;
    num_tokens_meta_and_reduce_query += Tiktoken::GetNumTokens(user_query);
    num_tokens_meta_and_reduce_query +=
        Tiktoken::GetNumTokens(PromptManager::GetTemplate(AggregateFunctionType::REDUCE));

    auto model_context_size = model.GetModelDetails().context_window;
    if (num_tokens_meta_and_reduce_query > model_context_size) {
        throw std::runtime_error("Fixed tokens exceed model context size");
    }

    auto available_tokens = model_context_size - num_tokens_meta_and_reduce_query;
    return available_tokens;
}

nlohmann::json LlmReduce::ReduceBatch(nlohmann::json& tuples) {
    nlohmann::json data;
    auto prompt = PromptManager::Render(user_query, tuples, AggregateFunctionType::REDUCE);
    auto response = model.CallComplete(prompt);
    return response["output"];
};

nlohmann::json LlmReduce::ReduceLoop(std::vector<nlohmann::json>& tuples) {
    auto available_tokens = GetAvailableTokens();
    auto accumulated_tuples_tokens = 0u;
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;

    do {
        accumulated_tuples_tokens = Tiktoken::GetNumTokens(batch_tuples.dump());
        accumulated_tuples_tokens +=
            Tiktoken::GetNumTokens(PromptManager::ConstructMarkdownHeader(tuples[start_index]));
        while (accumulated_tuples_tokens < available_tokens && start_index < tuples.size()) {
            auto num_tokens = Tiktoken::GetNumTokens(PromptManager::ConstructMarkdownSingleTuple(tuples[start_index]));
            if (accumulated_tuples_tokens + num_tokens > available_tokens) {
                break;
            }
            batch_tuples.push_back(tuples[start_index]);
            accumulated_tuples_tokens += num_tokens;
            start_index++;
        }
        auto response = ReduceBatch(batch_tuples);
        batch_tuples.clear();
        batch_tuples.push_back(response);
        accumulated_tuples_tokens = 0;
    } while (start_index < tuples.size());

    return batch_tuples[0];
}

void LlmReduce::Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
    auto states_vector = duckdb::FlatVector::GetData<AggregateFunctionState*>(states);

    auto function_instance = AggregateFunctionBase::GetInstance<LlmReduce>();
    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto state_ptr = states_vector[idx];
        auto state = function_instance->state_map[state_ptr];

        auto response = function_instance->ReduceLoop(state->value);
        result.SetValue(idx, response.dump());
    }
}

} // namespace flockmtl