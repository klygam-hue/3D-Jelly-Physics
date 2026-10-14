// Copyright (c) 2026 Mistrul.
// SPDX-License-Identifier: LicenseRef-PhysikX-Proprietary-1.0
// See the accompanying LICENSE for permitted use.

#include <PhysikX/physikx.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
namespace {
thread_local std::string error;
px_status fail(px_status s, const char *text) {
  error = text;
  return s;
}
px_vec3 add(px_vec3 a, px_vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
px_vec3 sub(px_vec3 a, px_vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
px_vec3 mul(px_vec3 a, double b) { return {a.x * b, a.y * b, a.z * b}; }
double dot(px_vec3 a, px_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double length(px_vec3 a) { return std::hypot(a.x, a.y, a.z); }
bool finite(px_vec3 a) {
  return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
}
bool bounded(px_vec3 a) {
  return finite(a) && std::abs(a.x) <= 1e9 && std::abs(a.y) <= 1e9 &&
         std::abs(a.z) <= 1e9;
}
px_vec3 clampv(px_vec3 p, px_vec3 lo, px_vec3 hi) {
  return {std::clamp(p.x, lo.x, hi.x), std::clamp(p.y, lo.y, hi.y),
          std::clamp(p.z, lo.z, hi.z)};
}
struct Body {
  px_id id;
  px_body_desc d;
  px_vec3 force{}, previous{};
  double inv;
  double breaking_force = 0, load = 0, impulse_load = 0;
};
struct Link {
  px_id id;
  px_distance_desc d;
  double lambda = 0;
  double breaking_force = 0, force = 0;
  bool broken = false;
};
struct Bounds {
  px_vec3 lo, hi;
};
double volume(const Body &b, uint32_t dimension) {
  constexpr double pi = 3.14159265358979323846;
  if (b.d.shape == PX_SPHERE)
    return dimension == 2 ? pi * b.d.radius * b.d.radius
                          : (4.0 / 3.0) * pi * b.d.radius * b.d.radius * b.d.radius;
  return dimension == 2 ? 4 * b.d.half_extents.x * b.d.half_extents.y
                        : 8 * b.d.half_extents.x * b.d.half_extents.y * b.d.half_extents.z;
}
Bounds bounds(const Body &b, uint32_t dim) {
  auto e = b.d.shape == PX_SPHERE ? px_vec3{b.d.radius, b.d.radius, b.d.radius}
                                  : b.d.half_extents;
  if (dim == 2)
    e.z = 0;
  return {sub(b.d.position, e), add(b.d.position, e)};
}
bool overlaps(Bounds a, Bounds b) {
  return a.lo.x <= b.hi.x && a.hi.x >= b.lo.x && a.lo.y <= b.hi.y &&
         a.hi.y >= b.lo.y && a.lo.z <= b.hi.z && a.hi.z >= b.lo.z;
}
// Normal points from a toward b. Boxes remain axis aligned.
bool collide(const Body &a, const Body &b, uint32_t dim, px_contact &c) {
  c.a = a.id;
  c.b = b.id;
  if (a.d.shape == PX_SPHERE && b.d.shape == PX_SPHERE) {
    auto d = sub(b.d.position, a.d.position);
    double len = length(d), r = a.d.radius + b.d.radius;
    if (len >= r)
      return false;
    c.normal = len > 1e-12 ? mul(d, 1 / len) : px_vec3{1, 0, 0};
    c.penetration = r - len;
    return true;
  }
  if (a.d.shape == PX_BOX && b.d.shape == PX_BOX) {
    auto ba = bounds(a, dim), bb = bounds(b, dim);
    if (!overlaps(ba, bb))
      return false;
    auto d = sub(b.d.position, a.d.position);
    auto sum = add(a.d.half_extents, b.d.half_extents);
    double p[3] = {sum.x - std::abs(d.x), sum.y - std::abs(d.y),
                   sum.z - std::abs(d.z)};
    int axis = 0;
    for (int i = 1; i < (dim == 2 ? 2 : 3); ++i)
      if (p[i] < p[axis])
        axis = i;
    if (p[axis] <= 0)
      return false;
    c.normal = {0, 0, 0};
    if (axis == 0)
      c.normal.x = d.x >= 0 ? 1 : -1;
    if (axis == 1)
      c.normal.y = d.y >= 0 ? 1 : -1;
    if (axis == 2)
      c.normal.z = d.z >= 0 ? 1 : -1;
    c.penetration = p[axis];
    return true;
  }
  bool flip = a.d.shape == PX_BOX;
  const auto &s = flip ? b : a;
  const auto &box = flip ? a : b;
  auto bb = bounds(box, dim);
  auto nearest = clampv(s.d.position, bb.lo, bb.hi);
  auto delta = sub(nearest, s.d.position);
  double len = length(delta);
  px_vec3 n{};
  double pen;
  if (len > 1e-12) {
    if (len >= s.d.radius)
      return false;
    n = mul(delta, 1 / len);
    pen = s.d.radius - len;
  } else {
    auto local = sub(s.d.position, box.d.position);
    auto e = box.d.half_extents;
    double p[3] = {e.x - std::abs(local.x), e.y - std::abs(local.y),
                   e.z - std::abs(local.z)};
    int axis = 0;
    for (int i = 1; i < (dim == 2 ? 2 : 3); ++i)
      if (p[i] < p[axis])
        axis = i;
    // The inward normal makes positional correction push the sphere out of the
    // box.
    if (axis == 0)
      n.x = local.x >= 0 ? -1 : 1;
    if (axis == 1)
      n.y = local.y >= 0 ? -1 : 1;
    if (axis == 2)
      n.z = local.z >= 0 ? -1 : 1;
    pen = s.d.radius + p[axis];
  }
  c.normal = flip ? mul(n, -1) : n;
  c.penetration = pen;
  return true;
}
} // namespace
struct px_world {
  px_world_desc d;
  std::vector<Body> bodies;
  std::unordered_map<px_id, size_t> indices;
  std::vector<Link> links;
  std::vector<size_t> order;
  std::vector<px_contact> contacts;
  std::vector<px_break_event> break_events;
  std::vector<px_id> fracture_queue;
  px_id next = 1;
  double accumulator = 0;
  px_stats stats{};
  explicit px_world(px_world_desc desc) : d(desc) {}
  Body *find(px_id id) {
    auto it = indices.find(id);
    return it == indices.end() ? nullptr : &bodies[it->second];
  }
  const Body *find(px_id id) const {
    auto it = indices.find(id);
    return it == indices.end() ? nullptr : &bodies[it->second];
  }
  px_vec3 planar(px_vec3 v) const {
    if (d.dimension == 2)
      v.z = 0;
    return v;
  }
  Link *find_link(px_id id) {
    auto it = std::find_if(links.begin(), links.end(),
                           [&](const Link &l) { return l.id == id; });
    return it == links.end() ? nullptr : &*it;
  }
  void erase_body(px_id id) {
    auto it = indices.find(id);
    size_t i = it->second;
    links.erase(std::remove_if(links.begin(), links.end(),
                  [&](const Link &l) { return l.d.a == id || l.d.b == id; }), links.end());
    if (i + 1 < bodies.size()) {
      bodies[i] = bodies.back();
      indices.at(bodies[i].id) = i;
    }
    bodies.pop_back();
    indices.erase(it);
    contacts.clear();
    stats.contacts = 0;
  }
  bool fragments(const Body &parent, Body (&parts)[8]) const {
    uint32_t count = 1u << d.dimension;
    if (!parent.inv || parent.d.mass / count < 1e-9)
      return false;
    for (uint32_t i = 0; i < count; ++i) {
      parts[i] = parent;
      auto &p = parts[i];
      p.d.mass /= count;
      p.inv = 1 / p.d.mass;
      p.force = mul(parent.force, 1.0 / count);
      p.breaking_force = p.load = p.impulse_load = 0;
      p.d.radius *= 0.5;
      p.d.half_extents.x *= 0.5;
      p.d.half_extents.y *= 0.5;
      if (d.dimension == 3) p.d.half_extents.z *= 0.5;
      auto e = parent.d.shape == PX_SPHERE
          ? px_vec3{parent.d.radius * 0.5, parent.d.radius * 0.5, parent.d.radius * 0.5}
          : p.d.half_extents;
      p.d.position = add(parent.d.position,
          {i & 1 ? e.x : -e.x, i & 2 ? e.y : -e.y,
           d.dimension == 3 ? (i & 4 ? e.z : -e.z) : 0});
      p.previous = p.d.position;
      if (!bounded(p.d.position) || p.d.radius <= 0 ||
          p.d.half_extents.x <= 0 || p.d.half_extents.y <= 0 || p.d.half_extents.z <= 0)
        return false;
    }
    return true;
  }
  px_break_event fracture(const Body &parent) {
    Body parts[8];
    if (!fragments(parent, parts)) throw std::runtime_error("Body cannot be fragmented");
    uint32_t count = 1u << d.dimension;
    if (next > UINT64_MAX - count) throw std::runtime_error("Handle space exhausted");
    // Complete all allocating work before removing the parent. Roll back map
    // nodes if allocation fails. No stale references survive vector growth.
    bodies.reserve(bodies.size() + count);
    indices.reserve(indices.size() + count);
    break_events.reserve(break_events.size() + 1);
    uint32_t inserted = 0;
    try {
      for (; inserted < count; ++inserted)
        indices.emplace(next + inserted, bodies.size() + inserted);
    } catch (...) {
      for (uint32_t i = 0; i < inserted; ++i) indices.erase(next + i);
      throw;
    }
    px_break_event event{PX_BODY_FRACTURED, count, parent.id, {}, parent.load};
    for (uint32_t i = 0; i < count; ++i) {
      parts[i].id = next++;
      event.fragments[i] = parts[i].id;
      bodies.push_back(parts[i]);
    }
    erase_body(parent.id);
    break_events.push_back(event);
    return event;
  }
  void limit(Body &b) {
    double speed = length(b.d.velocity);
    if (speed > d.max_speed)
      b.d.velocity = mul(b.d.velocity, d.max_speed / speed);
  }
  void detect() {
    contacts.clear();
    order.resize(bodies.size());
    for (size_t i = 0; i < order.size(); ++i)
      order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      double x = bounds(bodies[a], d.dimension).lo.x,
             y = bounds(bodies[b], d.dimension).lo.x;
      return x == y ? bodies[a].id < bodies[b].id : x < y;
    });
    stats.candidates = 0;
    for (size_t ii = 0; ii < order.size(); ++ii) {
      auto &a = bodies[order[ii]];
      auto ba = bounds(a, d.dimension);
      for (size_t jj = ii + 1; jj < order.size(); ++jj) {
        auto &b = bodies[order[jj]];
        auto bb = bounds(b, d.dimension);
        if (bb.lo.x > ba.hi.x)
          break;
        if ((a.inv + b.inv) == 0 || !(a.d.layer & b.d.mask) ||
            !(b.d.layer & a.d.mask) || !overlaps(ba, bb))
          continue;
        ++stats.candidates;
        px_contact c{};
        if (collide(a, b, d.dimension, c))
          contacts.push_back(c);
      }
    }
    stats.contacts = contacts.size();
  }
  void tick() {
    double dt = d.fixed_dt;
    for (auto &b : bodies) {
      b.previous = b.d.position;
      b.load = b.breaking_force > 0 ? std::max(length(b.force), b.impulse_load) : 0;
      b.impulse_load = 0;
      if (!b.inv)
        continue;
      b.d.velocity =
          add(b.d.velocity, mul(add(d.gravity, mul(b.force, b.inv)), dt));
      b.d.velocity = mul(b.d.velocity, std::exp(-d.linear_damping * dt));
      limit(b);
      b.d.position = add(b.d.position, mul(b.d.velocity, dt));
    }
    for (auto &l : links) {
      l.lambda = 0;
      l.force = 0;
    }
    // XPBD distance links: compliance is independent of iteration count.
    for (uint32_t i = 0; i < d.iterations; ++i)
      for (auto &l : links) {
        if (l.broken) continue;
        auto *a = find(l.d.a);
        auto *b = find(l.d.b);
        auto delta = sub(b->d.position, a->d.position);
        double len = length(delta), w = a->inv + b->inv;
        if (w == 0)
          continue;
        auto n = len > 1e-12 ? mul(delta, 1 / len) : px_vec3{1, 0, 0};
        double alpha = l.d.compliance / (dt * dt);
        double dl = (-(len - l.d.rest_length) - alpha * l.lambda) / (w + alpha);
        l.force = std::abs(l.lambda + dl) / (dt * dt);
        if (l.breaking_force > 0 && l.force > l.breaking_force) {
          l.broken = true;
          continue;
        }
        l.lambda += dl;
        a->d.position = sub(a->d.position, mul(n, dl * a->inv));
        b->d.position = add(b->d.position, mul(n, dl * b->inv));
      }
    for (auto &l : links) {
      if (l.broken) {
        break_events.push_back({PX_DISTANCE_BROKEN, 0, l.id, {}, l.force});
      } else {
        auto *a = find(l.d.a), *b = find(l.d.b);
        if (a->breaking_force > 0) a->load = std::max(a->load, l.force);
        if (b->breaking_force > 0) b->load = std::max(b->load, l.force);
      }
    }
    links.erase(std::remove_if(links.begin(), links.end(),
                              [](const Link &l) { return l.broken; }), links.end());
    if (!links.empty())
      for (auto &b : bodies)
        if (b.inv) {
          b.d.velocity = mul(sub(b.d.position, b.previous), 1 / dt);
          limit(b);
        }
    detect();
    // Resolve velocities iteratively; restitution only acts above a resting
    // threshold.
    for (uint32_t iteration = 0; iteration < d.iterations; ++iteration)
      for (auto &c : contacts) {
        auto *a = find(c.a);
        auto *b = find(c.b);
        double w = a->inv + b->inv;
        auto relative = sub(b->d.velocity, a->d.velocity);
        double vn = dot(relative, c.normal);
        if (vn >= 0)
          continue;
        double e = vn < -0.5 ? std::min(a->d.restitution, b->d.restitution) : 0;
        double j = -(1 + e) * vn / w;
        if (a->breaking_force > 0) a->load = std::max(a->load, j / dt);
        if (b->breaking_force > 0) b->load = std::max(b->load, j / dt);
        auto impulse = mul(c.normal, j);
        a->d.velocity = sub(a->d.velocity, mul(impulse, a->inv));
        b->d.velocity = add(b->d.velocity, mul(impulse, b->inv));
        auto tangent = sub(relative, mul(c.normal, vn));
        double t = length(tangent);
        if (t > 1e-12) {
          double friction = std::sqrt(a->d.friction * b->d.friction);
          double jt = std::min(t / w, j * friction);
          auto f = mul(tangent, -jt / t);
          a->d.velocity = sub(a->d.velocity, mul(f, a->inv));
          b->d.velocity = add(b->d.velocity, mul(f, b->inv));
        }
      }
    // Recompute penetration while correcting multi-contact stacks.
    for (uint32_t i = 0; i < d.iterations; ++i)
      for (auto &initial : contacts) {
        auto *a = find(initial.a);
        auto *b = find(initial.b);
        px_contact c{};
        if (!collide(*a, *b, d.dimension, c))
          continue;
        double w = a->inv + b->inv;
        double amount = std::max(c.penetration - 1e-5, 0.0) * 0.8 / w;
        a->d.position = sub(a->d.position, mul(c.normal, amount * a->inv));
        b->d.position = add(b->d.position, mul(c.normal, amount * b->inv));
      }
    fracture_queue.clear();
    for (auto &b : bodies) {
      b.d.position = planar(b.d.position);
      b.d.velocity = planar(b.d.velocity);
      limit(b);
      if (b.breaking_force > 0 && b.load > b.breaking_force && b.inv) {
        Body parts[8];
        if (fragments(b, parts)) fracture_queue.push_back(b.id);
      }
    }
    // Inspect only the bodies present before fracture; children are unbreakable
    // until the caller assigns strength, bounding fragment growth per step.
    for (auto id : fracture_queue) {
      Body parent = *find(id);
      fracture(parent);
    }
    ++stats.steps;
  }
};
#define PX_GUARD_BEGIN try {
#define PX_GUARD_END                                                           \
  }                                                                            \
  catch (const std::bad_alloc &) {                                             \
    return fail(PX_OUT_OF_MEMORY, "Allocation failed");                        \
  }                                                                            \
  catch (...) {                                                                \
    return fail(PX_INTERNAL_ERROR, "Unexpected native error");                 \
  }
