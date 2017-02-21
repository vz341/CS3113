/*

CS3113
Assignment #2
- Make PONG!
- Doesn't need to keep score.
- But it must detect player wins.
- Can use images or untextured polygons.
- Can use keyboard, mouse or joystick input.

vz341

*/

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Paddle {
public:
	Paddle(float left, float right, float top, float bottom) : left(left), right(right), top(top), bottom(bottom) {}

	float left;
	float right;
	float top;
	float bottom;
};

class Ball {
public:
	Ball(float xPosition, float yPosition, float sd, float an, float xDirection, float yDirection) : x_position(xPosition), y_position(yPosition), speed(sd), acceleration(an), x_direction(xDirection), y_direction(yDirection) {}

	Ball() {}

	float x_position = 0.0f;
	float y_position = 0.0f;
	float speed = 0.5f;
	float acceleration = 10.0f;
	float x_direction = (float)(rand() % 4 + 1);
	float y_direction = (float)(rand() % 10 - 5);

	void reset() {
		x_position = 0.0f;
		y_direction = 0.0f;
		speed = 0.5f;
		acceleration = 10.0f;
		x_direction = (float)(rand() % 4 + 1);
		y_direction = (float)(rand() % 10 - 5);
	}

	void move(float elapsed) {
		x_position += (speed * x_direction * elapsed);
		y_position += (speed * y_direction * elapsed);
	}
};

GLuint LoadTexture(const char *filePath) {
	SDL_Surface* surface = IMG_Load(filePath);

	//Generates a new OpenGL texture ID.
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	//Bind a texture to a texture target.
	glBindTexture(GL_TEXTURE_2D, retTexture);
	//Our texture target is always going to be GL_TEXTURE_2D
	//Sets the texture data of the specified texture target. Image format must be GL_RGBA for RGBA images or GL_RGB for RGB images.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	//Sets a texture parameter of the specified texture target.
	//We MUST set the texture filtering parameters GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER before the texture can be used.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//Use GL_LINEAR for linear filtering and GL_NEAREST for nearest neighbor filtering.

	SDL_FreeSurface(surface);
	
	return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Assignment #2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup (before the loop)

	glViewport(0, 0, 640, 360);

	//Need to use a shader program that supports textures!
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint white = LoadTexture(RESOURCE_FOLDER"White.png");

	Matrix projectionMatrix;
	Matrix leftPaddleMatrix;
	Matrix rightPaddleMatrix;
	Matrix ballMatrix;
	Matrix viewMatrix;

	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Paddle leftPaddle(-3.5f, -3.4f, 0.5f, -0.5f);
	Paddle rightPaddle(3.4f, 3.5f, 0.5f, -0.5f);
	Ball ball = Ball();

	//In setup
	float lastFrameTicks = 0.0f;

	bool startGame = false;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && !startGame) {
					startGame = true;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_W && leftPaddle.top < 2.0f) {
					leftPaddle.top += 0.3f;
					leftPaddle.bottom += 0.3f;
					leftPaddleMatrix.Translate(0.0f, 0.3f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_S && leftPaddle.bottom > -2.0f) {
					leftPaddle.top -= 0.3f;
					leftPaddle.bottom -= 0.3f;
					leftPaddleMatrix.Translate(0.0f, -0.3f, 0.0f);
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_UP && rightPaddle.top < 2.0f) {
					rightPaddle.top += 0.3f;
					rightPaddle.bottom += 0.3f;
					rightPaddleMatrix.Translate(0.0f, 0.3f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && rightPaddle.bottom > -2.0f) {
					rightPaddle.top -= 0.3f;
					rightPaddle.bottom -= 0.3f;
					rightPaddleMatrix.Translate(0.0f, -0.3f, 0.0f);
				}
			}
		}

		//Clears the screen to the set clear color.
		glClear(GL_COLOR_BUFFER_BIT);

		//Drawing (in your game loop)

		program.setModelMatrix(leftPaddleMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		float texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

		//Defines an array of vertex data.
		float leftPaddleTexCoords[] = { -3.5f, -0.5f, -3.4f, -0.5f, -3.4f, 0.5f, -3.4f, 0.5f, -3.5f, 0.5f, -3.5f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftPaddleTexCoords);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.setModelMatrix(rightPaddleMatrix);

		//Defines an array of vertex data.
		float rightPaddleTexCoords[] = { 3.4f, -0.5f, 3.5f, -0.5f, 3.5f, 0.5f, 3.5f, 0.5f, 3.4f, 0.5f, 3.4f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rightPaddleTexCoords);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.setModelMatrix(ballMatrix);

		//Defines an array of vertex data.
		float ballTexCoords[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 0.1f, 0.1f, -0.1f, 0.1f, -0.1f, -0.1f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballTexCoords);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//In game loop

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//elapsed is how many seconds elapsed since last frame.
		//We will use this value to move everything in our game.

		if (startGame) {
			if (ball.x_position <= leftPaddle.right && ball.y_position <= leftPaddle.top && ball.y_position >= leftPaddle.bottom || ball.x_position >= rightPaddle.left && ball.y_position <= rightPaddle.top && ball.y_position >= rightPaddle.bottom) {
				ball.x_direction *= -1;
				ball.speed += ball.acceleration * elapsed;
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.x_direction * elapsed), (ball.speed * ball.y_direction * elapsed), 0.0f);
			}
			else if (ball.x_position >= rightPaddle.right) {
				startGame = false;
				ballMatrix.Translate(-ball.x_position, -ball.y_position, 0.0f);
				ball.reset();
				std::cout << "Winner: Left Side!\n";
			}
			else if (ball.x_position <= leftPaddle.left) {
				startGame = false;
				ballMatrix.Translate(-ball.x_position, -ball.y_position, 0.0f);
				ball.reset();
				std::cout << "Winner: Right Side!\n";
			}
			else if (ball.y_position + 0.1f >= 2.25f || ball.y_position - 0.1f <= -2.25f) {
				ball.y_direction *= -1;
				ball.speed += ball.acceleration * elapsed;
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.x_direction * elapsed), (ball.speed * ball.y_direction * elapsed), 0.0f);
			}
			else {
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.x_direction * elapsed), (ball.speed * ball.y_direction * elapsed), 0.0f);
			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}