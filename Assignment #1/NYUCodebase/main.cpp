/*

CS3113
Assignment #1

Vivian Zhao
N10586937
vz341

*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

//Include stb_image header.
//NOTE: You must define STB_IMAGE_IMPLEMENTATION in one of the files you are including it from!

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ShaderProgram.h"
#include "Matrix.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	//Use stbi_load function to load the pixel data from an image file.
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	//Generates a new OpenGL texture ID.
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	//Bind a texture to a texture target.
	glBindTexture(GL_TEXTURE_2D, retTexture);
	//Our texture target is always going to be GL_TEXTURE_2D
	//Sets the texture data of the specified texture target. Image format must be GL_RGBA for RGBA images or GL_RGB for RGB images.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	//Sets a texture parameter of the specified texture target.
	//We MUST set the texture filtering parameters GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER before the texture can be used.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//Use GL_LINEAR for linear filtering and GL_NEAREST for nearest neighbor filtering.

	//After you are done with the image data, you must free it using the stbi_image_free function.
	stbi_image_free(image);
	return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Assignment #1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup (before the loop)

	glViewport(0, 0, 640, 360);

	//Need to use a shader program that supports textures!
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint cheeseburgerTexture = LoadTexture(RESOURCE_FOLDER"Cheeseburger.png");
	GLuint friesTexture = LoadTexture(RESOURCE_FOLDER"Fries.png");
	GLuint hotdogTexture = LoadTexture(RESOURCE_FOLDER"Hotdog.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float cheeseburgerPosition = 0.0f;
	float hotdogPosition = 0.0f;

	float angle = 0.0f;

	//Keeping time.

	//In setup

	float lastFrameTicks = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		//In game loop

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//elapsed is how many seconds elapsed since last frame.
		//We will use this value to move everything in our game.

		//Basic time-based animation.

		angle += (3.14159265359f / 1280);

		//rotate matrix by angle
		//draw sprite

		//Clears the screen to the set clear color.
		glClear(GL_COLOR_BUFFER_BIT);

		//Drawing (in your game loop)

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, friesTexture);

		modelMatrix.identity();
		modelMatrix.Rotate(angle);

		program.setModelMatrix(modelMatrix);

		float friesVertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, friesVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		//Defines an array of vertex data.
		float friesTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, friesTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, cheeseburgerTexture);

		modelMatrix.identity();
		modelMatrix.Translate(cheeseburgerPosition, 0.0f, 0.0f);

		program.setModelMatrix(modelMatrix);

		float cheeseburgerVertices[] = { -1.00, 0.5, -1.50, 0.5, -1.50, 0.0, -1.50, 0.0, -1.00, 0.0, -1.00, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cheeseburgerVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		//Defines an array of vertex data.
		float cheeseburgerTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, cheeseburgerTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, hotdogTexture);

		modelMatrix.identity();
		modelMatrix.Translate(hotdogPosition, 0.0f, 0.0f);

		program.setModelMatrix(modelMatrix);

		float hotdogVertices[] = { -2.00, 0.5, -2.50, 0.5, -2.50, 0.0, -2.50, 0.0, -2.00, 0.0, -2.00, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, hotdogVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		//Defines an array of vertex data.
		float hotdogTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, hotdogTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}