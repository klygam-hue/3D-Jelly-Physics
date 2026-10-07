#pragma once
#include <algorithm>
#include <array>
#include <numeric>
#include <ostream>
#include <vector>

namespace jely {
class FrameProfiler {
public:
    explicit FrameProfiler(int capacity) { for(auto& stage:stages_)stage.reserve(std::max(0,capacity)); }
    void record(int frame,double physics,double mesh,double draw,double total) {
        if(frame<=60)return; // Exclude initialization and driver warm-up from comparisons.
        const std::array<double,4> values{physics,mesh,draw,total};
        for(std::size_t i=0;i<stages_.size();i++)stages_[i].push_back(values[i]);
    }
    void report(std::ostream& output) const {
        constexpr const char* names[]{"physics_frame","mesh_update","draw_submit","frame_wall"};
        output<<"benchmark_samples="<<stages_[0].size()<<'\n';
        for(std::size_t i=0;i<stages_.size();i++) {
            if(stages_[i].empty())continue;
            auto ordered=stages_[i];std::sort(ordered.begin(),ordered.end());
            double mean=std::accumulate(ordered.begin(),ordered.end(),0.0)/ordered.size();
            output<<names[i]<<"_mean_ms="<<mean<<'\n'<<names[i]<<"_p95_ms="<<ordered[static_cast<std::size_t>((ordered.size()-1)*0.95)]<<'\n';
        }
    }
private:
    std::array<std::vector<double>,4> stages_;
};
}
