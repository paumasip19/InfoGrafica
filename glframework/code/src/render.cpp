#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

bool ex1 = true;

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
void setupAxis();
void cleanupAxis();
void drawAxis();
}
////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 50.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if(height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if(RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch(ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	} else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name="") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

////////////////////////////////////////////////// AXIS
namespace Axis {
GLuint AxisVao;
GLuint AxisVbo[3];
GLuint AxisShader[2];
GLuint AxisProgram;

float AxisVerts[] = {
	0.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 0.0, 1.0
};
float AxisColors[] = {
	1.0, 0.0, 0.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0
};
GLubyte AxisIdx[] = {
	0, 1,
	2, 3,
	4, 5
};
const char* Axis_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
const char* Axis_fragShader =
"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

void setupAxis() {
	glGenVertexArrays(1, &AxisVao);
	glBindVertexArray(AxisVao);
	glGenBuffers(3, AxisVbo);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
	AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

	AxisProgram = glCreateProgram();
	glAttachShader(AxisProgram, AxisShader[0]);
	glAttachShader(AxisProgram, AxisShader[1]);
	glBindAttribLocation(AxisProgram, 0, "in_Position");
	glBindAttribLocation(AxisProgram, 1, "in_Color");
	linkProgram(AxisProgram);
}
void cleanupAxis() {
	glDeleteBuffers(3, AxisVbo);
	glDeleteVertexArrays(1, &AxisVao);

	glDeleteProgram(AxisProgram);
	glDeleteShader(AxisShader[0]);
	glDeleteShader(AxisShader[1]);
}
void drawAxis() {
	glBindVertexArray(AxisVao);
	glUseProgram(AxisProgram);
	glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

	glUseProgram(0);
	glBindVertexArray(0);
}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
GLuint cubeVao;
GLuint cubeVbo[3];
GLuint cubeShaders[2];
GLuint cubeProgram;
glm::mat4 objMat = glm::mat4(1.f);

extern const float halfW = 0.5f;
int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

					   //   4---------7
					   //  /|        /|
					   // / |       / |
					   //5---------6  |
					   //|  0------|--3
					   //| /       | /
					   //|/        |/
					   //1---------2
glm::vec3 verts[] = {
	glm::vec3(-halfW, -halfW, -halfW),
	glm::vec3(-halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW, -halfW),
	glm::vec3(-halfW,  halfW, -halfW),
	glm::vec3(-halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW, -halfW)
};
glm::vec3 norms[] = {
	glm::vec3(0.f, -1.f,  0.f),
	glm::vec3(0.f,  1.f,  0.f),
	glm::vec3(-1.f,  0.f,  0.f),
	glm::vec3(1.f,  0.f,  0.f),
	glm::vec3(0.f,  0.f, -1.f),
	glm::vec3(0.f,  0.f,  1.f)
};

glm::vec3 cubeVerts[] = {
	verts[1], verts[0], verts[2], verts[3],
	verts[5], verts[6], verts[4], verts[7],
	verts[1], verts[5], verts[0], verts[4],
	verts[2], verts[3], verts[6], verts[7],
	verts[0], verts[4], verts[3], verts[7],
	verts[1], verts[2], verts[5], verts[6]
};
glm::vec3 cubeNorms[] = {
	norms[0], norms[0], norms[0], norms[0],
	norms[1], norms[1], norms[1], norms[1],
	norms[2], norms[2], norms[2], norms[2],
	norms[3], norms[3], norms[3], norms[3],
	norms[4], norms[4], norms[4], norms[4],
	norms[5], norms[5], norms[5], norms[5]
};
GLubyte cubeIdx[] = {
	0, 1, 2, 3, UCHAR_MAX,
	4, 5, 6, 7, UCHAR_MAX,
	8, 9, 10, 11, UCHAR_MAX,
	12, 13, 14, 15, UCHAR_MAX,
	16, 17, 18, 19, UCHAR_MAX,
	20, 21, 22, 23, UCHAR_MAX
};

const char* cube_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
const char* cube_fragShader =
"#version 330\n\
in vec4 vert_Normal;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 color;\n\
void main() {\n\
	out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
}";
void setupCube() {
	glGenVertexArrays(1, &cubeVao);
	glBindVertexArray(cubeVao);
	glGenBuffers(3, cubeVbo);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glPrimitiveRestartIndex(UCHAR_MAX);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
	cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

	cubeProgram = glCreateProgram();
	glAttachShader(cubeProgram, cubeShaders[0]);
	glAttachShader(cubeProgram, cubeShaders[1]);
	glBindAttribLocation(cubeProgram, 0, "in_Position");
	glBindAttribLocation(cubeProgram, 1, "in_Normal");
	linkProgram(cubeProgram);
}
void cleanupCube() {
	glDeleteBuffers(3, cubeVbo);
	glDeleteVertexArrays(1, &cubeVao);

	glDeleteProgram(cubeProgram);
	glDeleteShader(cubeShaders[0]);
	glDeleteShader(cubeShaders[1]);
}
void updateCube(const glm::mat4& transform) {
	objMat = transform;
}
void drawCube() {
	glEnable(GL_PRIMITIVE_RESTART);
	glBindVertexArray(cubeVao);
	glUseProgram(cubeProgram);
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
	glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
	glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

	glUseProgram(0);
	glBindVertexArray(0);
	glDisable(GL_PRIMITIVE_RESTART);
}
}

