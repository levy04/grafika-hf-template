#include "framework.h"

// csúcspont árnyaló
const char* vertSource = R"(
	#version 330				

	layout(location = 0) in vec3 cP;

	void main() {
		gl_Position = vec4(cP, 1);
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
    Geometry<vec3> points;

    void add(vec3 p) { points.Vtx().push_back(p); }
    void sync() { points.updateGPU(); }
    void draw(GPUProgram* prog, int primitive_type, vec3 color) {
        points.Draw(prog, primitive_type, color);
    }
};
//

// PointCollection
struct PointCollection : public Object {
    vec3 find_nearest_point(vec3 source) {
        vec3 nearest = points.Vtx()[0];
        float min_distance = 1000000.0;

        for (auto&& point : points.Vtx()) {
            float current_dist = distance(source, point);
            if (current_dist < min_distance) {
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
    vec3 point_a, point_b;
    vec3 coords;

    Line(vec3 a = vec3(-1, -1, 1), vec3 b = vec3(1, 1, 1), bool print = false) {
        this->add(a);
        this->add(b);
        point_a = a;
        point_b = b;
        coords = cross(a, b);

        vec3 v(a.x - b.x, a.y - b.y, 0);  // irányvektor
        vec3 n(v.y, -v.x, 0);             // normálvektor
        float d = (-1) * a.x * n.x + a.y * n.y;

        if (print) {
            printf("\tImplicit: %f x + %f y + %f = 0\n", n.x, n.y, d);
            printf("\tExplicit: r(t) = (%f, %f) + (%f, %f)t\n", a.x, a.y, v.x,
                   v.y);
        }
    }

    // static vec3 intersection_point(vec3 line_a, vec3 line_b) {
    //     return cross(vec3(line_a.x, line_a.y, 0), vec3(line_b.x, line_b.y,
    //     0));
    // }

    static vec3 intersection_point(vec3 line_a, vec3 line_b) {
        vec3 inter = cross(line_a, line_b);
        return vec3(inter.x / inter.z, inter.y / inter.z, 1);
    }

    bool contains_point(vec3 point) {
        vec3 point_asdf(point.x + coords.y, point.y - coords.x, 1);
        vec3 point_fdsa(-point.x - coords.y, -point.y + coords.x, 1);
        Line perp_line = Line(point_asdf, point_fdsa);
        vec3 perp = intersection_point(this->coords, perp_line.coords);

        bool result = distance(point, perp) < 0.01;
        printf("%s", result ? "\tYes\n" : "\tNo\n");

        return result;
    }

    // std::pair<vec3, vec3> bordering_points() {

    // }

    // void push_to_point(vec3 target) {
    //     vec2 push(target.x - )
    //     for (auto&& point : this->points.Vtx()) point += push;
    // }
};
//

// LineCollection
struct LineCollection : public Object {
    std::vector<Line> lines;

    void addLine(Line l) {
        this->add(l.point_a);
        this->add(l.point_b);
        lines.push_back(l);
    }

    vec3 get_closest_to_point(vec3 point) {
        for (auto& line : lines)
            if (line.contains_point(point)) return line.coords;
        return lines[0].coords;
    }
};
//

enum t_state {
    POINTS,
    LINES_FIRST,
    LINES_SECOND,
    MOVE_LINES,
    INTERSECT_FIRST,
    INTERSECT_SECOND
};

const int winWidth = 600, winHeight = 600;
class pointsAndLinesApp : public glApp {
    t_state state;
    PointCollection* pcoll;
    LineCollection* lcoll;
    GPUProgram* gpuProgram;  // csúcspont és pixel árnyalók
    vec3 line_points[2];
    vec3 lines[2];

    vec3 PixelToNDC(int pX, int pY) {
        return vec3(2.0f * pX / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight,
                    1);
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
                state = INTERSECT_FIRST;
                break;
            default:
                break;
        }
    }

    void onMousePressed(MouseButton button, int pX, int pY) {
        switch (state) {
            case POINTS:
                if (button == MOUSE_LEFT) {
                    vec3 point = PixelToNDC(pX, pY);
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
                    // std::pair<vec3, vec3> borders = line.bordering_points();

                    // lcoll->addLine(Line(borders.first, borders.second));
                    lcoll->addLine(line);
                    lcoll->sync();
                    refreshScreen();
                }
                break;

            case INTERSECT_FIRST:
                if (button == MOUSE_LEFT) {
                    lines[0] = lcoll->get_closest_to_point(PixelToNDC(pX, pY));
                    state = INTERSECT_SECOND;
                }
                break;

            case INTERSECT_SECOND:
                if (button == MOUSE_LEFT) {
                    lines[1] = lcoll->get_closest_to_point(PixelToNDC(pX, pY));
                    state = INTERSECT_FIRST;

                    pcoll->add(Line::intersection_point(lines[0], lines[1]));
                    pcoll->sync();
                    refreshScreen();
                }
                break;
        }
    }
};

pointsAndLinesApp app;
