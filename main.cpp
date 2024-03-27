//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>

int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048 * 2;
const unsigned int SHADOW_HEIGHT = 2048 * 2;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(
	glm::vec3(0.0f, 2.0f, 5.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.3f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;


gps::Model3D scene;
gps::Model3D lightCube;

gps::Model3D sky_day;
gps::Model3D sky_night;

gps::Model3D boat2;
gps::Model3D drone;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

GLuint fogDensityLoc;
GLfloat fogDensity;


bool is_night;
bool is_boat_moving;
bool is_forward;
bool is_floating_up;
bool is_drone_moving;
bool speed_drone;
bool choose_light;

float boat_delta_x = 0;    // for boat movement forward/backwards
float boat_delta_y = 0;    // for bouncing the boat up/down
float max_distance_boat = 2;
float drone_rotation_angle = 0.0f; 


// point light parameters
glm::vec3 pointLightPosition = glm::vec3(-12.85f, 8.13f, -16.15f);
glm::vec3 pointLightColor = glm::vec3(1.0f, 0.5f, 0.0f);
float pointConstant = 1.0f;
float pointLinear = 0.7f;
float pointQuadratic = 1.8f;

// for camera animation 
float cameraAutoMoveSpeed = 0.3f; 
float cameraRotationSpeed = 20.0f; 
bool isCameraAutoMoving = true;
float cameraAutoMoveStartTime;
float totalAnimationDuration = 10.0f; 
float maxAnimationDuration = 7.0f;

void updateBoatDelta() {
	if (is_boat_moving) {
		if (boat_delta_x >= max_distance_boat) {
			is_forward = false;
		}
		else if (boat_delta_x <= 0) {
			is_forward = true;
		}

		if (is_forward) {
			boat_delta_x += 0.01f;
		}
		else {
			boat_delta_x -= 0.01f;
		}
	}

	if (boat_delta_y >= 0.1f) {
		is_floating_up = false;
	}
	else if (boat_delta_y <= 0) {
		is_floating_up = true;
	}

	if (is_floating_up) {
		boat_delta_y += 0.005f;
	}
	else {
		boat_delta_y -= 0.005f;
	}
}

GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO	
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}

	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		if (is_night) {
			is_night = false;
		}
		else {
			is_night = true;
		}
	}
	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		is_boat_moving = !is_boat_moving;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		is_drone_moving = !is_drone_moving;
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		speed_drone = !speed_drone;
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		choose_light = !choose_light;
		//myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "choose_light"), choose_light);

	}
}

GLboolean firstMouse = true;
float lastX = 400, lastY = 300;
double yaw = -90.0f, pitch;

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	myCamera.rotate(pitch, yaw);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_F]) {
		fogDensity += 0.002f;
		if (fogDensity >= 0.3f)
			fogDensity = 0.3f;
		myCustomShader.useShaderProgram();
		glUniform1fv(fogDensityLoc, 1, &fogDensity);
	}

	if (pressedKeys[GLFW_KEY_G]) {
		fogDensity -= 0.002f;
		if (fogDensity <= 0.0f)
			fogDensity = 0.0f;
		myCustomShader.useShaderProgram();
		glUniform1fv(fogDensityLoc, 1, &fogDensity);
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	//window scaling for HiDPI displays
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	//for sRBG framebuffer
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	//for antialising
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
	scene.LoadModel("objects/scena/scene_final.obj");
	lightCube.LoadModel("objects/cube/cube.obj");

	sky_day.LoadModel("objects/skydome/sky_day2.obj");
	sky_night.LoadModel("objects/skydome/sky_night.obj");

	boat2.LoadModel("objects/boat2/boat2.obj");
	drone.LoadModel("objects/drone3/drone3.obj");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();

	depthShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
	depthShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//setting the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 15.0f, 1.0f);

	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");

	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
	//setting light color
	lightColor = glm::vec3(0.5f, 0.5f, 0.5f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	// setting the uniforms in the shader
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightPosition"), 1, glm::value_ptr(pointLightPosition));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointConstant"), pointConstant);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLinear"), pointLinear);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointQuadratic"), pointQuadratic);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(pointLightColor));

	//setting fog density
	fogDensity = 0.01f;
	fogDensityLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity");
	glUniform1fv(fogDensityLoc, 1, &fogDensity);

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix

	glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.1f, far_plane = 60.0f;
	glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;


	return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	scene.Draw(shader);


	if (is_night) {
		sky_night.Draw(shader);
	}
	else {
		sky_day.Draw(shader);
	}

	updateBoatDelta();
	model = glm::translate(model, glm::vec3(boat_delta_x, boat_delta_y, 0));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	boat2.Draw(shader);


	if (is_drone_moving) {
		if (speed_drone) {
			drone_rotation_angle += glm::radians(70.0f);
		}
		else {
			drone_rotation_angle += glm::radians(30.0f);
		}
		
	}

	model = glm::mat4(1.0f);
	glm::vec3 position(-1.0, -3.5, -1.0);
	model = glm::translate(model, position);
	model = glm::rotate(model, glm::radians(drone_rotation_angle), glm::vec3(0.0, 1.0, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	//do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	
	drone.Draw(shader);
}


void renderScene() {

	// render depth map on screen - toggled with the M key

	//render the scene to the depth buffer
	depthShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(depthShader, true);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		//glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		//screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);

	}
	else {

		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);


		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		//draw a white cube around the light

		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);

	}
}