extern "C" {
const char *px_version(void) { return "0.2.0-beta.1"; }
const char *px_last_error(void) { return error.c_str(); }
px_world_desc px_world_defaults(uint32_t dim) {
  return {dim, 8, 16, 0, 1.0 / 120.0, 0.05, 1000, {0, -9.81, 0}};
}
px_body_desc px_body_defaults(void) {
  return {PX_SPHERE,       0,   1, UINT32_MAX, {0, 0, 0}, {0, 0, 0},
          {0.5, 0.5, 0.5}, 0.5, 1, 0.2,        0.5,       0};
}
px_world *px_world_create(const px_world_desc *d) {
  if (!d || (d->dimension != 2 && d->dimension != 3) || d->reserved != 0 ||
      d->iterations < 1 || d->iterations > 128 || d->max_substeps < 1 ||
      d->max_substeps > 4096 || !std::isfinite(d->fixed_dt) ||
      d->fixed_dt < 1e-6 || d->fixed_dt > 1 ||
      !std::isfinite(d->linear_damping) || d->linear_damping < 0 ||
      d->linear_damping > 1e6 || !std::isfinite(d->max_speed) ||
      d->max_speed <= 0 || d->max_speed > 1e9 || !bounded(d->gravity)) {
    fail(PX_INVALID_ARGUMENT, "Invalid world descriptor");
    return nullptr;
  }
  try {
    auto *w = new px_world(*d);
    w->d.gravity = w->planar(w->d.gravity);
    return w;
  } catch (...) {
    fail(PX_OUT_OF_MEMORY, "World allocation failed");
    return nullptr;
  }
}
void px_world_destroy(px_world *w) { delete w; }
px_status px_world_step(px_world *w, double elapsed) {
  PX_GUARD_BEGIN
  if (!w || !std::isfinite(elapsed) || elapsed < 0 || elapsed > 3600)
    return fail(PX_INVALID_ARGUMENT,
                "Elapsed time must be finite and in [0,3600]");
  double total = w->accumulator + elapsed,
         cap = w->d.fixed_dt * w->d.max_substeps;
  if (total > cap) {
    w->stats.dropped_time += total - cap;
    total = cap;
  }
  w->accumulator = total;
  uint32_t count = 0;
  while (w->accumulator + 1e-12 >= w->d.fixed_dt && count < w->d.max_substeps) {
    if (count == 0) w->break_events.clear();
    w->tick();
    w->accumulator = std::max(0.0, w->accumulator - w->d.fixed_dt);
    ++count;
  }
  // Forces persist until a step actually consumes time, and across all its
  // substeps.
  if (count)
    for (auto &b : w->bodies)
      b.force = {0, 0, 0};
  return PX_OK;
  PX_GUARD_END
}
px_status px_world_set_gravity(px_world *w, px_vec3 v) {
  if (!w || !bounded(v))
    return fail(PX_INVALID_ARGUMENT, "Invalid gravity");
  w->d.gravity = w->planar(v);
  return PX_OK;
}
px_status px_world_get_stats(const px_world *w, px_stats *out) {
  if (!w || !out)
    return fail(PX_INVALID_ARGUMENT, "Null world or output");
  *out = w->stats;
  out->bodies = w->bodies.size();
  out->constraints = w->links.size();
  return PX_OK;
}
px_status px_body_create(px_world *w, const px_body_desc *d, px_id *out) {
  PX_GUARD_BEGIN
  if (!w || !d || !out || d->shape > PX_BOX || d->is_static > 1 ||
      !bounded(d->position) || !bounded(d->velocity) ||
      !bounded(d->half_extents) || d->half_extents.x <= 0 ||
      d->half_extents.y <= 0 || d->half_extents.z <= 0 ||
      !std::isfinite(d->radius) || d->radius <= 0 || d->radius > 1e9 ||
      !std::isfinite(d->mass) || d->mass < 1e-9 || d->mass > 1e12 ||
      !std::isfinite(d->restitution) || d->restitution < 0 ||
      d->restitution > 1 || !std::isfinite(d->friction) || d->friction < 0 ||
      d->friction > 10)
    return fail(PX_INVALID_ARGUMENT, "Invalid body descriptor");
  if (w->next == UINT64_MAX)
    return fail(PX_INTERNAL_ERROR, "Handle space exhausted");
  auto desc = *d;
  desc.position = w->planar(desc.position);
  desc.velocity = desc.is_static ? px_vec3{} : w->planar(desc.velocity);
  px_id id = w->next++;
  w->bodies.push_back(
      {id, desc, {}, desc.position, desc.is_static ? 0 : 1 / desc.mass});
  try {
    w->indices.emplace(id, w->bodies.size() - 1);
  } catch (...) {
    w->bodies.pop_back();
    throw;
  }
  *out = id;
  return PX_OK;
  PX_GUARD_END
}
px_status px_body_destroy(px_world *w, px_id id) {
  if (!w)
    return fail(PX_INVALID_ARGUMENT, "Null world");
  auto it = w->indices.find(id);
  if (it == w->indices.end())
    return fail(PX_NOT_FOUND, "Unknown body");
  w->erase_body(id);
  return PX_OK;
}
px_status px_body_get_state(const px_world *w, px_id id, px_body_state *out) {
  if (!w || !out)
    return fail(PX_INVALID_ARGUMENT, "Null world or output");
  auto *b = w->find(id);
  if (!b)
    return fail(PX_NOT_FOUND, "Unknown body");
  *out = {b->d.position, b->d.velocity, b->d.user_data};
  return PX_OK;
}
px_status px_body_set_position(px_world *w, px_id id, px_vec3 v) {
  if (!w || !bounded(v))
    return fail(PX_INVALID_ARGUMENT, "Invalid position");
  auto *b = w->find(id);
  if (!b)
    return fail(PX_NOT_FOUND, "Unknown body");
  b->d.position = w->planar(v);
  w->contacts.clear();
  w->stats.contacts = 0;
  return PX_OK;
}
px_status px_body_set_velocity(px_world *w, px_id id, px_vec3 v) {
  if (!w || !bounded(v))
    return fail(PX_INVALID_ARGUMENT, "Invalid velocity");
  auto *b = w->find(id);
  if (!b)
    return fail(PX_NOT_FOUND, "Unknown body");
  if (!b->inv)
    return fail(PX_INVALID_ARGUMENT, "Static body cannot move");
  b->d.velocity = w->planar(v);
  w->limit(*b);
  return PX_OK;
}
px_status px_body_apply_force(px_world *w, px_id id, px_vec3 v) {
  if (!w || !bounded(v))
    return fail(PX_INVALID_ARGUMENT, "Invalid force");
  auto *b = w->find(id);
  if (!b)
    return fail(PX_NOT_FOUND, "Unknown body");
  if (!b->inv)
    return fail(PX_INVALID_ARGUMENT, "Static body cannot receive forces");
  auto sum = add(b->force, w->planar(v));
  if (!bounded(sum))
    return fail(PX_INVALID_ARGUMENT, "Accumulated force exceeds limit");
  b->force = sum;
  return PX_OK;
}
px_status px_body_apply_impulse(px_world *w, px_id id, px_vec3 v) {
  if (!w || !bounded(v))
    return fail(PX_INVALID_ARGUMENT, "Invalid impulse");
  auto *b = w->find(id);
  if (!b)
    return fail(PX_NOT_FOUND, "Unknown body");
  if (!b->inv)
    return fail(PX_INVALID_ARGUMENT, "Static body cannot receive impulses");
  b->d.velocity = add(b->d.velocity, mul(w->planar(v), b->inv));
  if (b->breaking_force > 0)
    b->impulse_load = std::max(b->impulse_load, length(w->planar(v)) / w->d.fixed_dt);
  w->limit(*b);
  return PX_OK;
}
px_status px_distance_create(px_world *w, const px_distance_desc *d,
                             px_id *out) {
  PX_GUARD_BEGIN
  if (!w || !d || !out || d->a == d->b || !std::isfinite(d->rest_length) ||
      d->rest_length < 0 || d->rest_length > 1e9 ||
      !std::isfinite(d->compliance) || d->compliance < 0 || d->compliance > 1e9)
    return fail(PX_INVALID_ARGUMENT, "Invalid distance descriptor");
  if (!w->find(d->a) || !w->find(d->b))
    return fail(PX_NOT_FOUND, "Unknown constraint body");
  if (w->next == UINT64_MAX)
    return fail(PX_INTERNAL_ERROR, "Handle space exhausted");
  px_id id = w->next++;
  w->links.push_back({id, *d, 0});
  *out = id;
  return PX_OK;
  PX_GUARD_END
}
px_status px_distance_destroy(px_world *w, px_id id) {
  if (!w)
    return fail(PX_INVALID_ARGUMENT, "Null world");
  auto it = std::find_if(w->links.begin(), w->links.end(),
                         [&](const Link &l) { return l.id == id; });
  if (it == w->links.end())
    return fail(PX_NOT_FOUND, "Unknown constraint");
  w->links.erase(it);
  return PX_OK;
}
px_status px_body_get_mass_properties(const px_world *w, px_id id, px_mass_properties *out) {
  if (!w || !out) return fail(PX_INVALID_ARGUMENT, "Null world or output");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  double v = volume(*b, w->d.dimension), density = b->d.mass / v;
  if (!(v > 0) || !std::isfinite(v) || !std::isfinite(density))
    return fail(PX_INVALID_ARGUMENT, "Geometry is too small for finite mass properties");
  auto weight = mul(w->d.gravity, b->d.mass);
  *out = {v, density, b->d.mass, length(weight), weight};
  return PX_OK;
}
px_status px_body_set_mass(px_world *w, px_id id, double mass) {
  if (!w || !std::isfinite(mass) || mass < 1e-9 || mass > 1e12)
    return fail(PX_INVALID_ARGUMENT, "Mass must be in [1e-9,1e12] kg");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  b->d.mass = mass;
  b->inv = b->d.is_static ? 0 : 1 / mass;
  return PX_OK;
}
px_status px_body_set_density(px_world *w, px_id id, double density) {
  if (!w || !std::isfinite(density) || density <= 0)
    return fail(PX_INVALID_ARGUMENT, "Density must be finite and positive");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  return px_body_set_mass(w, id, density * volume(*b, w->d.dimension));
}
px_status px_body_apply_thrust(px_world *w, px_id id, px_vec3 direction, double force) {
  if (!w || !bounded(direction) || !std::isfinite(force) || force < 0 || force > 1e9)
    return fail(PX_INVALID_ARGUMENT, "Invalid thrust direction or force");
  direction = w->planar(direction);
  double len = length(direction);
  if (len < 1e-12) return fail(PX_INVALID_ARGUMENT, "Zero thrust direction");
  return px_body_apply_force(w, id, mul(direction, force / len));
}
px_status px_body_get_material(const px_world *w, px_id id, px_material *out) {
  if (!w || !out) return fail(PX_INVALID_ARGUMENT, "Null world or output");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  *out = {b->d.friction, b->d.restitution, b->breaking_force};
  return PX_OK;
}
px_status px_body_set_friction(px_world *w, px_id id, double friction) {
  if (!w || !std::isfinite(friction) || friction < 0 || friction > 10)
    return fail(PX_INVALID_ARGUMENT, "Friction must be in [0,10]");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  b->d.friction = friction;
  return PX_OK;
}
px_status px_body_set_strength(px_world *w, px_id id, double force) {
  if (!w || !std::isfinite(force) || force < 0 || force > 1e12)
    return fail(PX_INVALID_ARGUMENT, "Breaking force must be in [0,1e12] N");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  if (force > 0 && !b->inv) return fail(PX_INVALID_ARGUMENT, "Static body cannot fracture");
  b->breaking_force = force;
  return PX_OK;
}
px_status px_distance_get_properties(const px_world *w, px_id id, px_distance_properties *out) {
  if (!w || !out) return fail(PX_INVALID_ARGUMENT, "Null world or output");
  auto it = std::find_if(w->links.begin(), w->links.end(), [&](const Link &l) { return l.id == id; });
  if (it == w->links.end()) return fail(PX_NOT_FOUND, "Unknown constraint");
  *out = {it->d.a, it->d.b, it->d.rest_length, it->d.compliance, it->breaking_force, it->force};
  return PX_OK;
}
px_status px_distance_set_elasticity(px_world *w, px_id id, double stiffness) {
  if (!w || !std::isfinite(stiffness) || stiffness < 1e-9 || stiffness > 1e12)
    return fail(PX_INVALID_ARGUMENT, "Stiffness must be in [1e-9,1e12] N/m");
  auto *l = w->find_link(id);
  if (!l) return fail(PX_NOT_FOUND, "Unknown constraint");
  l->d.compliance = 1 / stiffness;
  l->lambda = l->force = 0;
  return PX_OK;
}
px_status px_distance_set_strength(px_world *w, px_id id, double force) {
  if (!w || !std::isfinite(force) || force < 0 || force > 1e12)
    return fail(PX_INVALID_ARGUMENT, "Breaking force must be in [0,1e12] N");
  auto *l = w->find_link(id);
  if (!l) return fail(PX_NOT_FOUND, "Unknown constraint");
  l->breaking_force = force;
  return PX_OK;
}
px_status px_body_fracture(px_world *w, px_id id, px_break_event *out) {
  PX_GUARD_BEGIN
  if (!w || !out) return fail(PX_INVALID_ARGUMENT, "Null world or output");
  auto *b = w->find(id);
  if (!b) return fail(PX_NOT_FOUND, "Unknown body");
  Body parent = *b, parts[8];
  if (!w->fragments(parent, parts))
    return fail(PX_INVALID_ARGUMENT, "Fracture needs a dynamic body, valid child positions and mass >= child_count * 1e-9 kg");
  if (w->next > UINT64_MAX - (1u << w->d.dimension))
    return fail(PX_INTERNAL_ERROR, "Handle space exhausted");
  *out = w->fracture(parent);
  return PX_OK;
  PX_GUARD_END
}
size_t px_world_break_events(const px_world *w, px_break_event *out, size_t capacity) {
  if (!w) { fail(PX_INVALID_ARGUMENT, "Null world"); return 0; }
  if (out) std::copy_n(w->break_events.begin(), std::min(capacity, w->break_events.size()), out);
  return w->break_events.size();
}
size_t px_world_contacts(const px_world *w, px_contact *out, size_t cap) {
  if (!w) {
    fail(PX_INVALID_ARGUMENT, "Null world");
    return 0;
  }
  if (out)
    std::copy_n(w->contacts.begin(), std::min(cap, w->contacts.size()), out);
  return w->contacts.size();
}
size_t px_world_query_aabb(const px_world *w, px_vec3 lo, px_vec3 hi,
                           px_id *out, size_t cap) {
  if (!w || !bounded(lo) || !bounded(hi) || lo.x > hi.x || lo.y > hi.y ||
      lo.z > hi.z) {
    fail(PX_INVALID_ARGUMENT, "Invalid AABB");
    return 0;
  }
  if (w->d.dimension == 2) {
    lo.z = 0;
    hi.z = 0;
  }
  size_t count = 0;
  for (const auto &b : w->bodies)
    if (overlaps({lo, hi}, bounds(b, w->d.dimension))) {
      if (out && count < cap)
        out[count] = b.id;
      ++count;
    }
  return count;
}
px_status px_world_raycast(const px_world *w, px_vec3 origin, px_vec3 dir,
                           double max_distance, uint32_t mask,
                           px_ray_hit *out) {
  if (!w || !out || !bounded(origin) || !bounded(dir) ||
      !std::isfinite(max_distance) || max_distance < 0 || max_distance > 1e9)
    return fail(PX_INVALID_ARGUMENT, "Invalid ray");
  origin = w->planar(origin);
  dir = w->planar(dir);
  double len = length(dir);
  if (len < 1e-12)
    return fail(PX_INVALID_ARGUMENT, "Zero ray direction");
  dir = mul(dir, 1 / len);
  bool found = false;
  double best = max_distance;
  for (const auto &b : w->bodies) {
    if (!(b.d.layer & mask))
      continue;
    double t = 0;
    px_vec3 n{};
    if (b.d.shape == PX_SPHERE) {
      auto oc = sub(origin, b.d.position);
      double c = dot(oc, oc) - b.d.radius * b.d.radius;
      if (c > 0) {
        double q = dot(oc, dir), disc = q * q - c;
        if (disc < 0)
          continue;
        t = -q - std::sqrt(disc);
        if (t < 0)
          continue;
        n = mul(sub(add(origin, mul(dir, t)), b.d.position), 1 / b.d.radius);
      }
    } else {
      auto bb = bounds(b, w->d.dimension);
      double o[3] = {origin.x, origin.y, origin.z},
             v[3] = {dir.x, dir.y, dir.z}, lo[3] = {bb.lo.x, bb.lo.y, bb.lo.z},
             hi[3] = {bb.hi.x, bb.hi.y, bb.hi.z};
      double far = best;
      bool miss = false;
      for (int i = 0; i < (w->d.dimension == 2 ? 2 : 3); ++i) {
        if (std::abs(v[i]) < 1e-15) {
          if (o[i] < lo[i] || o[i] > hi[i])
            miss = true;
          continue;
        }
        double near = (lo[i] - o[i]) / v[i], end = (hi[i] - o[i]) / v[i],
               sign = -1;
        if (near > end) {
          std::swap(near, end);
          sign = 1;
        }
        if (near > t) {
          t = near;
          n = {0, 0, 0};
          if (i == 0)
            n.x = sign;
          if (i == 1)
            n.y = sign;
          if (i == 2)
            n.z = sign;
        }
        far = std::min(far, end);
        if (t > far)
          miss = true;
      }
      if (miss || far < 0)
        continue;
    }
    if (t <= best && (!found || t < best || b.id < out->body)) {
      best = t;
      found = true;
      *out = {b.id, add(origin, mul(dir, t)), n, t};
    }
  }
  return found ? PX_OK : fail(PX_NOT_FOUND, "Ray did not hit a body");
}
}
