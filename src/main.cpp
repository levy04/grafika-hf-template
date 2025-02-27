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

    void add(const vec3& p) { points.Vtx().push_back(p); }
    void sync() { points.updateGPU(); }
    void draw(GPUProgram* prog, const int& primitive_type, const vec3& color) {
        points.Draw(prog, primitive_type, color);
    }
};
//

// PointCollection
struct PointCollection : public Object {
    vec3 find_nearest_point(const vec3& source) {
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
struct Line {
    vec3 start_point, end_point;
    vec3 coords;

    Line(const vec3& a = vec3(-1, -1, 1), const vec3& b = vec3(1, 1, 1),
         const bool& print = false) {
        start_point = a;
        end_point = b;
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

    bool operator==(const Line& line) {
        return (this->coords == line.coords &&
                this->start_point == line.start_point &&
                this->end_point == line.end_point);
    }

    bool operator!=(const Line& line) { return !(*this == line); }

    vec3 intersection_point(const Line& line) {
        vec3 inter = cross(this->coords, line.coords);
        return vec3(inter.x / inter.z, inter.y / inter.z, 1);
    }

    bool contains_point(const vec3& point) {
        vec3 lineDir = end_point - start_point;
        vec3 perpDir = normalize(vec3(lineDir.y, -lineDir.x, 0));
        vec3 dirToPointA = start_point - point;
        return abs(dot(perpDir, dirToPointA)) < 0.01;
    }

    std::pair<vec3, vec3> bordering_points() {
        vec3 lineDir = end_point - start_point;

        vec3 end_border = end_point;
        do {
            end_border += lineDir;
        } while ((end_border.x < 1 && end_border.x > -1) ||
                 (end_border.y < 1 && end_border.y > -1));

        vec3 start_border = start_point;
        do {
            start_border -= lineDir;
        } while ((start_border.x < 1 && start_border.x > -1) ||
                 (start_border.y < 1 && start_border.y > -1));

        return std::make_pair(start_border, end_border);
    }

    std::pair<vec3, vec3> push_to_point(vec3 target) {
        vec3 lineDir = normalize(end_point - start_point);

        start_point = target;
        end_point = target + lineDir;

        return bordering_points();
    }
};
//

// LineCollection
struct LineCollection : public Object {
    std::vector<Line> lines;

    void addLine(const Line& l) {
        this->add(l.start_point);
        this->add(l.end_point);
        this->sync();
        lines.push_back(l);
    }

    void move_line(int idx, vec3 target) {
        std::pair<vec3, vec3> result = lines[idx].push_to_point(target);

        lines[idx] = Line(result.first, result.second);

        this->points.Vtx()[2 * idx] = result.first;
        this->points.Vtx()[2 * idx + 1] = result.second;
        this->sync();
    }

    Line* get_closest_to_point(const vec3& point) {
        for (auto&& line : lines)
            if (line.contains_point(point)) return &line;
        return nullptr;
    }

    int get_closest_idx_to_point(const vec3& point) {
        for (int i = 0; i < lines.size(); i++)
            if (lines.at(i).contains_point(point)) return i;
        return -1;
    }
};
//

enum t_state {
    POINTS,
    LINES_FIRST,
    LINES_SECOND,
    SELECT_MOVED_LINE,
    MOVING_LINE,
    INTERSECT_FIRST,
    INTERSECT_SECOND
};

const int winWidth = 600, winHeight = 600;
class pointsAndLinesApp : public glApp {
    t_state state;
    PointCollection* pcoll;
    LineCollection* lcoll;
    GPUProgram* gpuProgram;
    vec3 selected_points[2];
    Line selected_lines[2];
    int moved_line_idx;

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
                state = SELECT_MOVED_LINE;
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
                    selected_points[0] =
                        pcoll->find_nearest_point(PixelToNDC(pX, pY));
                    state = LINES_SECOND;
                }
                break;

            case LINES_SECOND:
                if (button == MOUSE_LEFT) {
                    selected_points[1] =
                        pcoll->find_nearest_point(PixelToNDC(pX, pY));
                    state = LINES_FIRST;

                    Line line(selected_points[0], selected_points[1], true);
                    std::pair<vec3, vec3> borders = line.bordering_points();

                    lcoll->addLine(Line(borders.first, borders.second));
                    lcoll->sync();
                    refreshScreen();
                }
                break;

            case INTERSECT_FIRST:
                if (button == MOUSE_LEFT) {
                    Line* selected =
                        lcoll->get_closest_to_point(PixelToNDC(pX, pY));
                    if (selected == nullptr) break;
                    selected_lines[0] = *selected;
                    state = INTERSECT_SECOND;
                }
                break;

            case INTERSECT_SECOND:
                if (button == MOUSE_LEFT) {
                    Line* selected =
                        lcoll->get_closest_to_point(PixelToNDC(pX, pY));
                    if (selected == nullptr) break;
                    selected_lines[1] = *selected;
                    state = INTERSECT_FIRST;

                    pcoll->add(selected_lines[0].intersection_point(
                        selected_lines[1]));
                    pcoll->sync();
                    refreshScreen();
                }
                break;

            case SELECT_MOVED_LINE:
                if (button == MOUSE_LEFT) {
                    int selected =
                        lcoll->get_closest_idx_to_point(PixelToNDC(pX, pY));
                    if (selected == -1) break;
                    moved_line_idx = selected;
                    state = MOVING_LINE;
                }
                break;
        }
    }

    void onMouseReleased(MouseButton button, int pX, int pY) {
        if (state != MOVING_LINE) return;
        if (button == MOUSE_LEFT) {
            state = SELECT_MOVED_LINE;
            moved_line_idx = -1;
        }
    }

    void onMouseMotion(int pX, int pY) {
        if (state != MOVING_LINE) return;
        lcoll->move_line(moved_line_idx, PixelToNDC(pX, pY));
        lcoll->sync();
        refreshScreen();
    }
};

pointsAndLinesApp app;
