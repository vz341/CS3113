/*

Assignment #4

- Make a simple scrolling platformer game demo.
- It must use a tilemap or static/dynamic entities.
- Must have a controllable player character that interacts with at least one other dynamic entity (enemy or item)
- It must scroll to follow your player with the camera.
- You have two weeks.

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
float lastShot;

bool leftMovement = false;
bool rightMovement = false;
bool upMovement = false;
bool downMovement = false;
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

	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size_x, float size_y) {
		this->textureID = textureID;
		this->u = u;
		this->v = v;
		this->width = width;
		this->height = height;
		this->size_x = size_x;
		this->size_y = size_y;
	}

	void Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y);

	float size_x;
	float size_y;
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

	float verticies[] = {
		-0.5f * size_x, -0.5f * size_y,
		0.5f * size_x, 0.5f * size_y,
		-0.5f * size_x, 0.5f * size_y,
		0.5f * size_x, 0.5f * size_y,
		-0.5f * size_x, -0.5f * size_y,
		0.5f * size_x, -0.5f * size_y };

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

//Updating the Entity class.
class Entity {
public:

	Entity() {}

	Entity(float x, float y, float velocity_x, float velocity_y, bool grav) {
		this->x = x;
		this->y = y;

		this->velocity_x = velocity_x;
		this->velocity_y = velocity_y;

		//Contact flags that will detect collision between different entities
		left = x - 0.05f;
		right = x + 0.05f;
		top = y + 0.05f;
		bottom = y - 0.05f;

		acceleration_x = 0;
		acceleration_y = 0;

		if (grav) {
			acceleration_y = gravity;
		}

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

	float acceleration_x;
	float acceleration_y;

	float gravity = -1.0f;

	float timeAlive;

	Matrix entityMatrix;

	void Draw(ShaderProgram *program) {
		sprite.Draw(program, entityMatrix, x, y);
	}

	SheetSprite sprite;

	void collisionY(float adjustedDistance) {
		this->y += adjustedDistance;
		this->top += adjustedDistance;
		this->bottom += adjustedDistance;
		velocity_y = 0;
	}
	void collisionX(float adjustedDistance) {
		this->x += adjustedDistance;
		this->left += adjustedDistance;
		this->right += adjustedDistance;
		velocity_x = 0;
	}

	void Update(float elapsed) {
		//Contact flags match with entity movement

		left = x - (this->sprite.size_x) / 2;
		right = x + (this->sprite.size_x) / 2;

		//Velocity = Acceleration x Time
		//Gravity
		//A constant acceleration.
		velocity_x += acceleration_x * elapsed;

		//Distance = Speed x Time
		//Now only apply velocity to position on X-axis!
		x += velocity_x * elapsed;

		left += velocity_x * elapsed;
		right += velocity_x * elapsed;

		top = y + (this->sprite.size_y) / 2;
		bottom = y - (this->sprite.size_y) / 2;

		//Velocity = Acceleration x Time
		//Gravity
		//A constant acceleration.
		velocity_y += acceleration_y * elapsed;

		//Distance = Speed x Time
		//Only apply velocity to position on Y-axis!
		y += velocity_y * elapsed;

		top += velocity_y * elapsed;
		bottom += velocity_y * elapsed;
		
		timeAlive += elapsed;
	}
};

Entity player;

std::vector<Entity> enemies;

std::vector<Entity> bullets;

void shootBullet() {
	Entity newBullet(player.x, player.y, 3.0f, 0.0f, false);
	newBullet.sprite = SheetSprite(spriteSheetTexture, 474.0f / 2048.0f, 375.0f / 512.0f, 20.0f / 2048.0f, 40.0f / 512.0f, 0.1, 0.1);
	bullets.push_back(newBullet);
}

bool shouldRemoveBullet(Entity bullet) {
	if (bullet.timeAlive > 3.0) {
		return true;
	} else {
		return false;
	}
}

Entity title;

void RenderMainMenu() {
	title.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-0.3f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "PLAY", 0.2f, 0.0001f);
}

std::vector<Entity> grounds;
std::vector<Entity> bricks;

void RenderGameLevel() {
	//We need to translate everything so that the player is in the center of the screen.

	//The view matrix!
	viewMatrix.identity();
	//Translate the view matrix by the inverse of the player's position!
	//(or whatever you want to center on)!
	viewMatrix.Translate(-player.x, -player.y, 0.0f);
	program->setViewMatrix(viewMatrix);

	player.Draw(program);

	for (size_t i = 0; i < bullets.size(); i++) {
		bullets[i].Draw(program);
	}

	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Draw(program);
	}

	for (size_t i = 0; i < bricks.size(); i++) {
		bricks[i].Draw(program);
	}

	for (size_t i = 0; i < grounds.size(); i++) {
		grounds[i].Draw(program);
	}
}

void initializeEntities() {
	title = Entity(0.0, 1.3, 0.0, 0.0, false);
	title.sprite = SheetSprite(spriteSheetTexture, 0.0f / 2048.0f, 265.0f / 512.0f, 438.0f / 2048.0f, 228.0f / 512.0f, 4.0, 2.0);

	player = Entity(0.0, 0.0, 0.0, -1.0, true);
	player.sprite = SheetSprite(spriteSheetTexture, 474.0f / 2048.0f, 319.0f / 512.0f, 30.0f / 2048.0f, 22.0f / 512.0f, 0.2, 0.2);

	Entity smallEnemy(1.0f, -0.6, 0.0, 0.0, false);
	smallEnemy.sprite = SheetSprite(spriteSheetTexture, 474.0f / 2048.0f, 343.0f / 512.0f, 28.0f / 2048.0f, 30.0f / 512.0f, 0.2, 0.2);
	enemies.push_back(smallEnemy);

	Entity largeEnemy(1.5f, -0.5, 0.0, 0.0, false);
	largeEnemy.sprite = SheetSprite(spriteSheetTexture, 440.0f / 2048.0f, 265.0f / 512.0f, 52.0f / 2048.0f, 52.0f / 512.0f, 0.4, 0.4);
	enemies.push_back(largeEnemy);

	for (int i = 0; i < 50; i++) {
		Entity brick(-4.0f + (i * 0.2), -0.8, 0.0, 0.0, false);
		brick.sprite = SheetSprite(spriteSheetTexture, 440.0f / 2048.0f, 387.0f / 512.0f, 32.0f / 2048.0f, 32.0f / 512.0f, 0.2, 0.2);
		bricks.push_back(brick);
	}

	for (int i = 0; i < 50; i++) {
		Entity ground(-4.0f + (i * 0.2), -1.0, 0.0, 0.0, false);
		ground.sprite = SheetSprite(spriteSheetTexture, 440.0f / 2048.0f, 319.0f / 512.0f, 32.0f / 2048.0f, 32.0f / 512.0f, 0.2, 0.2);
		grounds.push_back(ground);
	}
}

//Is called when colliding
//Check full box/box collision against all entities.
void collidesWith(Entity& entOne, Entity& entTwo) {
	float penetration;
	//If collided, check Y-penetration
	//Checks y-axis collison or overlap or penetration
	//If entOne's bottom lower than entTwo's top or entOne's top higher than entTwo's bottom
	//The entities are intersecting.
	if (entOne.bottom < entTwo.top || entOne.top > entTwo.bottom) {
		//Calculates the distance overlap with float absolute value
		float distance = fabs(entOne.y - entTwo.y);
		//penetration = how much is overlapping and then add the adjustment
		//If the distance between the two entities is less than or equal to the sum of their half heights, then the entities are colliding!
		//Move on Y-axis by the ammount of penetration + tiny amount.
		//(Move up if above the other entity, otherwise move down!)
		penetration = fabs(distance - entOne.sprite.size_y / 2 - entTwo.sprite.size_y / 2) + 0.001f;

		//Checks for collision on the bottom
		if (entOne.y > entTwo.y) {
			//Moves the entity's y-coordinate by the penetration value
			entOne.collisionY(penetration);
		}
		//Checks for collision on the top
		else {
			//Pushes down the enttity
			entOne.collisionY(-penetration);
		}
	}
	//If collided, check X-penetration
	//Checks x-axis collison or overlap or penetration
	//Otherwise, if entOne's left is smaller than entTwo's right or entOne's right is larger than entTwo's left
	//The entities are intersecting.
	else if (entOne.left < entTwo.right || entOne.right > entTwo.left) {
		//Calculates the distance overlap with float absolute value
		float distance = fabs(entOne.x - entTwo.x);
		//penetration = how much is overlapping and then add the adjustment
		//If the distance between the two entities is less than or equal to the sum of their half lengths, then the entities are colliding!
		//Move on X-axis by the ammount of penetration + tiny amount.
		//(Move left if to the left of the other entity, otherwise move right!)
		penetration = fabs(distance - entOne.sprite.size_x/2 - entTwo.sprite.size_x/2) + 0.001f;
		//Checks for collision on the left
		if (entOne.x > entTwo.x) {
			//Moves the entity's x-coordinate by the penetration value to the right
			entOne.collisionX(penetration);
		}
		//Checks for collision on the right
		else {
			//Reverses
			entOne.collisionX(-penetration);
		}
	}
}

void UpdateGameLevel(float elapsed) {
	if (leftMovement) {
		player.velocity_x = -1.5f;
	}
	if (rightMovement) {
		player.velocity_x = 1.5f;
	}
	if (downMovement) {
		player.velocity_y = -1.5f;
	}
	if (upMovement) {
		//Set Y velocity directly to jump.
		player.velocity_y = 1.5f;
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
			for (size_t i = 0; i < enemies.size(); i++) {
				//Reverses direction
				enemies[i].velocity_x = -enemies[i].velocity_x;
			}
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
		//If the lasers' timeAlive is above 3.0f, remove them
		if (shouldRemoveBullet(bullets[i])) {
			bullets.erase(bullets.begin() + i);
		}
	}

	//Collision and penetration with player and enemies
	for (size_t i = 0; i < enemies.size(); i++) {
		if (!(player.bottom > enemies[i].top || player.top < enemies[i].bottom || player.left > enemies[i].right || player.right < enemies[i].left)) {
			//collidesWith(dynamic value, static value)
			collidesWith(player, enemies[i]);
		}
	}

	//Collision and penetration with player and the ground
	for (size_t i = 0; i < grounds.size(); i++) {
		grounds[i].Update(elapsed);
		if (player.bottom < grounds[i].top && player.top > grounds[i].bottom && player.left < grounds[i].right && player.right > grounds[i].left) {
			//collidesWith(dynamic value, static value)
			//Dynamic: gravity applied and checking collisions with other entities.
			//Static: No gravity, no movement, no collision checking!
			collidesWith(player, grounds[i]);
		}
	}

	//Collision and penetration with player and enemies
	for (size_t i = 0; i < bricks.size(); i++) {
		bricks[i].Update(elapsed);
		if (player.bottom < bricks[i].top && player.top > bricks[i].bottom && player.left < bricks[i].right && player.right > bricks[i].left) {
			//collidesWith(dynamic value, static value)
			//Dynamic: gravity applied and checking collisions with other entities.
			//Static: No gravity, no movement, no collision checking!
			collidesWith(player, bricks[i]);
		}
	}

	player.Update(elapsed);
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
	displayWindow = SDL_CreateWindow("Assignment #4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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
				else if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					upMovement = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					downMovement = true;
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					leftMovement = false;
					player.velocity_x = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					rightMovement = false;
					player.velocity_x = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					upMovement = false;
					player.velocity_y = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					downMovement = false;
					player.velocity_y = 0.0f;
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