#include "LineCollection.hpp"
#include "PointCollection.hpp"
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

enum t_state { POINTS, LINES, MOVE_LINES, INTERSECT };

const int winWidth = 600, winHeight = 600;
class pointsAndLinesApp : public glApp {
    t_state state;
    PointCollection* pcoll;
    LineCollection* lcoll;
    GPUProgram* gpuProgram;  // csúcspont és pixel árnyalók

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
        lcoll->draw(gpuProgram, GL_LINE, vec3(0.043f, 0.89f, 0.89f));
    }

    void onKeyboard(int key) {
        switch (key) {
            case 'p':
                state = POINTS;
                break;
            case 'l':
                state = LINES;
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
                    pcoll->addPoint(PixelToNDC(pX, pY));
                    pcoll->sync();
                    refreshScreen();
                }
                break;
            case LINES:
                if (button == MOUSE_LEFT) {
                    vec2 actual = PixelToNDC(pX, pY);
                    for (auto&& point : pcoll->points.Vtx()) {
                        if (distance(point, actual) < 0.01) {
                        }
                    }
                }
        }
    }
};

pointsAndLinesApp app;
