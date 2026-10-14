// Copyright (c) 2026 Mistrul.
// SPDX-License-Identifier: LicenseRef-PhysikX-Proprietary-1.0
// See the accompanying LICENSE for permitted use.

#pragma once
#include "physikx.h"
#include <stdexcept>
#include <utility>
#include <vector>
namespace PhysikX {
inline void check(px_status result) {
  if (result != PX_OK)
    throw std::runtime_error(px_last_error());
}
class World {
  px_world *handle_;

public:
  explicit World(px_world_desc desc = px_world_defaults(3))
      : handle_(px_world_create(&desc)) {
    if (!handle_)
      throw std::runtime_error(px_last_error());
  }
  ~World() { px_world_destroy(handle_); }
  World(const World &) = delete;
  World &operator=(const World &) = delete;
  World(World &&other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)) {}
  World &operator=(World &&other) noexcept {
    if (this != &other) {
      px_world_destroy(handle_);
      handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
  }
  px_world *native_handle() const { return handle_; }
  px_id create_body(const px_body_desc &desc) {
    px_id id;
    check(px_body_create(handle_, &desc, &id));
    return id;
  }
  px_body_state state(px_id id) const {
    px_body_state out;
    check(px_body_get_state(handle_, id, &out));
    return out;
  }
  void step(double elapsed) { check(px_world_step(handle_, elapsed)); }
  void gravity(px_vec3 value) { check(px_world_set_gravity(handle_, value)); }
  void position(px_id id, px_vec3 value) {
    check(px_body_set_position(handle_, id, value));
  }
  void velocity(px_id id, px_vec3 value) {
    check(px_body_set_velocity(handle_, id, value));
  }
  void force(px_id id, px_vec3 value) {
    check(px_body_apply_force(handle_, id, value));
  }
  void impulse(px_id id, px_vec3 value) {
    check(px_body_apply_impulse(handle_, id, value));
  }
  void destroy_body(px_id id) { check(px_body_destroy(handle_, id)); }
  px_id distance(const px_distance_desc &desc) {
    px_id id;
    check(px_distance_create(handle_, &desc, &id));
    return id;
  }
  void destroy_distance(px_id id) { check(px_distance_destroy(handle_, id)); }
  px_stats stats() const {
    px_stats out;
    check(px_world_get_stats(handle_, &out));
    return out;
  }
  px_mass_properties mass_properties(px_id id) const {
    px_mass_properties out;
    check(px_body_get_mass_properties(handle_, id, &out));
    return out;
  }
  void set_mass(px_id id, double value) { check(px_body_set_mass(handle_, id, value)); }
  void set_density(px_id id, double value) { check(px_body_set_density(handle_, id, value)); }
  void apply_thrust(px_id id, px_vec3 direction, double force) {
    check(px_body_apply_thrust(handle_, id, direction, force));
  }
  px_material material(px_id id) const {
    px_material out;
    check(px_body_get_material(handle_, id, &out));
    return out;
  }
  void set_friction(px_id id, double value) { check(px_body_set_friction(handle_, id, value)); }
  void set_strength(px_id id, double force) { check(px_body_set_strength(handle_, id, force)); }
  px_distance_properties distance_properties(px_id id) const {
    px_distance_properties out;
    check(px_distance_get_properties(handle_, id, &out));
    return out;
  }
  void set_elasticity(px_id id, double stiffness) { check(px_distance_set_elasticity(handle_, id, stiffness)); }
  void set_distance_strength(px_id id, double force) { check(px_distance_set_strength(handle_, id, force)); }
  px_break_event fracture(px_id id) {
    px_break_event out;
    check(px_body_fracture(handle_, id, &out));
    return out;
  }
  std::vector<px_break_event> break_events() const {
    std::vector<px_break_event> out(px_world_break_events(handle_, nullptr, 0));
    px_world_break_events(handle_, out.data(), out.size());
    return out;
  }
};
} // namespace PhysikX