/*namespace MyGeomShader {
	GLuint MyGeomShaderProgram;
	GLuint MyGeomShaderVAO;
	GLuint MyGeomShaderVBO;
	GLuint MyGeomShaders[3];
	GLuint vertex_shader;
	GLuint geometry_shader;
	GLuint fragment_shader;

	static const char * vertex_shader_source =
	{
	"#version 330\n\
			\n\
			void main() {\n\
			const vec4 vertices[3] = vec4[3](vec4( 0.25, -0.25, 0.5, 1.0),\n\
			vec4(0.25, 0.25, 0.5, 1.0),\n\
			vec4( -0.25, -0.25, 0.5, 1.0));\n\
			gl_Position = vertices[gl_VertexID];\n\
			}"
	};

	static const char * geom_shader_source =
	{ "#version 330\n\
		layout(triangles) in;\n\
		layout(triangle_strip, max_vertices = 3)\n\
		out; \n\
		void main()\n\
		{\n\
		const vec4 vertices[3] = vec4[3](vec4(0.25, -0.25, 0.5, 1.0),\n\
		vec4(0.25, 0.25, 0.5, 1.0),\n\
		vec4(-0.25, -0.25, 0.5, 1.0)); \n\
		for (int i = 0; i < 3; i++)\n\
		{\n\
		gl_Position = vertices[i] + gl_in[0].gl_Position; \n\
		EmitVertex(); \n\
		}\n\
		EndPrimitive(); \n\
		}" };

	static const char * fragment_shader_source =
	{
	"#version 330\n\
		\n\
		out vec4 color;\n\
		\n\
		void main() {\n\
		color = vec4(0.0,0.8,1.0,1.0);\n\
		}"
	};

	void setupGeometryShader()
	{
		glGenVertexArrays(1, &MyGeomShaderVAO);
		glBindVertexArray(MyGeomShaderVAO);
		glGenBuffers(3, &MyGeomShaderVBO);

		MyGeomShaders[0] = compileShader(vertex_shader_source, GL_VERTEX_SHADER, "vertex_shader");
		MyGeomShaders[1] = compileShader(geom_shader_source, GL_GEOMETRY_SHADER, "geometry_shader");
		MyGeomShaders[2] = compileShader(fragment_shader_source, GL_FRAGMENT_SHADER, "fragment_shader");

		glAttachShader(MyGeomShaderProgram, vertex_shader);
		glAttachShader(MyGeomShaderProgram, geometry_shader);
		glAttachShader(MyGeomShaderProgram, fragment_shader);
		glLinkProgram(MyGeomShaderProgram);
	}

	void cleanupGeometryShader()
	{
		glDeleteVertexArrays(1, &MyGeomShaderVAO);
		glDeleteProgram(MyGeomShaderProgram);

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	void drawGeometryShader()
	{
		glUseProgram(MyGeomShaderProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}*/

