#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <queue>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Material.h"
#include "Light.h"
#include "Component.h"

#define MAX_LIGHTS 3
using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Shape> shape;
shared_ptr<Material> material;
shared_ptr<Light> light;

shared_ptr<Component> torso = make_shared<Component>();
static queue<shared_ptr<Component>> bfsQueue;
static vector<shared_ptr<Component>> bfsVector;
int currentComponent = 0;

bool keyToggles[256] = {false}; // only for English keyboards!
char uniformName[50];

int programIndex;
int programNum;
vector<shared_ptr<Program>> programs;

int materialIndex;
int materialNum;
vector<shared_ptr<Material>> materials;

int lightIndex;
int lightNum;
vector<shared_ptr<Light>> lights;

int shapeIndex;
int shapeNum;
vector<shared_ptr<Shape>> shapes;

float a = 0.05;
float f = 2;

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void char_callback(GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case GLFW_KEY_COMMA:
		{
			currentComponent += 1;
			currentComponent %= bfsVector.size();
			break;
		}
		case GLFW_KEY_PERIOD:
		{
			currentComponent -= 1;
			currentComponent = (currentComponent + bfsVector.size()) % bfsVector.size();
			break;
		}
		case GLFW_KEY_X:
		{
			bfsVector[currentComponent]->jointRotation.x += (10.0f / 180.0f * PI);
			break;
		}
		case GLFW_KEY_Y:
		{
			bfsVector[currentComponent]->jointRotation.y += (10.0f / 180.0f * PI);
			break;
		}
		case GLFW_KEY_Z:
		{
			bfsVector[currentComponent]->jointRotation.z += (10.0f / 180.0f * PI);
			break;
		}
		case GLFW_KEY_S:
		{
			programIndex = (programIndex + 1) % programNum;
			break;
		}
		case GLFW_KEY_M:
		{
			materialIndex = (materialIndex + 1) % materialNum;
			break;
		}
		case GLFW_KEY_L:
		{
			lightIndex = (lightIndex + 1) % lightNum;
			break;
		}
		case GLFW_KEY_LEFT:
		{
			lights[lightIndex]->position.x -= 0.1;
			break;
		}
		case GLFW_KEY_RIGHT:
		{
			lights[lightIndex]->position.x += 0.1;
			break;
		}
		case GLFW_KEY_UP:
		{
			lights[lightIndex]->position.y += 0.1;
			break;
		}
		case GLFW_KEY_DOWN:
		{
			lights[lightIndex]->position.y -= 0.1;
			break;
		}
		/*case GLFW_KEY_SPACE:
		{
			keyToggles[key] = !keyToggles[key];
			break;
		}*/
	}
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		char_callback(window, key);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

