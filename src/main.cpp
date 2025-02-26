#include "framework.h"

// csúcspont árnyaló
const char* vertSource = R"(
	#version 330				

	layout(location = 0) in vec2 cP;

	void main() {
		gl_Position = vec4(cP, 0, 1);
	}
)";

// pixel árnyaló
const char* fragSource = R"(
	#version 330

	uniform vec3 color;
	out vec4 outColor;

	void main() {
		outColor = vec4(color, 1);
	}
)";

// Object
struct Object {
    Geometry<vec2> points;

    void add(vec2 p) { points.Vtx().push_back(p); }
    void sync() { points.updateGPU(); }
    void draw(GPUProgram* prog, int primitive_type, vec3 color) {
        points.Draw(prog, primitive_type, color);
    }
};
//

// PointCollection
struct PointCollection : public Object {
    vec2 find_nearest_point(vec2 source) {
        vec2 nearest = source;
        float min_distance = 1000000.0;

        for (auto&& point : points.Vtx()) {
            float current_dist = distance(source, point);
            if (current_dist < min_distance && current_dist != 0) {
                min_distance = current_dist;
                nearest = point;
            }
        }
        return nearest;
    }
};
//

// Line
struct Line : public Object {
    vec2 line_point;
    vec2 v;

    Line(vec2 a, vec2 b, bool print) {
        this->add(a);
        this->add(b);

        line_point = a;
        v = a - b;          // irányvektor
        vec2 n(-v.y, v.x);  // normálvektor
        float d = (-1) * a.x * n.x + a.y * n.y;

        if (print) {
            printf("\tImplicit: %f x + %f y + %f = 0\n", n.x, n.y, d);
            printf("\tExplicit: r(t) = (%f, %f) + (%f, %f)t\n", a.x, a.y, v.x,
                   v.y);
        }
    }

    vec2 intersection_point(Line line_b) { return cross(vec3(), vec3()); }

    bool contains_point(vec2 point) {
        vec2 n(-v.y, v.x);

        Line perp(point, point + n, false);
        vec2 intersect = intersection_point(perp);

        return distance(point, intersect) < 0.01;
    }

    std::pair<vec2, vec2> bordering_points() {
        std::vector<vec2> intersections = {
            intersection_point(Line(vec2(-1, 1), vec2(-1, -1), false)),
            intersection_point(Line(vec2(-1, 1), vec2(1, 1), false)),
            intersection_point(Line(vec2(1, 1), vec2(1, -1), false)),
            intersection_point(Line(vec2(1, -1), vec2(-1, -1), false)),
        };

        std::vector<vec2> result = {};

        for (auto&& intpoint : intersections) {
            if (intpoint.x >= -1 && intpoint.x <= 1 && intpoint.y >= -1 &&
                intpoint.y <= 1) {
                result.push_back(intpoint);
            }
        }

        return std::make_pair(result[0], result[1]);

        // return std::make_pair(vec2(-1, -1), vec2(1, 1));
    }

    void push_to_point(vec2 target) {
        vec2 push = target - line_point;
        for (auto&& point : this->points.Vtx()) point += push;
    }
};
//

// LineCollection
struct LineCollection : public Object {
    std::vector<Line> lines;

    void addLine(Line l) {
        lines.push_back(l);
        this->add(l.line_point);
        this->add(l.line_point - l.v);
    }

    Line get_closest_to_point(vec2 point) {
        for (auto&& line : lines)
            if (line.contains_point(point)) return line;
    }
};
//

enum t_state { POINTS, LINES_FIRST, LINES_SECOND, MOVE_LINES, INTERSECT };

const int winWidth = 600, winHeight = 600;
class pointsAndLinesApp : public glApp {
    t_state state;
    PointCollection* pcoll;
    LineCollection* lcoll;
    GPUProgram* gpuProgram;  // csúcspont és pixel árnyalók
    vec2 line_points[2];

    vec2 PixelToNDC(int pX, int pY) {
        return vec2(2.0f * pX / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight);
    }

   public:
    pointsAndLinesApp() : glApp("Points And Lines") {}

    void onInitialization() {
        glViewport(0, 0, winWidth, winHeight);
        glLineWidth(3);
        glPointSize(10);
        pcoll = new PointCollection;
        lcoll = new LineCollection;
        state = POINTS;
        gpuProgram = new GPUProgram(vertSource, fragSource);
    }

    void onDisplay() {
        glClearColor(0.3f, 0.3f, 0.3f, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        pcoll->draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
        lcoll->draw(gpuProgram, GL_LINES, vec3(0.043f, 0.89f, 0.89f));
    }

    void onKeyboard(int key) {
        switch (key) {
            case 'p':
                state = POINTS;
                break;
            case 'l':
                state = LINES_FIRST;
                break;
            case 'm':
                state = MOVE_LINES;
                break;
            case 'i':
                state = INTERSECT;
                break;
            default:
                break;
        }
    }

    void onMousePressed(MouseButton button, int pX, int pY) {
        switch (state) {
            case POINTS:
                if (button == MOUSE_LEFT) {
                    vec2 point = PixelToNDC(pX, pY);
                    printf("Point %f, %f added\n", point.x, point.y);
                    pcoll->add(point);
                    pcoll->sync();
                    refreshScreen();
                }
                break;
            case LINES_FIRST:
                if (button == MOUSE_LEFT) {
                    line_points[0] =
                        pcoll->find_nearest_point(PixelToNDC(pX, pY));
                    state = LINES_SECOND;
                }
                break;

            case LINES_SECOND:
                if (button == MOUSE_LEFT) {
                    line_points[1] =
                        pcoll->find_nearest_point(PixelToNDC(pX, pY));
                    state = LINES_FIRST;

                    Line line(line_points[0], line_points[1], true);

                    std::pair<vec2, vec2> borders = line.bordering_points();

                    lcoll->addLine(Line(borders.first, borders.second, true));
                    lcoll->sync();
                    refreshScreen();
                }
                break;
        }
    }
};

pointsAndLinesApp app;
