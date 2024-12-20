#include "flockmtl/prompt_manager/repository.hpp"
#include "flockmtl/prompt_manager/prompt_manager.hpp"

namespace flockmtl {

template <>
std::string INSTRUCTIONS::Get(ScalarFunctionType option) {
    return INSTRUCTIONS::SCALAR_FUNCTION;
};

template <>
std::string INSTRUCTIONS::Get(AggregateFunctionType option) {
    return INSTRUCTIONS::AGGREGATE_FUNCTION;
};

template <>
std::string RESPONSE_FORMAT::Get(const ScalarFunctionType option) {
    switch (option) {
    case ScalarFunctionType::COMPLETE_JSON:
        return RESPONSE_FORMAT::COMPLETE_JSON;
    case ScalarFunctionType::COMPLETE:
        return RESPONSE_FORMAT::COMPLETE;
    case ScalarFunctionType::FILTER:
        return RESPONSE_FORMAT::FILTER;
    default:
        return "";
    }
}

template <>
std::string RESPONSE_FORMAT::Get(const AggregateFunctionType option) {
    switch (option) {
    case AggregateFunctionType::REDUCE:
        return RESPONSE_FORMAT::REDUCE;
    case AggregateFunctionType::REDUCE_JSON:
        return RESPONSE_FORMAT::REDUCE_JSON;
    case AggregateFunctionType::RERANK:
        return RESPONSE_FORMAT::RERANK;
    case AggregateFunctionType::FIRST:
    case AggregateFunctionType::LAST: {
        return PromptManager::ReplaceSection(RESPONSE_FORMAT::FIRST_OR_LAST, "{{RELEVANCE}}",
                                             option == AggregateFunctionType::FIRST ? "most" : "least");
    }
    default:
        return "";
    }
}

} // namespace flockmtl