void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}


int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	
	isCameraAutoMoving = true;
	cameraAutoMoveStartTime = glfwGetTime();

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();

		if (isCameraAutoMoving) {
			float currentTime = glfwGetTime();
			float deltaTime = currentTime - cameraAutoMoveStartTime;

			//Pentru animatia camerei la pornirea aplicatiei
			// Limitam durata animatiei in functie de maxAnimationDuration
			if (deltaTime < maxAnimationDuration) {
				
				// miscarea automata a camerei
				// efectul de animatie, valorile pot sa fie modificate in functie de preferinte
				float backwardSpeed = 0.1f;
				float rotationSpeed = glm::radians(30.0f);
				float rotationDuration = 3.0f;
				float closerStartDuration = 1.0f;

				// ACCeleram miscarea inapoi initial pentru a face animatia sa porneasca instant
				float adjustedBackwardSpeed = backwardSpeed * (1.0f + deltaTime);

				if (deltaTime < rotationDuration) {
					// Rotim la dreapta si in acelasi timp usor in spate in prima parte a animatiei
					myCamera.rotate(0.0f, rotationSpeed * deltaTime);
					myCamera.move(gps::MOVE_BACKWARD, adjustedBackwardSpeed);
				}
				else if (deltaTime < (rotationDuration + closerStartDuration)) {
					// Incepem sa schimbam directia spre stanga mai repede si continuam sa miscam camera inapoi
					float leftRotationDeltaTime = deltaTime - rotationDuration;
					float adjustedRotationSpeed = rotationSpeed * (1.0f - leftRotationDeltaTime / closerStartDuration);

					myCamera.rotate(0.0f, adjustedRotationSpeed * leftRotationDeltaTime);
					myCamera.move(gps::MOVE_BACKWARD, adjustedBackwardSpeed);
				}
				else {
					// Rotim la stanga si ne apropiem in timpul ramas
					float leftRotationDeltaTime = deltaTime - rotationDuration - closerStartDuration;

					// Incetinim miscarea inapoi in timp ce rotim la stanga
					float adjustedBackwardSpeedLeftRotation = backwardSpeed * (1.0f - leftRotationDeltaTime / (maxAnimationDuration - rotationDuration - closerStartDuration));

					myCamera.rotate(0.0f, rotationSpeed * (closerStartDuration - leftRotationDeltaTime));
					myCamera.move(gps::MOVE_BACKWARD, adjustedBackwardSpeedLeftRotation);
				}
			}
			else {
				// Miscarea automata este completa si putem finaliza animatia
				isCameraAutoMoving = false;
			}
		}

		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}


	cleanup();

	return 0;
}
