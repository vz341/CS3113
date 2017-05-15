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

ShaderProgram* program;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

//Generates a new OpenGL texture ID.
GLuint spriteSheetTexture;
GLuint fontSheetTexture;

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER};

int state;

bool start = true;

//Keeping time.

//In setup

float lastFrameTicks = 0.0f;
float elapsed;
float lastLaser;
float lastShot;

bool leftMovement = false;
bool rightMovement = false;
bool projectileMovement = false;

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//Use GL_LINEAR for linear filtering and GL_NEAREST for nearest neighbor filtering.

	//After you are done with the image data, you must free it using the stbi_image_free function.
	stbi_image_free(image);
	return retTexture;
}

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {

	float texture_size = 1.0 / 16.0f;

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {

		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	// draw this data (use the .data() method of std::vector to get pointer to data)

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

class SheetSprite {
public:
	SheetSprite() {}

	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) {
		this->textureID = textureID;
		this->u = u;
		this->v = v;
		this->width = width;
		this->height = height;
		this->size = size;
	}

	void Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};

	float aspect = width / height;
	float verticies[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };

	// draw our arrays

	modelMatrix.identity();
	modelMatrix.Translate(x, y, 0.0f);
	program->setModelMatrix(modelMatrix);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, verticies);
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

class Entity {
public:

	Entity() {}

	Entity(float x, float y, float velocity_x, float velocity_y) {
		this->x = x;
		this->y = y;

		//Although the player moves in the x direction,
		this->velocity_x = velocity_x;
		//Enemies will move in both the x and y directions
		this->velocity_y = velocity_y;

		//Contact flags that will detect collision between different entities
		left = x - 0.05f;
		right = x + 0.05f;
		top = y + 0.05f;
		bottom = y - 0.05f;

		timeAlive = 0.0f;
	}

	float x;
	float y;

	float left;
	float right;
	float top;
	float bottom;

	float velocity_x;
	float velocity_y;

	//Time bullets and lasers are alive so that they can be removed
	float timeAlive;

	Matrix entityMatrix;

	void Draw(ShaderProgram *program) {
		sprite.Draw(program, entityMatrix, x, y);
	}

	SheetSprite sprite;

	void Update(float elapsed) {
		//Distance = Speed x Time

		//elapsed controls time

		//x and y values are based on the center

		x += velocity_x * elapsed;
		left += velocity_x * elapsed;
		right += velocity_x * elapsed;

		y += velocity_y * elapsed;
		top += velocity_y * elapsed;
		bottom += velocity_y * elapsed;

		timeAlive += elapsed;
	}
};

Entity player;

Entity enemyOne;
Entity enemyTwo;
Entity enemyThree;
Entity enemyFour;
Entity enemyFive;
Entity enemySix;

std::vector<Entity> enemies;

std::vector<Entity> bullets;

//Player shoots bullets
void shootBullet() {
	//Initializes or spawns at the player's coordinates
	Entity newBullet(player.x, player.y, 0.0f, 3.0f);
	newBullet.sprite = SheetSprite(spriteSheetTexture, 268.0f / 512.0f, 0.0f / 512.0f, 24.0f / 512.0f, 48.0f / 512.0f, 0.2);
	bullets.push_back(newBullet);
}

//Removes bullet
bool shouldRemoveBullet(Entity bullet) {
	//If the bullet is alive longer than 3.0f
	if (bullet.timeAlive > 3.0f) {
		//Return true to remove
		return true;
	} else {
		//Otherwise, return false to not remove
		return false;
	}
}

std::vector<Entity> lasers;

//Enemies shoot lasers
void shootLaser(int shoot) {
	//Initializes or spawns at the enemies' coordinates
	Entity newlaser(enemies[shoot].x, enemies[shoot].y, 0.0f, -3.0f);
	newlaser.sprite = SheetSprite(spriteSheetTexture, 268.0f / 512.0f, 0.0f / 512.0f, 24.0f / 512.0f, 48.0f / 512.0f, 0.2);
	lasers.push_back(newlaser);
}

Entity topS;
Entity topP;
Entity topA;
Entity topC;
Entity topE;

Entity bottomI;
Entity bottomN;
Entity bottomV;
Entity bottomA;
Entity bottomD;
Entity bottomE;
Entity bottomR;
Entity bottomS;

//Renders main menu
void RenderMainMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-2.7f, 2.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "SCORE<1>  HI-SCORE  SCORE<2>", 0.2f, 0.0001f);

	topS.Draw(program);
	topP.Draw(program);
	topA.Draw(program);
	topC.Draw(program);
	topE.Draw(program);

	enemyOne.Draw(program);
	enemyTwo.Draw(program);
	enemyThree.Draw(program);
	enemyFour.Draw(program);
	enemyFive.Draw(program);
	enemySix.Draw(program);

	bottomI.Draw(program);
	bottomN.Draw(program);
	bottomV.Draw(program);
	bottomA.Draw(program);
	bottomD.Draw(program);
	bottomE.Draw(program);
	bottomR.Draw(program);
	bottomS.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-0.3f, -0.3f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "PLAY", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(0.9f, -2.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "CREDIT  00", 0.2f, 0.0001f);
}

