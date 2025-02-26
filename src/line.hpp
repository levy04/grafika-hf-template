#pragma once

#include "object.hpp"

struct Line : public Object {
    vec2 v;

    Line(vec2 a, vec2 b) {
        this->addPoint(a);
        this->addPoint(b);

        v = a - b;     // irányvektor
        vec2 n(-v.y, v.x);  // normálvektor
        float d = (-1) * a.x * n.x + a.y*n.y; 

        printf("\tImplicit: %f x + %f y + %f = 0\n", n.x, n.y, d);
        printf("\tExplicit: r(t) = (%f, %f) + (%f, %f)t\n", a.x, a.y, v.x, v.y);
    }

    vec2 intersection_point(Line line_b) {
        return vec2(this->v.x * line_b.v.x, this->v.y * line_b.v.y);
    }

    bool contains_point(vec2 point) {
        vec2 n(-v.y, v.x);

        Line perp(point, point+n);
        vec2 intersect = intersection_point(perp);

        return distance(point, intersect) < 0.01;
    }
};