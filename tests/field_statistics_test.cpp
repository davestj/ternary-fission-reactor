#include "http.ternary.fission.server.h"
#include <cassert>
#include <cmath>
#include <map>
#include <memory>

using namespace TernaryFission;

Json::Value computeFieldStatistics(const std::map<std::string, std::unique_ptr<EnergyFieldResponse>>& fields) {
    int total_fields = static_cast<int>(fields.size());
    int active_fields = 0;
    double total_energy = 0.0;
    double peak_energy = 0.0;

    for (const auto& [id, field] : fields) {
        total_energy += field->energy_level_mev;
        if (field->energy_level_mev > peak_energy) {
            peak_energy = field->energy_level_mev;
        }
        if (field->status == "active" || field->active) {
            active_fields++;
        }
    }

    int inactive_fields = total_fields - active_fields;
    double average_energy = total_fields > 0 ? total_energy / total_fields : 0.0;

    Json::Value stats;
    stats["total_fields"] = static_cast<Json::Int64>(total_fields);
    stats["active_fields"] = static_cast<Json::Int64>(active_fields);
    stats["inactive_fields"] = static_cast<Json::Int64>(inactive_fields);
    stats["total_energy_mev"] = total_energy;
    stats["average_energy_mev"] = average_energy;
    stats["peak_energy_mev"] = peak_energy;
    return stats;
}

int main() {
    std::map<std::string, std::unique_ptr<EnergyFieldResponse>> fields;

    auto active = std::make_unique<EnergyFieldResponse>();
    active->field_id = "A";
    active->energy_level_mev = 100.0;
    active->status = "active";
    active->active = true;
    active->total_energy_mev = 100.0;
    fields["A"] = std::move(active);

    auto inactive = std::make_unique<EnergyFieldResponse>();
    inactive->field_id = "B";
    inactive->energy_level_mev = 50.0;
    inactive->status = "inactive";
    inactive->active = false;
    inactive->total_energy_mev = 50.0;
    fields["B"] = std::move(inactive);

    Json::Value stats = computeFieldStatistics(fields);

    assert(stats["total_fields"].asInt() == 2);
    assert(stats["active_fields"].asInt() == 1);
    assert(stats["inactive_fields"].asInt() == 1);
    assert(std::abs(stats["total_energy_mev"].asDouble() - 150.0) < 1e-9);
    assert(std::abs(stats["average_energy_mev"].asDouble() - 75.0) < 1e-9);
    assert(std::abs(stats["peak_energy_mev"].asDouble() - 100.0) < 1e-9);
    return 0;
}
