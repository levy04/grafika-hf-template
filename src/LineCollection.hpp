#pragma once

#include "object.hpp"

struct LineCollection : public Object {
    vec2 find_nearest_point(vec2 source) {
        vec2 nearest = source;
        double min_distance = 0;

        for(auto&& point : points.Vtx()) {
            double current_dist = distance(source, point);
            if (current_dist < min_distance && current_dist != 0) {
                min_distance = current_dist;
                nearest = point;
            }
        }
        return nearest;
    }
};