//Renders game level
void RenderGameLevel() {
	modelMatrix.identity();
	modelMatrix.Translate(-2.7f, 2.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "SCORE<1>  HI-SCORE  SCORE<2>", 0.2f, 0.0001f);

	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Draw(program);
	}
	for (size_t i = 0; i < bullets.size(); i++) {
		bullets[i].Draw(program);
	}
	for (size_t i = 0; i < lasers.size(); i++) {
		lasers[i].Draw(program);
	}

	player.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(0.9f, -2.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "CREDIT  00", 0.2f, 0.0001f);
}

//Entities are a useful way for us to think about objects in the game.

//Initializes many different types of entities
void initializeEntities() {
	//Initializes the word, "SPACE" during the main menu
	topS = Entity(-0.6, 1.2, 0.0, 0.0);
	topS.sprite = SheetSprite(spriteSheetTexture, 341.0f / 512.0f, 0.0f / 512.0f, 44.0f / 512.0f, 93.0f / 512.0f, 0.5);
	topP = Entity(-0.3, 1.2, 0.0, 0.0);
	topP.sprite = SheetSprite(spriteSheetTexture, 254.0f / 512.0f, 150.0f / 512.0f, 45.0f / 512.0f, 93.0f / 512.0f, 0.5);
	topA = Entity(0.0, 1.2, 0.0, 0.0);
	topA.sprite = SheetSprite(spriteSheetTexture, 301.0f / 512.0f, 100.0f / 512.0f, 44.0f / 512.0f, 98.0f / 512.0f, 0.5);
	topC = Entity(0.3, 1.2, 0.0, 0.0);
	topC.sprite = SheetSprite(spriteSheetTexture, 295.0f / 512.0f, 0.0f / 512.0f, 44.0f / 512.0f, 98.0f / 512.0f, 0.5);
	topE = Entity(0.6, 1.2, 0.0, 0.0);
	topE.sprite = SheetSprite(spriteSheetTexture, 244.0f / 512.0f, 50.0f / 512.0f, 49.0f / 512.0f, 98.0f / 512.0f, 0.5);

	//Initializes the different types of enemies during the main menu
	enemyOne = Entity(-1.5, -1.2, 0.0, 0.0);
	enemyOne.sprite = SheetSprite(spriteSheetTexture, 188.0f / 512.0f, 196.0f / 512.0f, 64.0f / 512.0f, 48.0f / 512.0f, 0.2);
	enemyTwo = Entity(-0.9, -1.2, 0.0, 0.0);
	enemyTwo.sprite = SheetSprite(spriteSheetTexture, 178.0f / 512.0f, 50.0f / 512.0f, 64.0f / 512.0f, 48.0f / 512.0f, 0.2);
	enemyThree = Entity(-0.3, -1.2, 0.0, 0.0);
	enemyThree.sprite = SheetSprite(spriteSheetTexture, 178.0f / 512.0f, 0.0f / 512.0f, 88.0f / 512.0f, 48.0f / 512.0f, 0.2);
	enemyFour = Entity(0.3, -1.2, 0.0, 0.0);
	enemyFour.sprite = SheetSprite(spriteSheetTexture, 98.0f / 512.0f, 196.0f / 512.0f, 88.0f / 512.0f, 48.0f / 512.0f, 0.2);
	enemyFive = Entity(0.9, -1.2, 0.0, 0.0);
	enemyFive.sprite = SheetSprite(spriteSheetTexture, 106.0f / 512.0f, 130.0f / 512.0f, 96.0f / 512.0f, 64.0f / 512.0f, 0.2);
	enemySix = Entity(1.5, -1.2, 0.0, 0.0);
	enemySix.sprite = SheetSprite(spriteSheetTexture, 0.0f / 512.0f, 196.0f / 512.0f, 96.0f / 512.0f, 56.0f / 512.0f, 0.2);

	//Initializes the word, "INVADERS" during the main menu
	bottomI = Entity(-1.1, 0.6, 0.0, 0.0);
	bottomI.sprite = SheetSprite(spriteSheetTexture, 374.0f / 512.0f, 146.0f / 512.0f, 10.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomN = Entity(-0.8, 0.6, 0.0, 0.0);
	bottomN.sprite = SheetSprite(spriteSheetTexture, 204.0f / 512.0f, 100.0f / 512.0f, 30.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomV = Entity(-0.5, 0.6, 0.0, 0.0);
	bottomV.sprite = SheetSprite(spriteSheetTexture, 301.0f / 512.0f, 200.0f / 512.0f, 30.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomA = Entity(-0.2, 0.6, 0.0, 0.0);
	bottomA.sprite = SheetSprite(spriteSheetTexture, 347.0f / 512.0f, 95.0f / 512.0f, 25.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomD = Entity(0.1, 0.6, 0.0, 0.0);
	bottomD.sprite = SheetSprite(spriteSheetTexture, 374.0f / 512.0f, 95.0f / 512.0f, 24.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomE = Entity(0.4, 0.6, 0.0, 0.0);
	bottomE.sprite = SheetSprite(spriteSheetTexture, 333.0f / 512.0f, 200.0f / 512.0f, 25.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomR = Entity(0.7, 0.6, 0.0, 0.0);
	bottomR.sprite = SheetSprite(spriteSheetTexture, 347.0f / 512.0f, 146.0f / 512.0f, 25.0f / 512.0f, 49.0f / 512.0f, 0.5);
	bottomS = Entity(1.0, 0.6, 0.0, 0.0);
	bottomS.sprite = SheetSprite(spriteSheetTexture, 360.0f / 512.0f, 197.0f / 512.0f, 24.0f / 512.0f, 49.0f / 512.0f, 0.5);

	//Initializes the player during the game level
	player = Entity(0.0, -1.85, 0.0, 0.0);
	player.sprite = SheetSprite(spriteSheetTexture, 0.0f / 512.0f, 130.0f / 512.0f, 104.0f / 512.0f, 64.0f / 512.0f, 0.2);

	//Initializes the aliens during the game level
	for (int i = 0; i < 50; i++) {
		//(i % 10) is used to initialize 10 enemies per row
		//(i / 10) is used for the spacing of each enemy
		Entity enemy(-3.0f + (i % 10) * 0.5, 1.9 - (i / 10 * 0.4), 1.0f, -0.03f);
		enemy.sprite = SheetSprite(spriteSheetTexture, 0.0f / 512.0f, 196.0f / 512.0f, 96.0f / 512.0f, 56.0f / 512.0f, 0.2);
		enemies.push_back(enemy);
	}
}

void UpdateGameLevel(float elapsed) {
	player.Update(elapsed);

	if (leftMovement) {
		player.velocity_x = -3.0;
	}
	if (rightMovement) {
		player.velocity_x = 3.0;
	}
	if (projectileMovement) {
		//Shoots every 0.4f or waits until 0.4f to shoot again
		//lastShot is always increasing
		if (lastShot > 0.4f) {
			//lastShot needs to be reset to shoot
			lastShot = 0.0f;
			shootBullet();
		}
	}

	//Enemies movement
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Update(elapsed);
		//Enemies move left and move right
		if ((enemies[i].left < -4.0f && enemies[i].velocity_x < 0) || (enemies[i].right > 4.0f && enemies[i].velocity_x > 0)) {
			//Reverses direction
			for (size_t i = 0; i < enemies.size(); i++) {
				enemies[i].velocity_x = -enemies[i].velocity_x;
			}
		}
		//Invades by going down and past the player
		if (enemies[i].y < player.y) {
			//Game is over
			start = false;
		}
	}

	if (lastLaser > 0.5f) {
		lastLaser = 0;
		//Randomly picks an enemy to shoot from
		int enemyToShoot = rand() % enemies.size();
		shootLaser(enemyToShoot);
	}

	for (size_t i = 0; i < lasers.size(); i++) {
		lasers[i].Update(elapsed);
		//If the lasers' timeAlive is above 3.0f, remove them
		if (shouldRemoveBullet(lasers[i])) {
			lasers.erase(lasers.begin() + i);
		}
		//If lasers touch the player
		if (lasers[i].bottom < player.top && lasers[i].top > player.bottom && lasers[i].left < player.right && lasers[i].right > player.left) {
			//The game is over
			start = false;
		}
	}

	//Updates bullet and hits enemy
	for (size_t i = 0; i < bullets.size(); i++) {
		bullets[i].Update(elapsed);
		for (size_t j = 0; j < enemies.size(); j++) {
			if (enemies[j].bottom < bullets[i].top && enemies[j].top > bullets[i].bottom && enemies[j].left < bullets[i].right && enemies[j].right > bullets[i].left) {
				enemies.erase(enemies.begin() + j);
				//timeAlive for bullets = 4.0 so that shouldRemoveBullet can be true
				bullets[i].timeAlive = 4.0;
			}
		}
		if (shouldRemoveBullet(bullets[i])) {
			bullets.erase(bullets.begin() + i);
		}
	}

	if (enemies.size() == 0) {
		start = false;
	}
}

void Render() {
	switch (state) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	}
}
void Update(float elapsed) {
	switch (state) {
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

	fontSheetTexture = LoadTexture("pixel_font.png");
	spriteSheetTexture = LoadTexture("sprites.png");

	projectionMatrix.setOrthoProjection(-4.00, 4.00, -2.5f, 2.5f, -1.0f, 1.0f);

	glUseProgram(program->programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Drawing (in your game loop)
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	
	initializeEntities();

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
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
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					leftMovement = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					rightMovement = true;
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					leftMovement = false;
					player.velocity_x = 0;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					rightMovement = false;
					player.velocity_x = 0;
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
		lastLaser += elapsed;
		lastShot += elapsed;

		//Clears the screen to the set clear color.
		glClear(GL_COLOR_BUFFER_BIT);

		if (start) {
			Update(elapsed);
			Render();
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}