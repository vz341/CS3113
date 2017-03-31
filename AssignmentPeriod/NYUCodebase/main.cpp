/*

Assignment.

- Make a simple scrolling platformer game demo.
- It must use a tilemap or static/dynamic entities.
- Must have a controllable player character that interacts with at least one other dynamic entity (enemy or item)
- It must scroll to follow your player with the camera.
- You have two weeks.

Vivian Zhao
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

// 60 FPS (1.0f/60.0f)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;

ShaderProgram* program;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

GLuint titleTexture;
GLuint fontTexture;
GLuint skyTexture;
GLuint cloudsTexture;
GLuint buildingsTexture;
GLuint fencesTexture;
GLuint metalBeamTexture;
GLuint metalScaffoldingTexture;
GLuint brickTexture;
GLuint groundTexture;
GLuint fireTexture;
GLuint playerTexture;
GLuint smallEnemyTexture;
GLuint crateTexture;

class Entity;

//Defines an array of vertex data.

float texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

int state;

bool start = true;

//Keeping time.

//In setup

float lastFrameTicks = 0.0f;
float lastPlayerFire = 0.0f;
float elapsed;

bool leftMovement = false;
bool rightMovement = false;
bool jumpMovement = false;
bool projectileMovement = false;

float speed = 2.5f;

enum EntityType { ENTITY_PLAYER, ENTITY_SMALL_ENEMY, ENTITY_BLOCK, ENTITY_CRATE };

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
	Matrix entityMatrix;
	float position[2];
	float boundaries[4];
	float size[2];

	float speed[2];
	float acceleration[2];
	bool collided[4];

	bool isStatic = true;
	EntityType type;

	float u;
	float v;
	float width;
	float height;
	GLuint texture;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, EntityType newType) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size[0] = 1.0f;
		size[1] = 1.0f;
		boundaries[0] = y + 0.05f * size[1] * 2;
		boundaries[1] = y - 0.05f * size[1] * 2;
		boundaries[2] = x - 0.05f * size[0] * 2;
		boundaries[3] = x + 0.05f * size[0] * 2;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;

		type = newType;
	}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, float sizeX, float sizeY, EntityType newType) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size[0] = sizeX;
		size[1] = sizeY;
		boundaries[0] = y + 0.05f * size[1] * 2;
		boundaries[1] = y - 0.05f * size[1] * 2;
		boundaries[2] = x - 0.05f * size[0] * 2;
		boundaries[3] = x + 0.05f * size[0] * 2;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;

		type = newType;
	}

	void draw() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);
		program->setModelMatrix(entityMatrix);

		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		float x_texture = u;
		float y_texture = v;
		vertexData.insert(vertexData.end(), {
			(-0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
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

		//Bind a texture to a texture target.
		glBindTexture(GL_TEXTURE_2D, texture);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	void update(float elapsed) {
		if (!isStatic) {
			position[0] += speed[0] * elapsed;
			boundaries[2] += speed[0] * elapsed;
			boundaries[3] += speed[0] * elapsed;

			speed[1] += acceleration[1];
			position[1] += speed[1] * elapsed;
			boundaries[0] += speed[1] * elapsed;
			boundaries[1] += speed[1] * elapsed;
		}
	}

	void updateX(float elapsed) {
		if (!isStatic) {
			position[0] += speed[0] * elapsed;
			boundaries[2] += speed[0] * elapsed;
			boundaries[3] += speed[0] * elapsed;
		}
	}

	void updateY(float elapsed) {
		if (!isStatic) {
			speed[1] += acceleration[1];
			position[1] += speed[1] * elapsed;
			boundaries[0] += speed[1] * elapsed;
			boundaries[1] += speed[1] * elapsed;
		}
	}
};

Entity player;
std::vector<Entity> smallEnemies;
std::vector<Entity> playerProjectiles;
std::vector<Entity> blocks;
std::vector<Entity> crates;

void RenderMainMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-0.7f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "MODIFIED", 0.2f, 0.0001f);
	modelMatrix.Translate(-0.75f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "SUPER CRATE BOX", 0.2f, 0.0001f);

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
	DrawText(program, fontTexture, "[   ] = SPACE BUTTON TO JUMP", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.3f, -2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS SPACE TO START NOW", 0.2f, 0.0001f);
}

void UpdateMainMenu(float elapsed) {
}

void RenderGameBackground() {
	Matrix sky;

	//Clears the screen to the set clear color.
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program->programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	modelMatrix.identity();
	
	program->setModelMatrix(sky);

	float backgroundVertices[] = { -4.25f, -2.25f, 4.25f, -2.25f, 4.25f, 2.25f, 4.25f, 2.25f, -4.25f, 2.25f, -4.25f, -2.25f };

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, backgroundVertices);
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	//Bind a texture to a texture target.
	glBindTexture(GL_TEXTURE_2D, skyTexture);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void RenderGameLevel() {
	player.draw();
	for (size_t i = 0; i < smallEnemies.size(); i++) {
		smallEnemies[i].draw();
	}
	for (size_t i = 0; i < playerProjectiles.size(); i++) {
		playerProjectiles[i].draw();
	}
	for (size_t i = 0; i < blocks.size(); i++) {
		blocks[i].draw();
	}
	for (size_t i = 0; i < crates.size(); i++) {
		crates[i].draw();
	}

	viewMatrix.identity();
	viewMatrix.Translate(-player.position[0], -player.position[1], 0.0f);
	program->setViewMatrix(viewMatrix);
}

void UpdateGameLevel(float elapsed) {
	for (int i = 0; i < 4; i++) {
		player.collided[i] = false;
	}

	float penetration;

	player.updateY(elapsed);
	for (size_t i = 0; i < crates.size(); i++) {
		crates[i].updateY(elapsed);
	}
	for (size_t i = 0; i < smallEnemies.size(); i++) {
		smallEnemies[i].updateY(elapsed);
	}

	for (size_t i = 0; i < blocks.size(); i++) {
		if (player.boundaries[1] < blocks[i].boundaries[0] &&
			player.boundaries[0] > blocks[i].boundaries[1] &&
			player.boundaries[2] < blocks[i].boundaries[3] &&
			player.boundaries[3] > blocks[i].boundaries[2])
		{
			float y_distance = fabs(player.position[1] - blocks[i].position[1]);
			float playerHeightHalf = 0.05f * player.size[1] * 2;
			float blockHeightHalf = 0.05f * blocks[i].size[1] * 2;
			penetration = fabs(y_distance - playerHeightHalf - blockHeightHalf);

			if (player.position[1] > blocks[i].position[1]) {
				player.position[1] += penetration + 0.001f;
				player.boundaries[0] += penetration + 0.001f;
				player.boundaries[1] += penetration + 0.001f;
				player.collided[1] = true;
			}
			else {
				player.position[1] -= (penetration + 0.001f);
				player.boundaries[0] -= (penetration + 0.001f);
				player.boundaries[1] -= (penetration + 0.001f);
				player.collided[0] = true;
			}
			player.speed[1] = 0.0f;
			break;
		}
	}

	for (size_t j = 0; j < crates.size(); j++)
		for (size_t i = 0; i < blocks.size(); i++) {
			if (crates[j].boundaries[1] < blocks[i].boundaries[0] &&
				crates[j].boundaries[0] > blocks[i].boundaries[1] &&
				crates[j].boundaries[2] < blocks[i].boundaries[3] &&
				crates[j].boundaries[3] > blocks[i].boundaries[2])
			{
				float y_distance = fabs(crates[j].position[1] - blocks[i].position[1]);
				float powerupsHeightHalf = 0.05f * crates[j].size[1] * 2;
				float blockHeightHalf = 0.05f * blocks[i].size[1] * 2;
				penetration = fabs(y_distance - powerupsHeightHalf - blockHeightHalf);

				if (crates[j].position[1] > blocks[i].position[1]) {
					crates[j].position[1] += penetration + 0.001f;
					crates[j].boundaries[0] += penetration + 0.001f;
					crates[j].boundaries[1] += penetration + 0.001f;
					crates[j].speed[1] = 0.5f;
				}
				else {
					crates[j].position[1] -= (penetration + 0.001f);
					crates[j].boundaries[0] -= (penetration + 0.001f);
					crates[j].boundaries[1] -= (penetration + 0.001f);
					crates[j].collided[0] = true;
					crates[j].speed[1] = 0.0f;
				}
				break;
			}
		}

	for (size_t j = 0; j < smallEnemies.size(); j++)
		for (size_t i = 0; i < blocks.size(); i++) {
			if (smallEnemies[j].boundaries[1] < blocks[i].boundaries[0] &&
				smallEnemies[j].boundaries[0] > blocks[i].boundaries[1] &&
				smallEnemies[j].boundaries[2] < blocks[i].boundaries[3] &&
				smallEnemies[j].boundaries[3] > blocks[i].boundaries[2])
			{
				float y_distance = fabs(smallEnemies[j].position[1] - blocks[i].position[1]);
				float powerupsHeightHalf = 0.05f * smallEnemies[j].size[1] * 2;
				float blockHeightHalf = 0.05f * blocks[i].size[1] * 2;
				penetration = fabs(y_distance - powerupsHeightHalf - blockHeightHalf);

				if (smallEnemies[j].position[1] > blocks[i].position[1]) {
					smallEnemies[j].position[1] += penetration + 0.001f;
					smallEnemies[j].boundaries[0] += penetration + 0.001f;
					smallEnemies[j].boundaries[1] += penetration + 0.001f;
					smallEnemies[j].speed[1] = 0.5f;
				}
				else {
					smallEnemies[j].position[1] -= (penetration + 0.001f);
					smallEnemies[j].boundaries[0] -= (penetration + 0.001f);
					smallEnemies[j].boundaries[1] -= (penetration + 0.001f);
					smallEnemies[j].collided[0] = true;
					smallEnemies[j].speed[1] = 0.0f;
				}
				break;
			}
		}

	player.updateX(elapsed);
	for (size_t i = 0; i < blocks.size(); i++) {
		if (player.boundaries[1] < blocks[i].boundaries[0] &&
			player.boundaries[0] > blocks[i].boundaries[1] &&
			player.boundaries[2] < blocks[i].boundaries[3] &&
			player.boundaries[3] > blocks[i].boundaries[2])
		{
			float x_distance = fabs(player.position[0] - blocks[i].position[0]);
			float playerWidthHalf = 0.05f * player.size[0] * 2;
			float blockWidthHalf = 0.05f * blocks[i].size[0] * 2;
			penetration = fabs(x_distance - (playerWidthHalf + blockWidthHalf));

			if (player.position[0] > blocks[i].position[0]) {
				player.position[0] += penetration + 0.001f;
				player.boundaries[2] += penetration + 0.001f;
				player.boundaries[3] += penetration + 0.001f;
				player.collided[3] = true;
			}
			else {
				player.position[0] -= (penetration + 0.001f);
				player.boundaries[2] -= (penetration + 0.001f);
				player.boundaries[3] -= (penetration + 0.001f);
				player.collided[2] = true;
			}
			player.speed[0] = 0.0f;
			break;
		}
	}

	player.speed[0] = 0.0f;

	for (size_t i = 0; i < crates.size(); i++) {
		if (player.boundaries[1] < crates[i].boundaries[0] &&
			player.boundaries[0] > crates[i].boundaries[1] &&
			player.boundaries[2] < crates[i].boundaries[3] &&
			player.boundaries[3] > crates[i].boundaries[2])
		{
			crates.erase(crates.begin() + i);
		}
	}

	for (size_t i = 0; i < smallEnemies.size(); i++) {
		if (player.boundaries[1] < smallEnemies[i].boundaries[0] &&
			player.boundaries[0] > smallEnemies[i].boundaries[1] &&
			player.boundaries[2] < smallEnemies[i].boundaries[3] &&
			player.boundaries[3] > smallEnemies[i].boundaries[2])
		{
			smallEnemies.erase(smallEnemies.begin() + i);
		}
	}

	if (leftMovement)
		player.speed[0] = -speed;
	else if (rightMovement)
		player.speed[0] = speed;
	if (jumpMovement && player.collided[1])
		player.speed[1] = 5.0f;
}

void Render() {
	//Clears the screen to the set clear color.
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameBackground();
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
	displayWindow = SDL_CreateWindow("Assignment.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 580, 475, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup (before the loop)

	glViewport(0, 0, 580, 475);

	//Need to use a shader program that supports textures!
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.25f, 2.25f, -1.0f, 1.0f);

	program->setProjectionMatrix(projectionMatrix);
	program->setModelMatrix(modelMatrix);
	program->setViewMatrix(viewMatrix);

	titleTexture = LoadTexture("Title.png");
	fontTexture = LoadTexture("Font.png");
	skyTexture = LoadTexture("Sky.png");
	cloudsTexture = LoadTexture("Clouds.png");
	buildingsTexture = LoadTexture("Buildings.png");
	fencesTexture = LoadTexture("Fences.png");
	metalBeamTexture = LoadTexture("Metal Beam.png");
	metalScaffoldingTexture = LoadTexture("Metal Scaffolding.png");
	brickTexture = LoadTexture("Bricks.png");
	groundTexture = LoadTexture("Ground.png");
	fireTexture = LoadTexture("Fire.png");
	playerTexture = LoadTexture("Player.png");
	smallEnemyTexture = LoadTexture("Small Enemy.png");
	crateTexture = LoadTexture("Crate.png");

	player = Entity(-3.55f, 0.7f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, playerTexture, 1.0f, 1.4f, ENTITY_PLAYER);
	player.isStatic = false;
	player.acceleration[1] = -0.01f;

	for (int i = 0; i < 18; i++) {
		blocks.push_back(Entity(-4.15f + (i) * 0.2f, -0.15f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, groundTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 18; i++) {
		blocks.push_back(Entity(-4.15f + (i) * 0.2f, 0.05f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, brickTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 18; i++) {
		blocks.push_back(Entity(0.75f + (i)* 0.2f, -0.15f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, groundTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 7; i++) {
		blocks.push_back(Entity(-0.58f + (i)* 0.2f, -0.15f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, fireTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 18; i++) {
		blocks.push_back(Entity(0.75f + (i)* 0.2f, 0.05f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, brickTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 40; i++) {
		blocks.push_back(Entity(-4.15f, 2.15 - (i * 0.1f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalBeamTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 40; i++) {
		blocks.push_back(Entity(4.15f, 2.15 - (i * 0.1f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalBeamTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 20; i++) {
		blocks.push_back(Entity(-2.0f + (i) * 0.2f, 0.0f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 20; i++) {
		blocks.push_back(Entity(-2.0f + (i)* 0.2f, 1.2f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 5; i++) {
		blocks.push_back(Entity(-3.95f + (i)* 0.2f, 0.6f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 5; i++) {
		blocks.push_back(Entity(3.15f + (i)* 0.2f, 0.6f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 17; i++) {
		blocks.push_back(Entity(-3.95f + (i)* 0.2f, 2.65f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 7; i++) {
		blocks.push_back(Entity(-0.6f + (i)* 0.2f, 2.65f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}
	for (int i = 0; i < 17; i++) {
		blocks.push_back(Entity(0.75f + (i)* 0.2f, 2.65f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, metalScaffoldingTexture, ENTITY_BLOCK));
	}

	smallEnemies.push_back(Entity(0.0f, 1.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, smallEnemyTexture, ENTITY_SMALL_ENEMY));
	smallEnemies[0].acceleration[1] = -0.01f;
	smallEnemies[0].isStatic = false;

	crates.push_back(Entity(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, crateTexture, ENTITY_CRATE));
	crates[0].acceleration[1] = -0.01f;
	crates[0].isStatic = false;

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
						jumpMovement = true;
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
					jumpMovement = false;
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

		if (start) {
			//Fixed timestep.

			float fixedElapsed = elapsed;
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				Update(FIXED_TIMESTEP);
			}
			Update(fixedElapsed);

			Render();
		}
	}

	SDL_Quit();
	return 0;
}