//=============================================================================================
// OpenGL keretrendszer
//=============================================================================================
#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define _USE_MATH_DEFINES		// M_PI
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define FILE_OPERATIONS
#ifdef FILE_OPERATIONS
#include <fstream>
#include <filesystem>
#if _HAS_CXX17
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif
#include "lodepng.h"
#endif

using namespace glm;

inline mat4 translate(const vec3& v) { return translate(mat4(1.0f), v); }
inline mat4 scale(const vec3& v) { return scale(mat4(1.0f), v); }
inline mat4 rotate(float angle, const vec3& v) { return rotate(mat4(1.0f), angle, v); }

//---------------------------
class GPUProgram {
//--------------------------
	GLuint shaderProgramId = 0;
	bool waitError = true;

	bool checkShader(unsigned int shader, std::string message) { // shader fordítási hibák kezelése
		GLint infoLogLength = 0, result = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (!result) {
			std::string errorMessage(infoLogLength, '\0');
			glGetShaderInfoLog(shader, infoLogLength, NULL, (GLchar *)errorMessage.data());
			printf("%s! \n Log: \n%s\n", message.c_str(), errorMessage.c_str());
			if (waitError) getchar();
			return false;
		}
		return true;
	}

	bool checkLinking(unsigned int program) { 	// shader szerkesztési hibák kezelése
		GLint infoLogLength = 0, result = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (!result) {
			std::string errorMessage(infoLogLength, '\0');
			glGetProgramInfoLog(program, infoLogLength, nullptr, (GLchar *)errorMessage.data());
			printf("Failed to link shader program! \n Log: \n%s\n", errorMessage.c_str());
			if (waitError) getchar();
			return false;
		}
		return true;
	}

	int getLocation(const std::string& name) {	// uniform változó címének lekérdezése
		int location = glGetUniformLocation(shaderProgramId, name.c_str());
		if (location < 0) printf("uniform %s cannot be set\n", name.c_str());
		return location;
	}

#ifdef FILE_OPERATIONS
	std::string file2string(const fs::path& _fileName) {
		std::string shaderCodeOut = "";
		std::ifstream shaderStream(_fileName);
		if (!shaderStream.is_open()) {
			printf("Error while opening shader code file %s!", _fileName.string().c_str());
			return "";
		}
		std::string line = "";
		while (std::getline(shaderStream, line)) {
			shaderCodeOut += line + "\n";
		}
		shaderStream.close();
		return shaderCodeOut;
	}
#endif

	std::string shaderType2string(GLenum shadeType) {
		switch (shadeType)
		{
		case GL_VERTEX_SHADER:			return "Vertex"; break;
		case GL_FRAGMENT_SHADER:		return "Fragment"; break;
		case GL_GEOMETRY_SHADER:		return "Geometry"; break;
		case GL_TESS_CONTROL_SHADER:	return "Tessellation control"; break;
		case GL_TESS_EVALUATION_SHADER:	return "Tessellation evaluation"; break;
		case GL_COMPUTE_SHADER:			return "Compute"; break;
		default:						return "Unknown [shader type]"; break;
		}
	}

public:
	GPUProgram( ) { }
	GPUProgram(const char* const vertexShaderSource, const char * const fragmentShaderSource, const char * const geometryShaderSource = nullptr) {
		create(vertexShaderSource, fragmentShaderSource, geometryShaderSource);
	}

	void create(const char* const vertexShaderSource, const char * const fragmentShaderSource, const char * const geometryShaderSource = nullptr) {
		// Program létrehozása a forrás sztringbõl
		GLuint  vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) {
			printf("Error in vertex shader creation\n");
			exit(1);
		}
		glShaderSource(vertexShader, 1, (const GLchar**)&vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		if (!checkShader(vertexShader, "Vertex shader error")) return;

		// Program létrehozása a forrás sztringbõl, ha van geometria árnyaló
		GLuint geometryShader = 0;
		if (geometryShaderSource != nullptr) {
			geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
			if (!geometryShader) {
				printf("Error in geometry shader creation\n");
				exit(1);
			}
			glShaderSource(geometryShader, 1, (const GLchar**)&geometryShaderSource, NULL);
			glCompileShader(geometryShader);
			if (!checkShader(geometryShader, "Geometry shader error")) return;
		}

		// Program létrehozása a forrás sztringbõl
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) {
			printf("Error in fragment shader creation\n");
			exit(1);
		}

		glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		if (!checkShader(fragmentShader, "Fragment shader error")) return;

		shaderProgramId = glCreateProgram();
		if (!shaderProgramId) {
			printf("Error in shader program creation\n");
			exit(-1);
		}
		glAttachShader(shaderProgramId, vertexShader);
		glAttachShader(shaderProgramId, fragmentShader);
		if (geometryShader > 0) glAttachShader(shaderProgramId, geometryShader);

		// Szerkesztés
		if (!link()) return;

		// Ez fusson
		glUseProgram(shaderProgramId); 
	}

