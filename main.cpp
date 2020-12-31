#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "Light.h"

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;
Camera camera;

Texture brickTexture;
Texture dirtTexture;

Light mainLight;

GLfloat dT { 0.0f };
GLfloat lastT{ 0.0f };

// Vertex Shader
static const char* vShader = "shaders/vertex.shader";

// Fragment Shader
static const char* fShader = "shaders/fragment.shader";

void calcAverageNormals(unsigned int* indices, unsigned int indicesCount, GLfloat* vertices, unsigned int verticesCount,
						unsigned int vLength, unsigned int normalOffset)
{
	for (auto i = 0; i < indicesCount; i += 3)
	{
		unsigned int in0 = indices[i]     * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;

		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset; in1 += normalOffset; in2 += normalOffset;
		vertices[in0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	for (auto i = 0; i < verticesCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;

		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}


}


void CreateObject() 
{
	// ������� ������, ���������� �������� � ����������� ����������.
	unsigned int indices[] =
	{
		0, 3, 1,
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	GLfloat vertices[] =
	{
	//	 x		y	  z		  u	   v	 n.x    n.y  n.z
	   -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,	 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 1.0f,	0.5f, 0.0f,	 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f,	1.0f, 0.0f,	 0.0f, 0.0f, 0.0f,
		0.0f,  1.0f, 0.0f,  0.5f, 1.0f,	 0.0f, 0.0f, 0.0f
	};

	// ������������ ������� �� �������� �������� ������ � ������ � �����������.
	calcAverageNormals(indices, 12, vertices, 32, 8, 5);

	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj1);

	Mesh* obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj2);
}

void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
	GLuint theShader = glCreateShader(shaderType);

	const GLchar* theCode[1];
	theCode[0] = shaderCode;

	GLint codeLength[1];
	codeLength[0] = strlen(shaderCode);

	glShaderSource(theShader, 1, theCode, codeLength);
	glCompileShader(theShader);

	GLint result = 0;
	GLchar eLog[1024]{ 0 };

	glLinkProgram(theShader);
	glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		glGetShaderInfoLog(theShader, sizeof(eLog), nullptr, eLog);
		printf("Error compiling the %d shader: '%s'\n", shaderType, eLog);
		return;
	}

	glAttachShader(theProgram, theShader);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

int main()
{
	GLuint uniformModel{ 0 }, uniformProjection{ 0 }, uniformView{ 0 }, 
		   uniformAmbientIntencity{ 0 }, uniformAmbientColour{ 0 }, uniformDirection{ 0 },
		   uniformDiffuseIntensity{0};

	mainWindow.initialize();	

	CreateObject();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 5.0f, 0.1f);

	brickTexture = Texture("textures/brick.png");
	brickTexture.loadTexture();
	dirtTexture = Texture("textures/dirt.png");
	dirtTexture.loadTexture();

	mainLight = Light(1.0f, 1.0f, 1.0f, 0.2f, 
					 2.0f,-1.0f, -2.0f, 1.0f);

	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(), 0.1f, 100.0f);

	// loop until window is closed
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();	// SDL_GetPerformanceCounter();
		dT = now - lastT;				// (now - lastT)*1000/SDL_GetPerformanceFrequency();
		lastT = now;

		// Handle user input events(keyboard, mouse etc.)
		glfwPollEvents();

		camera.keyControl(mainWindow.getsKeys(), dT);
		camera.mouseControl(mainWindow.get_dX(), mainWindow.get_dY());

		// Clear Window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderList[0].UseShader();
		uniformModel			= shaderList[0].GetModelLocation();
		uniformProjection		= shaderList[0].GetProjectionLocation();
		uniformView				= shaderList[0].getViewLocation();
		uniformAmbientIntencity = shaderList[0].getAmbientIntencityLocation();
		uniformAmbientColour	= shaderList[0].getAmbientColourLocation();
		uniformDirection		= shaderList[0].getDirectionLocation();
		uniformDiffuseIntensity = shaderList[0].getDiffuseIntensityLocation();

		mainLight.UseLight(uniformAmbientIntencity, uniformAmbientColour, uniformDiffuseIntensity, uniformDirection);

		glm::mat4 model;	// model matrix is full of zeroes

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.5f));	//just multiplies model matrix with a �translation matrix� and dot produc it to vec3
		model = glm::rotate(model, 60.0f * toRadians, glm::vec3(0.0f, 1.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model)); // ������� �� ����� ���� �� ������ �������� � ������, ������� �������� ��������� �� ���
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection)); 
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		brickTexture.useTexture();
		meshList[0]->RenderMesh();

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, -2.5f));	
		model = glm::rotate(model, 30.0f * toRadians, glm::vec3(0.0f, 1.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model)); 
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		dirtTexture.useTexture();
		meshList[1]->RenderMesh();


		glUseProgram(0);
		mainWindow.swapBuffers();
	}


	return 0;
}