void initializeBFS(shared_ptr<Component> root, queue<shared_ptr<Component>>& bfsQueue, vector<shared_ptr<Component>>& bfsVector) {
	bfsQueue.push(root);
	int index = 0;
	shared_ptr<Component> tmp;
	while (!bfsQueue.empty()) {
		tmp = bfsQueue.front();
		bfsQueue.pop();
		bfsVector.push_back(tmp);
		tmp->index = index;
		index += 1;
		for (auto child : tmp->children) {
			bfsQueue.push(child);
		}
	}
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initial programs
	programIndex = 0;
	programNum = 4;
	for (int i = 0; i < programNum; ++i) {
		programs.push_back(make_shared<Program>());
	}

	prog = programs[0];
	prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "normal_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->setVerbose(false);

	prog = programs[1];
	prog->setShaderNames(RESOURCE_DIR + "blinn_phong_vert.glsl", RESOURCE_DIR + "blinn_phong_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->addUniform("ka");
	prog->addUniform("kd");
	prog->addUniform("ks");
	prog->addUniform("s");
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		sprintf(uniformName, "lights[%d].position", i);
		prog->addUniform(string(uniformName));

		sprintf(uniformName, "lights[%d].color", i);
		prog->addUniform(string(uniformName));
	}
	prog->addUniform("lightNum");
	prog->setVerbose(false);

	prog = programs[2];
	prog->setShaderNames(RESOURCE_DIR + "silhouette_vert.glsl", RESOURCE_DIR + "silhouette_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->setVerbose(false);

	prog = programs[3];
	prog->setShaderNames(RESOURCE_DIR + "cel_vert.glsl", RESOURCE_DIR + "cel_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->addUniform("ka");
	prog->addUniform("kd");
	prog->addUniform("ks");
	prog->addUniform("s");
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		sprintf(uniformName, "lights[%d].position", i);
		prog->addUniform(string(uniformName));

		sprintf(uniformName, "lights[%d].color", i);
		prog->addUniform(string(uniformName));
	}
	prog->addUniform("lightNum");
	prog->setVerbose(false);

	// Initial materials
	materialIndex = 0;
	materialNum = 3;
	for (int i = 0; i < materialNum; ++i) {
		materials.push_back(make_shared<Material>());
	}

	material = materials[0];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.8f, 0.7f, 0.7f);
	material->ks = glm::vec3(1.0f, 0.9f, 0.8f);
	material->s = 200.0f;

	material = materials[1];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.0f, 0.0f, 1.0f);
	material->ks = glm::vec3(0.0f, 1.0f, 0.0f);
	material->s = 100.0f;

	material = materials[2];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.3f, 0.3f, 0.3f);
	material->ks = glm::vec3(0.1f, 0.1f, 0.1f);
	material->s = 100.0f;

	// Initial lights
	lightNum = 2;
	for (int i = 0; i < lightNum; ++i) {
		lights.push_back(make_shared<Light>());
	}

	light = lights[0];
	light->position = glm::vec3(1.0, 1.0, 1.0);
	light->color = glm::vec3(0.8, 0.8, 0.8);

	light = lights[1];
	light->position = glm::vec3(-1.0, 1.0, 1.0);
	light->color = glm::vec3(0.2, 0.2, 0.0);

	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	// Initial shapes
	shapeNum = 2;
	for (int i = 0; i < shapeNum; ++i) {
		shapes.push_back(make_shared<Shape>());
	}
	shape = shapes[0];
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
	shape = shapes[1];
	shape->loadMesh(RESOURCE_DIR + "teapot.obj");
	shape->init();

	// hard code to build robot
	torso->jointTranslation = vec3(0.0f, 3.0f, 0.0f);
	torso->meshScale = vec3(2.0f, 4.0f, 2.0f);
	torso->meshTranslation = vec3(0.0f, -2.0f, 0.0f);
	torso->loadMesh(RESOURCE_DIR + "cube.obj");
	torso->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	torso->init();

	shared_ptr<Component> head = make_shared<Component>();
	head->meshTranslation = vec3(0.0f, 0.5f, 0.0f);
	head->loadMesh(RESOURCE_DIR + "cube.obj");
	head->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	head->init();
	torso->addChild(head);

	shared_ptr<Component> upperRightArm = make_shared<Component>();
	upperRightArm->jointTranslation = vec3(1.0f, -0.5f, 0.0f);
	upperRightArm->meshScale = vec3(3.0f, 0.8f, 0.8f);
	upperRightArm->meshTranslation = vec3(1.5f, 0.0f, 0.0f);
	upperRightArm->loadMesh(RESOURCE_DIR + "cube.obj");
	upperRightArm->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	upperRightArm->init();
	torso->addChild(upperRightArm);

	shared_ptr<Component> lowerRightArm = make_shared<Component>();
	lowerRightArm->jointTranslation = vec3(3.0f, 0.0f, 0.0f);
	lowerRightArm->meshScale = vec3(3.0f, 0.5f, 0.5f);
	lowerRightArm->meshTranslation = vec3(1.5f, 0.0f, 0.0f);
	lowerRightArm->loadMesh(RESOURCE_DIR + "cube.obj");
	lowerRightArm->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	lowerRightArm->init();
	upperRightArm->addChild(lowerRightArm);

	shared_ptr<Component> upperLeftArm = make_shared<Component>();
	upperLeftArm->jointTranslation = vec3(-1.0f, -0.5f, 0.0f);
	upperLeftArm->meshScale = vec3(3.0f, 0.8f, 0.8f);
	upperLeftArm->meshTranslation = vec3(-1.5f, 0.0f, 0.0f);
	upperLeftArm->loadMesh(RESOURCE_DIR + "cube.obj");
	upperLeftArm->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	upperLeftArm->init();
	torso->addChild(upperLeftArm);

	shared_ptr<Component> lowerLeftArm = make_shared<Component>();
	lowerLeftArm->jointTranslation = vec3(-3.0f, 0.0f, 0.0f);
	lowerLeftArm->meshScale = vec3(3.0f, 0.5f, 0.5f);
	lowerLeftArm->meshTranslation = vec3(-1.5f, 0.0f, 0.0f);
	lowerLeftArm->loadMesh(RESOURCE_DIR + "cube.obj");
	lowerLeftArm->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	lowerLeftArm->init();
	upperLeftArm->addChild(lowerLeftArm);

	shared_ptr<Component> upperRightLeg = make_shared<Component>();
	upperRightLeg->jointTranslation = vec3(0.5f, -4.0f, 0.0f);
	upperRightLeg->meshScale = vec3(0.8f, 3.0f, 0.8f);
	upperRightLeg->meshTranslation = vec3(0.0f, -1.5f, 0.0f);
	upperRightLeg->loadMesh(RESOURCE_DIR + "cube.obj");
	upperRightLeg->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	upperRightLeg->init();
	torso->addChild(upperRightLeg);

	shared_ptr<Component> lowerRightLeg = make_shared<Component>();
	lowerRightLeg->jointTranslation = vec3(0.0f, -3.0f, 0.0f);
	lowerRightLeg->meshScale = vec3(0.7f, 3.0f, 0.7f);
	lowerRightLeg->meshTranslation = vec3(0.0f, -1.5f, 0.0f);
	lowerRightLeg->loadMesh(RESOURCE_DIR + "cube.obj");
	lowerRightLeg->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	lowerRightLeg->init();
	upperRightLeg->addChild(lowerRightLeg);

	shared_ptr<Component> upperLeftLeg = make_shared<Component>();
	upperLeftLeg->jointTranslation = vec3(-0.5f, -4.0f, 0.0f);
	upperLeftLeg->meshScale = vec3(0.8f, 3.0f, 0.8f);
	upperLeftLeg->meshTranslation = vec3(0.0f, -1.5f, 0.0f);
	upperLeftLeg->loadMesh(RESOURCE_DIR + "cube.obj");
	upperLeftLeg->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	upperLeftLeg->init();
	torso->addChild(upperLeftLeg);

	shared_ptr<Component> lowerLeftLeg = make_shared<Component>();
	lowerLeftLeg->jointTranslation = vec3(0.0f, -3.0f, 0.0f);
	lowerLeftLeg->meshScale = vec3(0.7f, 3.0f, 0.7f);
	lowerLeftLeg->meshTranslation = vec3(0.0f, -1.5f, 0.0f);
	lowerLeftLeg->loadMesh(RESOURCE_DIR + "cube.obj");
	lowerLeftLeg->loadJointMesh(RESOURCE_DIR + "sphere.obj");
	lowerLeftLeg->init();
	upperLeftLeg->addChild(lowerLeftLeg);

	initializeBFS(torso, bfsQueue, bfsVector);
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();
	/*if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}*/
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	
	prog = programs[programIndex]; 
	prog->bind();

	switch (programIndex) {
		case 0:
		{
			break;
		}
		case 3:
		case 1:
		{
			material = materials[materialIndex];
			glUniform3f(prog->getUniform("ka"), material->ka.x, material->ka.y, material->ka.z);
			glUniform3f(prog->getUniform("kd"), material->kd.x, material->kd.y, material->kd.z);
			glUniform3f(prog->getUniform("ks"), material->ks.x, material->ks.y, material->ks.z);
			glUniform1f(prog->getUniform("s"), material->s);
			for (int i = 0; i < lightNum; ++i) {
				sprintf(uniformName, "lights[%d].position", i);
				glUniform3fv(prog->getUniform(string(uniformName)), 1, glm::value_ptr(lights[i]->position));
				sprintf(uniformName, "lights[%d].color", i);
				glUniform3fv(prog->getUniform(string(uniformName)), 1, glm::value_ptr(lights[i]->color));
			}
			glUniform1i(prog->getUniform("lightNum"), lightNum);
			break;
		}
		case 2:
		{
			break;
		}
	}
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

	MV->pushMatrix();
	MV->translate(-0.5f, -0.5f, 0.0f);
	MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
	MV->scale(0.2f, 0.2f, 0.2f);
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	shapes[0]->draw(prog);
	MV->popMatrix();

	MV->pushMatrix();
	MV->translate(0.5f, -0.3f, 0.0f);
	glm::mat4 S(1.0f);
	S[0][1] = 0.5f * cos(t);
	MV->multMatrix(S);
	MV->rotate(M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
	MV->scale(0.2f, 0.2f, 0.2f);
	glm::mat4 newMV = glm::transpose(glm::inverse(MV->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	shapes[1]->draw(prog);
	MV->popMatrix();

	MV->scale(0.1, 0.1, 0.1);

	float s = 1 + a / 2.0f + a / 2.0f * sin(2 * PI * f * t);
	bfsVector[currentComponent]->meshScale *= s;

	torso->draw(prog, MV);

	prog->unbind();

	bfsVector[currentComponent]->meshScale /= s;
	
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	// glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