/////////////////////////////////////////////////

namespace MyGeomShader {
	GLuint myRenderProgram;
	GLuint myVAO;
	void myCleanupCode() {
		glDeleteVertexArrays(1, &myVAO);
		glDeleteProgram(myRenderProgram);

	}
	GLuint myShaderCompile(void) {
		static const GLchar * vertex_shader_source[] =
		{
		"#version 330\n\
		\n\
		void main() {\n\
		const vec4 vertices[3] = vec4[3](vec4( 0.25, -0.25, 0.5, 1.0),\n\
		vec4(0.25, 0.25, 0.5, 1.0),\n\
		vec4( -0.25, -0.25, 0.5, 1.0));\n\
		gl_Position = vertices[gl_VertexID];\n\
		}"
		};

		static const GLchar * geom_shader_source[] =
		{ "#version 330\n\
			layout(triangles) in;\n\
			layout(triangle_strip, max_vertices = 3) out; \n\
			void main()\n\
			{\n\
			const vec4 vertices[3] = vec4[3](vec4(0.25, -0.25, 0.5, 1.0),\n\
			vec4(0.25, 0.25, 0.5, 1.0),\n\
			vec4(-0.25, -0.25, 0.5, 1.0)); \n\
			for (int i = 0; i < 3; i++)\n\
			{\n\
			gl_Position = vertices[i] + gl_in[0].gl_Position; \n\
			EmitVertex(); \n\
			}\n\
			EndPrimitive(); \n\
			}" 
		};


		static const GLchar * fragment_shader_source[] =
		{
		"#version 330\n\
		\n\
		out vec4 color;\n\
		\n\
		void main() {\n\
		color = vec4(0.0,0.8,1.0,1.0);\n\
		}"
		};

		GLuint vertex_shader;
		GLuint geom_shader;
		GLuint fragment_shader;
		GLuint program;
		vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, vertex_shader_source, NULL);
		glCompileShader(vertex_shader);
		geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geom_shader, 1, geom_shader_source, NULL);
		glCompileShader(geom_shader);
		fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);
		glCompileShader(fragment_shader);
		program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, geom_shader);
		glAttachShader(program, fragment_shader);
		glLinkProgram(program);
		
		return program;
	}
	void myInitCode(void) {
		myRenderProgram = myShaderCompile();
		glGenVertexArrays(1, &myVAO);
		glBindVertexArray(myVAO);
	}
	void myRenderCode(double currentTime) {
		glUseProgram(myRenderProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

namespace Geometry2 {
	GLuint cubeVao;
	GLuint cubeVbo[3];
	GLuint cubeShaders[2];
	GLuint cubeProgram;
	GLuint geom_shader;

	glm::mat4 objMat = glm::mat4(1.f);

	extern const float halfW = 0.5f;
	int numVerts = 20;//24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

						   //   4---------7
						   //  /|        /|
						   // / |       / |
						   //5---------6  |
						   //|  0------|--3
						   //| /       | /
						   //|/        |/
						   //1---------2
	glm::vec3 verts[] = {
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5),
		glm::vec3(rand() % 10 - 5, rand() % 10, rand() % 10 - 5)
	};
	glm::vec3 norms[] = {
		glm::vec3(0.f, -1.f,  0.f),
		glm::vec3(0.f,  1.f,  0.f),
		glm::vec3(-1.f,  0.f,  0.f),
		glm::vec3(1.f,  0.f,  0.f),
		glm::vec3(0.f,  0.f, -1.f),
		glm::vec3(0.f,  0.f,  1.f)
	};

	glm::vec3 cubeVerts[] = {
		verts[0], verts[1], verts[2], verts[3], 
		verts[4], verts[5], verts[6], verts[7], 
		verts[8], verts[9], verts[10], verts[11], 
		verts[12], verts[13], verts[14], verts[15], 
		verts[16], verts[17], verts[18], verts[19]
	};
	glm::vec3 cubeNorms[] = {
		norms[0], norms[0], norms[0], norms[0],
		norms[1], norms[1], norms[1], norms[1],
		norms[2], norms[2], norms[2], norms[2],
		norms[3], norms[3], norms[3], norms[3],
		norms[4], norms[4], norms[4], norms[4],
		norms[5], norms[5], norms[5], norms[5]
	};
	GLubyte cubeIdx[] = {
		0, 1, 2, 3, 4,
		5, 6, 7, 8, 9,
		10, 11, 12, 13, 
		14, 15, 16, 17, 18, 19
	};

	const char* cube_vertShader =
		"#version 330\n\
		\n\
		in vec3 in_Position;\n\
		in vec3 in_Normal;\n\
		out vec4 vert_Normal;\n\
		uniform mat4 objMat;\n\
		uniform mat4 mv_Mat;\n\
		uniform mat4 mvpMat;\n\
		void main() {\n\
		gl_Position = vec4(in_Position, 1.0);\n\
		vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
		}";

	const char* cube_fragShader =
		"#version 330\n\
		\n\
		out vec4 color;\n\
		flat in int isHexagon;\n\
		\n\
		void main() {\n\
		if(isHexagon == 0) { color = vec4(1.0f, 0.0f, 0.0f, 1.0f); }\n\
		else { color = vec4(0.0f, 1.0f, 0.0f, 1.0f);}\n\
		}";


	static const GLchar * geom_shader_source[] =
	{ "#version 330\n\
			layout(points) in;\n\
			layout(triangle_strip, max_vertices = 72) out; \n\
			uniform mat4 mvpMat;\n\
			flat out int isHexagon;\n\
			void main()\n\
			{\n\
			vec4 point1 = vec4(1.0, 0.50, 0.0, 0.0);//RightQuadTop\n\
			vec4 point2 = vec4(1.0, -0.50, 0.0, 0.0);//RightQuadBot\n\
			vec4 point21 = vec4(1.0, 0.0, 0.50, 0.0);//RightQuadFront\n\
			vec4 point22 = vec4(1.0, 0.0, -0.50, 0.0);//RightQuadBack\n\
			vec4 point3 = vec4(-1.0, 0.50, 0.0, 0.0);//LeftQuadTop\n\
			vec4 point4 = vec4(-1.0, -0.50, 0.0, 0.0);//LeftQuadBot\n\
			vec4 point23 = vec4(-1.0, 0.0, 0.50, 0.0);//LeftQuadFront\n\
			vec4 point24 = vec4(-1.0, 0.0, -0.50, 0.0);//LeftQuadBack\n\
			vec4 point5 = vec4(0.0, 1.0, 0.50, 0.0);//TopQuadFront\n\
			vec4 point6 = vec4(0.0, 1.0, -0.50, 0.0);//TopQuadBack\n\
			vec4 point13 = vec4(0.50, 1.0, 0.0, 0.0);//TopQuadRight\n\
			vec4 point14 = vec4(-0.50, 1.0, 0.0, 0.0);//TopQuadLeft\n\
			vec4 point7 = vec4(0.0, -1.0, 0.50, 0.0);//BotQuadFront\n\
			vec4 point8 = vec4(0.0, -1.0, -0.50, 0.0);//BotQuadBack\n\
			vec4 point15 = vec4(0.50, -1.0, 0.0, 0.0);//BotQuadRight\n\
			vec4 point16 = vec4(-0.50, -1.0, 0.0, 0.0);//BotQuadLeft\n\
			vec4 point9 = vec4(0.50, 0.0, 1.0, 0.0);//FrontQuadRight\n\
			vec4 point10 = vec4(-0.50, 0.0, 1.0, 0.0);//FrontQuadLeft\n\
			vec4 point17 = vec4(0.0, 0.50, 1.0, 0.0);//FrontQuadTop\n\
			vec4 point18 = vec4(0.0, -0.50, 1.0, 0.0);//FrontQuadBot\n\
			vec4 point11 = vec4(0.50, 0.0, -1.0, 0.0);//BackQuadRight\n\
			vec4 point12 = vec4(-0.50, 0.0, -1.0, 0.0);//BackQuadLeft\n\
			vec4 point19 = vec4(0.0, 0.50, -1.0, 0.0);//BackQuadTop\n\
			vec4 point20 = vec4(0.0, -0.50, -1.0, 0.0);//BackQuadBot\n\
			for(int i = 0; i < 20; i++){\n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point8); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point16); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point20); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point4); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point12); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point24); \n\
            EmitVertex(); \n\
            EndPrimitive();\n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point15); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point8); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point2); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point20); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point22); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point11); \n\
            EmitVertex(); \n\
            EndPrimitive();\n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point12); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point24); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point19); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point3); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point6); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point14); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point22); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point11); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point1); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point19); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point13); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point6); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point16); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point7); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point4); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point18); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point23); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point10); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point7); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point15); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point18); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point2); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point9); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point21); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point23); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point10); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point3); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point17); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point14); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point5); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 0;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point9); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point21); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point17); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point1); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point5); \n\
            EmitVertex(); \n\
			isHexagon = 0;\n\
            gl_Position = mvpMat * (gl_in[i].gl_Position + point13); \n\
            EmitVertex(); \n\
            EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point20); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point12); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point11); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point19); \n\
			EmitVertex(); \n\
			EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point10); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point18); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point17); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point9); \n\
			EmitVertex(); \n\
			EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point4); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point23); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point24); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point3); \n\
			EmitVertex(); \n\
			EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point13); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point6); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point5); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point14); \n\
			EmitVertex(); \n\
			EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point21); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point2); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point1); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point22); \n\
			EmitVertex(); \n\
			EndPrimitive(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point16); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point8); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point7); \n\
			EmitVertex(); \n\
			isHexagon = 1;\n\
			gl_Position = mvpMat * (gl_in[i].gl_Position + point15); \n\
			EmitVertex(); \n\
			EndPrimitive();}\n\
			}"
	};

	void move(int dt)
	{
		verts[0] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[1] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[2] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[3] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[4] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[5] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[6] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[7] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[8] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[9] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[10] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[11] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[12] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[13] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[14] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[15] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[16] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[17] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[18] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
		verts[19] = glm::vec3(rand() % 10 - dt, rand() % 10, rand() % 10 - dt);
	}

	void setupCube() {
		glGenVertexArrays(1, &cubeVao);
		glBindVertexArray(cubeVao);
		glGenBuffers(3, cubeVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		geom_shader =
			glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geom_shader, 1,
			geom_shader_source, NULL);
		glCompileShader(geom_shader);


		glPrimitiveRestartIndex(UCHAR_MAX);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		glAttachShader(cubeProgram, geom_shader);
		glBindAttribLocation(cubeProgram, 0, "in_Position");
		glBindAttribLocation(cubeProgram, 1, "in_Normal");
		linkProgram(cubeProgram);
	}
	void cleanupCube() {
		glDeleteBuffers(3, cubeVbo);
		glDeleteVertexArrays(1, &cubeVao);

		glDeleteProgram(cubeProgram);
		glDeleteShader(cubeShaders[0]);
		glDeleteShader(cubeShaders[1]);
	}
	void updateCube(const glm::mat4& transform) {
		objMat = transform;
	}
	void drawCube(float dt) {
		glEnable(GL_PRIMITIVE_RESTART);
		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glDrawArrays(GL_POINTS, 0, numVerts);

		move(dt);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}


void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry

	Axis::setupAxis();
	//Cube::setupCube();
	//MyGeomShader::myInitCode();
	Geometry2::setupCube();
	/////////////////////////////////////////////////////TODO
	// Do your init code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

void GLcleanup() {
	Axis::cleanupAxis();
	//Cube::cleanupCube();
	//MyGeomShader::myCleanupCode();
	Geometry2::cleanupCube();
	/////////////////////////////////////////////////////TODO
	// Do your cleanup code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	static float accum = 0.f;
	accum += dt;
	if (accum > glm::two_pi<float>())
	{
		accum = 0.f;
	}


	Axis::drawAxis();
	//Cube::drawCube();
	//MyGeomShader::myRenderCode(dt);
	if (ex1)Geometry2::drawCube(accum);
	/////////////////////////////////////////////////////TODO
	// Do your render code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////

	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (ImGui::Button("Exercice 1", ImVec2(150, 20)))
		{
			ex1 = !ex1;
		}
	}
	// .........................

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}