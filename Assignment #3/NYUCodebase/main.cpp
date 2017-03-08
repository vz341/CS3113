/*

CS3113

Assignment #3

- Make Space Invaders
- It must have two states: TITLE SCREEN, GAME
- It must display text
- It must use sprite sheets
- You can use any graphics you want (it doesn't have to be in space! :)

vz341

*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <vector>

#include "ShaderProgram.h"
#include "Matrix.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint fontTexture;
GLuint spriteSheetTexture;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

class Entity;

//Defines an array of vertex data.

float texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

bool start = true;

enum Type { PLAYER, ENEMY };

//Keeping time.

//In setup

float lastFrameTicks = 0.0f;
float elapsed;
float lastPlayerFire = 0.0f;
float lastEnemyFire = 0.0f;

bool leftMovement = false;
bool rightMovement = false;
bool projectileMovement = false;

int state;

ShaderProgram* program;

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float textureSize = 1.0 / 16.0f;

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		
		float x_texture = (float)(((int)text[i]) % 16) / 16.0f;
		float y_texture = (float)(((int)text[i]) / 16) / 16.0f;
		
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});

		texCoordData.insert(texCoordData.end(), {
			x_texture, y_texture,
			x_texture, y_texture + textureSize,
			x_texture + textureSize, y_texture,
			x_texture + textureSize, y_texture + textureSize,
			x_texture + textureSize, y_texture,
			x_texture, y_texture + textureSize,
		});
	}

	glUseProgram(program->programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	//Bind a texture to a texture target.
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

	//draw this data (use the .data() method of std::vector to get pointer to data)
}

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

class Entity {
public:
	float position[2];
	float boundaries[4];
	float speed[2];

	Matrix entityMatrix;
	
	float u;
	float v;
	float width;
	float height;
	float size = 1.0f;
	
	Type type;

	Entity() {}

	Entity(float x_position, float y_position, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float x_direction, float y_direction) {
		position[0] = x_position;
		position[1] = y_position;
		
		speed[0] = x_direction;
		speed[1] = y_direction;
		
		entityMatrix.identity();
		entityMatrix.Translate(x_position, y_position, 0);
		
		boundaries[0] = y_position + 0.05f * size;
		boundaries[1] = y_position - 0.05f * size;
		boundaries[2] = x_position - 0.05f * size;
		boundaries[3] = x_position + 0.05f * size;

		u = spriteU;
		v = spriteV;

		width = spriteWidth;
		height = spriteHeight;
	}

	void Draw() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);

		program->setModelMatrix(entityMatrix);

		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		
		float x_texture = u;
		float y_texture = v;
		
		vertexData.insert(vertexData.end(), {
			(-0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
		});

		texCoordData.insert(texCoordData.end(), {
			x_texture, y_texture,
			x_texture, y_texture + height,
			x_texture + width, y_texture,
			x_texture + width, y_texture + height,
			x_texture + width, y_texture,
			x_texture, y_texture + height,
		});

		glUseProgram(program->programID);

		//Enabling blending

		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Bind a texture to a texture target.
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
};

void RenderMainMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-0.4f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "SPACE", 0.2f, 0.0001f);
	modelMatrix.Translate(-0.3f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "INVADERS", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.1f, 0.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "GAMEPLAY INSTRUCTIONS:", 0.2f, 0.0001f);
	modelMatrix.Translate(1.6f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS:", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.0f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "[ < ] = LEFT KEY TO MOVE LEFT", 0.2f, 0.0001f);
	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "[ > ] = RIGHT KEY TO MOVE RIGHT", 0.2f, 0.0001f);
	modelMatrix.Translate(0.0f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "[   ] = SPACE BUTTON TO SHOOT", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.3f, -2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS SPACE TO START NOW", 0.2f, 0.0001f);
}

Entity player;
std::vector<Entity> enemies;
std::vector<Entity> playerLasers;
std::vector<Entity> enemyLasers;

void UpdateMainMenu(float elapsed) {
}

void RenderGameLevel() {
	player.Draw();
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Draw();
	}
	for (size_t i = 0; i < playerLasers.size(); i++) {
		playerLasers[i].Draw();
	}
	for (size_t i = 0; i < enemyLasers.size(); i++) {
		enemyLasers[i].Draw();
	}
}

void UpdateGameLevel(float elapsed) {
	if (leftMovement) {
		player.position[0] -= player.speed[0] * elapsed;
		player.boundaries[2] -= player.speed[0] * elapsed;
		player.boundaries[3] -= player.speed[0] * elapsed;
	}
	else if (rightMovement) {
		player.position[0] += player.speed[0] * elapsed;
		player.boundaries[2] += player.speed[0] * elapsed;
		player.boundaries[3] += player.speed[0] * elapsed;
	}

	if (projectileMovement) {
		if (lastPlayerFire > 0.5f) {
			lastPlayerFire = 0;
			playerLasers.push_back(Entity(player.position[0], player.position[1], 858.0f / 1024.0f, 230.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0, 4.0f));
		}
	}

	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].position[1] -= enemies[i].speed[1] * elapsed;
		enemies[i].boundaries[0] -= enemies[i].speed[1] * elapsed;
		enemies[i].boundaries[1] -= enemies[i].speed[1] * elapsed;

		enemies[i].position[0] += enemies[i].speed[0] * elapsed;
		enemies[i].boundaries[2] += enemies[i].speed[0] * elapsed;
		enemies[i].boundaries[3] += enemies[i].speed[0] * elapsed;

		if ((enemies[i].boundaries[3] > 3.3f && enemies[i].speed[0] > 0) || (enemies[i].boundaries[2] < -3.3f && enemies[i].speed[0] < 0)) {
			for (size_t i = 0; i < enemies.size(); i++) {
				enemies[i].speed[0] = -enemies[i].speed[0];
			}
		}

		if (enemies[i].boundaries[1] < player.boundaries[0] && enemies[i].boundaries[0] > player.boundaries[1] && enemies[i].boundaries[2] < player.boundaries[3] && enemies[i].boundaries[3] > player.boundaries[2]) {
			start = false;
		}
	}

	std::vector<int> removePlayerLaserIndex;

	for (size_t i = 0; i < playerLasers.size(); i++) {
		playerLasers[i].position[1] += playerLasers[i].speed[1] * elapsed;
		playerLasers[i].boundaries[0] += playerLasers[i].speed[1] * elapsed;
		playerLasers[i].boundaries[1] += playerLasers[i].speed[1] * elapsed;

		for (size_t j = 0; j < enemies.size(); j++) {
			if (enemies[j].boundaries[1] < playerLasers[i].boundaries[0] && enemies[j].boundaries[0] > playerLasers[i].boundaries[1] && enemies[j].boundaries[2] < playerLasers[i].boundaries[3] && enemies[j].boundaries[3] > playerLasers[i].boundaries[2]) {
				enemies.erase(enemies.begin() + j);
				removePlayerLaserIndex.push_back(i);
			}
		}
	}

	for (int i = 0; i < removePlayerLaserIndex.size(); i++) {
		playerLasers.erase(playerLasers.begin() + removePlayerLaserIndex[i] - i);
	}

	if (lastEnemyFire > 0.4f) {
		lastEnemyFire = 0;
		int shooter = rand() % enemies.size();
		enemyLasers.push_back(Entity(enemies[shooter].position[0], enemies[shooter].position[1], 854.0f / 1024.0f, 639.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0, -2.0f));
	}

	for (size_t i = 0; i < enemyLasers.size(); i++) {
		enemyLasers[i].position[1] += enemyLasers[i].speed[1] * elapsed;
		enemyLasers[i].boundaries[0] += enemyLasers[i].speed[1] * elapsed;
		enemyLasers[i].boundaries[1] += enemyLasers[i].speed[1] * elapsed;
		if (enemyLasers[i].boundaries[1] < player.boundaries[0] && enemyLasers[i].boundaries[0] > player.boundaries[1] && enemyLasers[i].boundaries[2] < player.boundaries[3] && enemyLasers[i].boundaries[3] > player.boundaries[2]) {
			start = false;
		}
	}

	if (enemies.size() == 0) {
		start = false;
	}
}

void Render() {
	//Clears the screen to the set clear color.
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	}

	SDL_GL_SwapWindow(displayWindow);
}

void Update(float elapsed) {
	switch (state) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(elapsed);
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Assignment #3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup (before the loop)

	glViewport(0, 0, 640, 360);

	//Need to use a shader program that supports textures!
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	fontTexture = LoadTexture("Font.png");
	spriteSheetTexture = LoadTexture("Sprite Sheet.png");

	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.25f, 2.25f, -1.0f, 1.0f);

	program->setProjectionMatrix(projectionMatrix);
	program->setModelMatrix(modelMatrix);
	program->setViewMatrix(viewMatrix);

	player = Entity(0.0f, -2.0f, 224.0f / 1024.0f, 832.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 3.0f, 0);
	for (int i = 0; i < 30; i++) {
		enemies.push_back(Entity(-2.5 + (i % 11) * 0.5, 2.0 - (i / 11 * 0.5), 425.0f / 1024.0f, 552.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 1.0f, 0.03f));
	}

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				done = true;
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (state == STATE_MAIN_MENU) {
						state = STATE_GAME_LEVEL;
					}
					else {
						projectileMovement = true;
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT && player.boundaries[2] > -3.5f) {
					leftMovement = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT && player.boundaries[3] < 3.5f) {
					rightMovement = true;
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					leftMovement = false;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					rightMovement = false;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					projectileMovement = false;
				}
				break;
			}
		}
		//In game loop

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;

		//elapsed is how many seconds elapsed since last frame.
		//We will use this value to move everything in our game.

		lastFrameTicks = ticks;

		lastPlayerFire += elapsed;
		lastEnemyFire += elapsed;

		if (start) {
			Update(elapsed);
			Render();
		}
	}

	SDL_Quit();
	return 0;
}
