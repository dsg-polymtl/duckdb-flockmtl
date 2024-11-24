#include "flockmtl/model_manager/model.hpp"

namespace flockmtl {

Model::Model(const nlohmann::json &model_json) {
    // TODO: remove this when refactoring the aggregate functions
    if (model_json.empty()) {
        return;
    }
    LoadModelDetails(model_json);
    ConstructProvider();
}

void Model::LoadModelDetails(const nlohmann::json &model_json) {
    model_details_.model_name = model_json.contains("model_name") ? model_json.at("model_name").get<std::string>() : "";
    if (model_details_.model_name.empty()) {
        throw std::invalid_argument("`model_name` is required in model settings");
    }
    auto query_result = GetQueriedModel(model_details_.model_name);
    model_details_.model =
        model_json.contains("model") ? model_json.at("model").get<std::string>() : std::get<0>(query_result);
    model_details_.provider_name =
        model_json.contains("provider") ? model_json.at("provider").get<std::string>() : std::get<1>(query_result);
    model_details_.secret = LoadSecret(model_details_.provider_name);
    model_details_.context_window =
        model_json.contains("context_window") ? model_json.at("context_window").get<int>() : std::get<2>(query_result);
    model_details_.max_output_tokens = model_json.contains("max_output_tokens")
                                           ? model_json.at("max_output_tokens").get<int>()
                                           : std::get<3>(query_result);
    model_details_.temperature = model_json.contains("temperature") ? model_json.at("temperature").get<float>() : 0.5;
}

std::string Model::LoadSecret(const std::string &provider_name) {
    auto query = "SELECT secret FROM "
                 "flockmtl_config.FLOCKMTL_SECRET_INTERNAL_TABLE "
                 "WHERE provider = '" +
                 provider_name + "'";

    auto query_result = core::CoreModule::GetConnection().Query(query);

    if (query_result->RowCount() == 0) {
        return "";
    }

    return query_result->GetValue(0, 0).ToString();
}

std::tuple<std::string, std::string, int32_t, int32_t> Model::GetQueriedModel(const std::string &model_name) {
    std::string query = "SELECT model, provider_name, model_args FROM "
                        "flockmtl_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                        "WHERE model_name = '" +
                        model_name + "'";

    auto con = core::CoreModule::GetConnection();
    auto query_result = con.Query(query);

    if (query_result->RowCount() == 0) {
        query_result = con.Query("SELECT model, provider_name, model_args FROM "
                                 "flockmtl_config.FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE WHERE model_name = '" +
                                 model_name + "'");

        if (query_result->RowCount() == 0) {
            throw std::runtime_error("Model not found");
        }
    }

    auto model = query_result->GetValue(0, 0).ToString();
    auto provider_name = query_result->GetValue(1, 0).ToString();
    auto model_args = nlohmann::json::parse(query_result->GetValue(2, 0).ToString());

    return {model, provider_name, model_args["context_window"], model_args["max_output_tokens"]};
}

void Model::ConstructProvider() {
    auto provider = GetProviderType(model_details_.provider_name);

    switch (provider) {
    case FLOCKMTL_OPENAI:
        provider_ = std::make_shared<OpenAIProvider>(model_details_);
        break;
    case FLOCKMTL_AZURE:
        provider_ = std::make_shared<AzureProvider>(model_details_);
        break;
    case FLOCKMTL_OLLAMA:
        provider_ = std::make_shared<OllamaProvider>(model_details_);
        break;
    default:
        throw std::invalid_argument("Unsupported provider: " + model_details_.provider_name);
    }
}

ModelDetails Model::GetModelDetails() { return model_details_; }

nlohmann::json Model::CallComplete(const std::string &prompt, bool json_response) {
    return provider_->CallComplete(prompt, json_response);
}

nlohmann::json Model::CallEmbedding(const std::vector<std::string> &inputs) { return provider_->CallEmbedding(inputs); }

} // namespace flockmtl