#ifdef FILE_OPERATIONS
	bool addShader(const fs::path& _fileName) {
		GLenum shaderType = 0;
		auto ext = _fileName.extension();
		if (ext == ".vert") { shaderType = GL_VERTEX_SHADER; }
		else if (ext == ".frag") { shaderType = GL_FRAGMENT_SHADER; }
		else if (ext == ".geom") { shaderType = GL_GEOMETRY_SHADER; }
		else if (ext == ".tesc") { shaderType = GL_TESS_CONTROL_SHADER; }
		else if (ext == ".tese") { shaderType = GL_TESS_EVALUATION_SHADER; }
		else if (ext == ".comp") { shaderType = GL_COMPUTE_SHADER; }
		else { printf("Unknown shader extension"); return false; }
		return addShader(shaderType, _fileName);
	}

	bool addShader(GLenum shaderType, const fs::path& _fileName) {
		std::string shaderCode = file2string(_fileName);
		GLuint shaderID = glCreateShader(shaderType);
		if (!shaderID) {
			printf("Error in vertex shader creation\n");
			exit(1);
		}
		const char* sourcePointer = shaderCode.data();
		GLint sourceLength = static_cast<GLint>(shaderCode.length());
		glShaderSource(shaderID, 1, &sourcePointer, &sourceLength);
		glCompileShader(shaderID);
		if (!checkShader(shaderID, shaderType2string(shaderType) + " shader error")) return false;
		if (shaderProgramId == 0) shaderProgramId = glCreateProgram();
		glAttachShader(shaderProgramId, shaderID);
		return true;
	}
#endif

	bool link() {
		glLinkProgram(shaderProgramId);
		return checkLinking(shaderProgramId);
	}

	void Use() { glUseProgram(shaderProgramId); } 		// make this program run

	void setUniform(int i, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform1i(location, i);
	}

	void setUniform(float f, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform1f(location, f);
	}

	void setUniform(const vec2& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform2fv(location, 1, &v.x);
	}

	void setUniform(const vec3& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform3fv(location, 1, &v.x);
	}

	void setUniform(const vec4& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform4fv(location, 1, &v.x);
	}

	void setUniform(const mat4& mat, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
	}

	~GPUProgram() { if (shaderProgramId > 0) glDeleteProgram(shaderProgramId); }
};

//---------------------------
template<class T>
class Geometry {
//---------------------------
	unsigned int vao, vbo;	// GPU
protected:
	std::vector<T> vtx;	// CPU
public:
	Geometry() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		int nf = min((int)(sizeof(T) / sizeof(float)), 4);
		glVertexAttribPointer(0, nf, GL_FLOAT, GL_FALSE, 0, NULL);
	}
	std::vector<T>& Vtx() { return vtx; }
	void updateGPU() {	// CPU -> GPU
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(T), &vtx[0], GL_DYNAMIC_DRAW);
	}
	void Bind() { glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo); } // aktiválás
	void Draw(GPUProgram* prog, int type, vec3 color) {
		if (vtx.size() > 0) {
			prog->setUniform(color, "color");
			glBindVertexArray(vao);
			glDrawArrays(type, 0, (int)vtx.size());
		}
	}
	virtual ~Geometry() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

//---------------------------
class Texture {
//---------------------------
	unsigned int textureId = 0;
public:
#ifdef FILE_OPERATIONS
	Texture(const fs::path pathname, bool transparent = false, int sampling = GL_LINEAR) {
		if (textureId == 0) glGenTextures(1, &textureId);  				// azonosító generálás
		glBindTexture(GL_TEXTURE_2D, textureId);    // kötés
		unsigned int width, height;
		unsigned char* pixels;
		if (transparent) {
			lodepng_decode32_file(&pixels, &width, &height, pathname.string().c_str());
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					float sum = 0;
					for (int c = 0; c < 3; ++c) {
						sum += pixels[4 * (x + y * width) + c];
					}
					pixels[4 * (x + y * width) + 3] = sum / 6;
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels); // GPU-ra
		}
		else {
			lodepng_decode24_file(&pixels, &width, &height, pathname.string().c_str());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels); // GPU-ra
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling); // szûrés
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling);
		printf("%s, w: %d, h: %d\n", pathname.string().c_str(), width, height);
	}
#endif
	Texture(int width, int height) {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		// procedurális textúra elõállítása programmal
		const vec3 yellow(1, 1, 0), blue(0, 0, 1);
		std::vector<vec3> image(width * height);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			image[y * width + x] = (x & 1) ^ (y & 1) ? yellow : blue;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	Texture(int width, int height, std::vector<vec3>& image) {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); // aktiválás
		glBindTexture(GL_TEXTURE_2D, textureId); // piros nyíl
	}
	~Texture() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

enum MouseButton { MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
enum SpecialKeys { KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265 };
bool pollKey(int key);

//---------------------------
class glApp {
//---------------------------
public:
	glApp(const char * caption);
	glApp(unsigned int major, unsigned int minor,        // Kért OpenGL major.minor verzió
		  unsigned int winWidth, unsigned int winHeight, // Alkalmazói ablak felbontása
		  const char * caption);       // Megfogócsík szövege
	void refreshScreen(); // Ablak érvénytelenítése
	// Eseménykezelõk
	virtual void onInitialization() {}    // Inicializáció
	virtual void onDisplay() {}           // Ablak érvénytelen
	virtual void onKeyboard(int key) {}   // Klaviatúra gomb lenyomás
	virtual void onKeyboardUp(int key) {} // Klaviatúra gomb elenged
	// Egér gomb lenyomás/elengedés
	virtual void onMousePressed(MouseButton but, int pX, int pY) {}
	virtual void onMouseReleased(MouseButton but, int pX, int pY) {}
	// Egér mozgatás lenyomott gombbal
	virtual void onMouseMotion(int pX, int pY) {}
	// Telik az idõ
	virtual void onTimeElapsed(float startTime, float endTime) {}
};

