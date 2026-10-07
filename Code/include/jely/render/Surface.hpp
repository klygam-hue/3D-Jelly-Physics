#pragma once
#include "jely/physics/SoftBody.hpp"
#include <cstdint>

namespace jely {
class Surface {
public:
    explicit Surface(int subdivisions=24);
    void update(const SoftBody& body,double alpha);
    const std::vector<Vec3>& positions() const { return positions_; }
    const std::vector<Vec3>& normals() const { return normals_; }
    const std::vector<unsigned short>& indices() const { return indices_; }
    const std::vector<Vec3>& coordinates() const { return coordinates_; }
private:
    void bind(int resolution);
    struct Binding { std::array<std::size_t,4> nodes{};std::array<double,4> weights{};unsigned char count{}; };
    std::vector<Vec3> coordinates_,positions_,normals_,scratch_;
    std::vector<unsigned short> indices_;
    std::vector<unsigned short> neighbours_;
    std::vector<std::size_t> neighbourOffsets_;
    std::vector<double> inverseDegree_;
    std::vector<Binding> bindings_;
    std::vector<Vec3> interpolatedNodes_;
    int boundResolution_{};
};
}
