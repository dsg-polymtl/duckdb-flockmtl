#include <large_flock/common.hpp>
#include <large_flock/core/functions/scalar.hpp>
#include <large_flock/core/parser/scalar.hpp>
#include <large_flock_extension.hpp>

namespace large_flock {
namespace core {

void CoreScalarParsers::LfEmbeddingScalarParser(DataChunk &args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 3) {
        throw std::runtime_error("LfEmbedScalarParser: Invalid number of arguments.");
    }
    if (args.data[0].GetType().id() != LogicalTypeId::STRUCT) {
        throw std::runtime_error("LfEmbedScalarParser: Inputs must be a struct.");
    }
    if (args.data[1].GetType() != LogicalType::VARCHAR) {
        throw std::runtime_error("LfEmbedScalarParser: Model must be a string.");
    }
    if (args.ColumnCount() == 3) {
        if (args.data[2].GetType().id() != LogicalTypeId::STRUCT) {
            throw std::runtime_error("LfEmbedScalarParser: Settings value must be a struct.");
        }
    }
}

} // namespace core
} // namespace large_flock