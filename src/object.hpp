#pragma once

#ifndef GLAD_GL_IMPLEMENTATION
#include "framework.h"
#endif

struct Object {
    Geometry<vec2> points;

    void addPoint(vec2 p) { points.Vtx().push_back(p); }

    void sync() { points.updateGPU(); }
    
    void draw(GPUProgram* prog, int primitive_type, vec3 color) { 
        points.Draw(prog, primitive_type, color);
    }
};