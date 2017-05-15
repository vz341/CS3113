/*

CS3113

Final project requirements.

- Must have a title screen and proper states for game over, etc.
- Must have a way to quit the game.
- Must have music and sound effects.
- Must have at least 3 different levels or be procedurally generated.
- Must be either local multiplayer or have AI (or both!).
- Must have at least some animation or particle effects.

Kevin To
kt1386

Vivian Zhao
vz341

*/

/*

NOTES NOT MENTIONED DURING OUR PRESENTATION:

- Background music is supposed to be played for the main menu, but unfortunately, it does not
- Sounds are supposed to be played for both the player (when it gets hit by a fish bone, so it barks) and enemies (when they get hit by a bone, so they screech, but their screech can be very annoying after a while), but unfortunately they do not, because every time they are implemented, a random break occurs (similar to when we presented), and we are not too sure why after many failed attempts to implement them and we had to remove every instance of sound for the break to not occur anymore; for your convenience, the sound files are inside of the NYUCodebase folder
- Player can attempt to dodge all of the enemies and their fish bones to reach to the goal and move on to the next level and even win as well
- Artificial intelligence is present in level three, and it is not the best, because the enemies drop into pits if they are not hit yet
- Other than these notes, we believe we fulfilled all of the requirements to the bare minimum

*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <iostream>
#include <sstream>

#include <ctime>
#include <vector>

//Playing audio with SDL2
//SDL_mixer (the easy way).

//Initializing SDL_mixer

//Include SDL_mixer header.
#include <SDL_mixer.h>

//Include stb_image header.
//NOTE: You must define STB_IMAGE_IMPLEMENTATION in one of the files you are including it from!

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Matrix.h"
#include "ShaderProgram.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

// SDL, object matrix, and global values

SDL_Window* displayWindow;

ShaderProgram* program;

GLuint backgroundSheet;
GLuint fontSheet;
GLuint keyboardSheet;
GLuint dogSheet;
GLuint catSheet;
GLuint boneSheet;
GLuint groundSheet;
GLuint buildingSheet;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

float globalTexCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

bool gameRunning = true;
bool playerWins = false;

int gameState = 0;
int gameLevel = 1;

bool nextLevel = false;

int lives = 10;
int alive = 0;

int deathRange = 0;

//Time values
float lastFrameTicks = 0.0f;
float elapsed = 0.0f;

float animationTime = 0.0f;
float timer = 0.0f;
float runAnimationTimer = 0.0f;
float newTimer = 0.0f;

float playerLastJump = 0.0f;
float playerLastShot = 0.0f;
float enemyLastShot = 0.0f;

//Control booleans
bool moveUp = false;
bool moveDown = false;
bool moveLeft = false;
bool moveRight = false;
bool shootBone = false;

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

//Draws text
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

	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float szX, float szY) {
		this->textureID = textureID;
		this->u = u;
		this->v = v;
		this->width = width;
		this->height = height;
		this->sizeX = szX;
		this->sizeY = szY;
	}

	void Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y);

	void draw(ShaderProgram *program, Matrix modelMatrix);

	float sizeX;
	float sizeY;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y) {
	modelMatrix.identity();
	modelMatrix.Translate(x, y, 0.0f);
	program->setModelMatrix(modelMatrix);

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
		-0.5f * sizeX, -0.5f * sizeY,
		0.5f * sizeX, 0.5f * sizeY,
		-0.5f * sizeX, 0.5f * sizeY,
		0.5f * sizeX, 0.5f * sizeY,
		-0.5f * sizeX, -0.5f * sizeY,
		0.5f * sizeX, -0.5f * sizeY };

	// draw our arrays

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, verticies);
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

}

void SheetSprite::draw(ShaderProgram *program, Matrix modelMatrix) {
	program->setModelMatrix(modelMatrix);

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
		-0.5f * sizeX, -0.5f * sizeY,
		0.5f * sizeX, 0.5f * sizeY,
		-0.5f * sizeX, 0.5f * sizeY,
		0.5f * sizeX, 0.5f * sizeY,
		-0.5f * sizeX, -0.5f * sizeY,
		0.5f * sizeX, -0.5f * sizeY };

	// draw our arrays

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

	Entity(float x, float y, float velocityX, float velocityY, unsigned int textureID, float u, float v, float width, float height, float szX, float szY, float gravity) {
		this->x = x;
		this->y = y;
		
		left = x - szX / 2;
		right = x + szX / 2;
		top = y + szY / 2;
		bottom = y - szY / 2;

		collisionX = false;
		collisionY = false;

		this->velocityX = velocityX;
		this->velocityY = velocityY;
		
		acceleratonX = 0;
		acceleratonY = gravity;

		timeAlive = 0.0f;

		sprite = SheetSprite(textureID, u, v, width, height, szX, szY);
	}

	float x;
	float y;

	float left;
	float right;
	float top;
	float bottom;

	bool collisionX;
	bool collisionY;

	float velocityX;
	float velocityY;

	float acceleratonX;
	float acceleratonY;

	float gravity = 0.0f;

	float timeAlive;

	Matrix entityMatrix;

	void Draw(ShaderProgram *program) {
		sprite.Draw(program, entityMatrix, x, y);
	}

	SheetSprite sprite;

	void Update(float elapsed) {
		velocityX += acceleratonX * elapsed;
		x += velocityX * elapsed;
		left += velocityX * elapsed;
		right += velocityX * elapsed;

		if (collisionX == true) {
			velocityX = 0;
			collisionX = false;
		}
		velocityY += acceleratonY * elapsed;
		y += velocityY * elapsed;
		top += velocityY * elapsed;
		bottom += velocityY * elapsed;
		if (collisionY == true) {
			velocityY = 0;
			collisionY = false;
		}
		timeAlive += elapsed;
	}
};

//Global entities

Entity player;
Entity goal;

std::vector<Entity> enemies;
std::vector<Entity> platform;
std::vector<Entity> bones;
std::vector<Entity> fishBones;

float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
	float retVal = dstMin + ((value - srcMin) / (srcMax - srcMin) * (dstMax - dstMin));
	if (retVal < dstMin) {
		retVal = dstMin;
	}
	if (retVal > dstMax) {
		retVal = dstMax;
	}
	return retVal;
}

//LERP
//LinEar InteRPolation

float lerp(float from, float to, float time) {
	return (1.0 - time) * from + time * to;
}

Entity spaceBar;
Entity upAndDownKeys;
Entity leftKey;
Entity rightKey;
Entity escapeKey;

//Loading music.
Mix_Music *defaultMusic;

//Main menu settings text and rendering
void RenderMainMenu() {
	//Loading and playing music.

	//Loading music.
	defaultMusic = Mix_LoadMUS("sleeping at last - households.wav");

	//Playing music.

	//int Mix_PlayMusic(Mix_Music *music, int loops);
	//Plays specified music. Loops can be -1 to loop forever.

	Mix_PlayMusic(defaultMusic, -1);

	modelMatrix.identity();
	modelMatrix.Translate(-3.5f, 1.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "GET DOGGO HOME!", 0.5f, 0.0001f);
	player = Entity(0.0f, 1.25f, 0.0, 0.0, dogSheet, 0.0f / 32.0f, 0.0f / 32.0f, 19.0f / 32.0f, 20.0f / 32.0f, 0.5f, 0.5f, 0.0f);
	player.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.1f, 0.60f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS SPACE TO PLAY", 0.2f, 0.0001f);
	spaceBar = Entity(-1.5f, 0.6f, 0.0, 0.0, keyboardSheet, 0.0f / 512.0f, 0.0f / 1024.0f, 329.0f / 512.0f, 67.0f / 1024.0f, 1.3f, 0.5f, 0.0f);
	spaceBar.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.1f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS SPACE TO THROW BONES", 0.2f, 0.0001f);
	spaceBar = Entity(-1.5f, 0.0f, 0.0, 0.0, keyboardSheet, 0.0f / 512.0f, 0.0f / 1024.0f, 329.0f / 512.0f, 67.0f / 1024.0f, 1.3f, 0.5f, 0.0f);
	spaceBar.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.1f, -0.6f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS       TO MOVE UP", 0.2f, 0.0001f);
	upAndDownKeys = Entity(-1.5f, -0.6f, 0.0, 0.0, keyboardSheet, 283.0f / 512.0f, 311.0f / 1024.0f, 60.0f / 512.0f, 68.0f / 1024.0f, 0.5f, 0.5f, -0.5f);
	upAndDownKeys.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.1f, -1.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS       TO MOVE LEFT OR RIGHT", 0.2f, 0.0001f);
	leftKey = Entity(-1.9f, -1.2f, 0.0, 0.0, keyboardSheet, 191.0f / 512.0f, 980.0f / 1024.0f, 60.0f / 512.0f, 35.0f / 1024.0f, 0.5f, 0.5f, -0.5f);
	leftKey.Draw(program);
	rightKey = Entity(-1.1f, -1.2f, 0.0, 0.0, keyboardSheet, 253.0f / 512.0f, 977.0f / 1024.0f, 60.0f / 512.0f, 35.0f / 1024.0f, 0.5f, 0.5f, -0.5f);
	rightKey.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-3.1f, -1.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS       TO EXIT", 0.2f, 0.0001f);
	escapeKey = Entity(-1.5f, -1.8f, 0.0, 0.0, keyboardSheet, 0.0f / 512.0f, 936.0f / 1024.0f, 63.0f / 512.0f, 39.0f / 1024.0f, 0.5f, 0.5f, -0.5f);
	escapeKey.Draw(program);
}

//Game over settings text and rendering
void RenderGameOver() {
	//Loading and playing music.

	//Loading music.
	defaultMusic = Mix_LoadMUS("sleeping at last - households.wav");

	//Playing music.

	//int Mix_PlayMusic(Mix_Music *music, int loops);
	//Plays specified music. Loops can be -1 to loop forever.

	Mix_PlayMusic(defaultMusic, -1);

	//Displays text

	modelMatrix.identity();
	modelMatrix.Translate(-1.8f + player.x, 1.0f + player.y, 0.0f);
	program->setModelMatrix(modelMatrix);
	if (playerWins == true) {
		DrawText(program, fontSheet, "VICTORY", 0.4f, 0.0001f);
		modelMatrix.identity();
		modelMatrix.Translate(-3.3f + player.x, .25f + player.y, 0.0f);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontSheet, "Doggo is at home!", 0.4f, 0.0001f);
	}
	else {
		DrawText(program, fontSheet, "GAME OVER", 0.4f, 0.0001f);
		modelMatrix.identity();
		modelMatrix.Translate(-3.4f + player.x, .25f + player.y, 0.0f);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontSheet, "Doggo is lost forever!", 0.3f, 0.0001f);
	}

	modelMatrix.identity();
	modelMatrix.Translate(-1.70f + player.x, -1.0f + player.y, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS ESC TO EXIT", 0.3f, 0.0001f);

	gameRunning = false;
}

//Game level text and rendering
void RenderGameLevel() {
	modelMatrix.identity();
	modelMatrix.Translate(player.x - 4.0f, player.y + 3.0f , 0.0);
	program->setModelMatrix(modelMatrix);
	std::string text = "LIVE(S) X ";
	std::ostringstream oss;
	oss << lives;
	text += oss.str();
	DrawText(program, fontSheet, text, .4f, 0.0001f);

	goal.Draw(program);
	player.Draw(program);
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Draw(program);
	}
	for (size_t i = 0; i < platform.size(); i++) {
		platform[i].Draw(program);
	}
	for (size_t i = 0; i < bones.size(); i++) {
		bones[i].Draw(program);
	}
	for (size_t i = 0; i < fishBones.size(); i++) {
		fishBones[i].Draw(program);
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.x, -(player.y +1.0f), 0.0f);
	program->setViewMatrix(viewMatrix);

	std::string text1;
	std::string text2;

	if (gameLevel == 1) {
		text1 = "LEVEL 1";
		text2 = "Doggo is lost! Help get Doggo home!";
	}
	if (gameLevel == 2) {
		text1 = "LEVEL 2";
		text2 = "Uh, oh! Wrong home! Watch out for the pits!";
	}
	if (gameLevel == 3) {
		text1 = "LEVEL 3";
		text2 = "Almost there! Get doggo home!";
	}

	animationTime += elapsed;
	float animationStart = 0.0f;
	float animationEnd = 4.0f;
	float animationValue = mapValue(animationTime, animationStart, animationEnd, 0.0, 1.0);
	runAnimationTimer += elapsed;
	if (runAnimationTimer < timer) {
		modelMatrix.identity();
		modelMatrix.Translate((player.x - 1.0f), lerp(2.5f + player.y, -1.0f + player.y, animationValue), 0.0);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontSheet, text1, .5f, 0.0001f);
	}

	animationStart = 5.0f;
	animationEnd = 15.0f;
	animationValue = mapValue(animationTime, animationStart, animationEnd, 0.0, 1.0);
	if (runAnimationTimer > timer && runAnimationTimer < timer + 10.0f) {
		modelMatrix.identity();
		modelMatrix.Translate(lerp(3.0f + player.x, -8.0f + player.x, animationValue), player.y - 1.0f, 0.0);
		program->setModelMatrix(modelMatrix);
		DrawText(program, fontSheet, text2, .3f, 0.0001f);
	}
}

void cleanUpLevel() {
	enemies.clear();
	platform.clear();
	bones.clear();
	fishBones.clear();
}

//Game update
void UpdateGameLevel(float elapsed) {
	alive = 1;
	nextLevel = false;
	
	if (moveUp && (playerLastJump > 2.00f)) {
		playerLastJump = 0.0f;
		player.velocityY = 2.5f;
	}
	if (moveLeft) {
		player.velocityX = -2.0f;
	}
	if (moveRight) {
		player.velocityX = 2.0f;
	}

	if (player.y < deathRange) {
		alive = 0;
		if (lives <= 1) {
			gameState = STATE_GAME_OVER;
		}
	}
	
	player.Update(elapsed);
	for (size_t i = 0; i < platform.size(); i++) {
		if (player.bottom < platform[i].top &&
			player.top > platform[i].bottom &&
			player.left < platform[i].right &&
			player.right > platform[i].left)
		{
			if (player.bottom < platform[i].top || player.top > platform[i].bottom) {
				float distance = fabs(player.y - platform[i].y);
				float firstEnitityHeightHalf = player.sprite.sizeY / 2;
				float secondEntityHeightHalf = platform[i].sprite.sizeY / 2;
				float penetration = fabs(distance - firstEnitityHeightHalf - secondEntityHeightHalf);

				if (player.y > platform[i].y) {
					player.y += penetration + 0.001f;
					player.top += penetration + 0.001f;
					player.bottom += penetration + 0.001f;
				}
				else {
					player.y -= (penetration + 0.001f);
					player.top -= (penetration + 0.001f);
					player.bottom -= (penetration + 0.001f);
				}
				player.velocityY = 0.0f;
			}

			else if (player.left < platform[i].right || player.right > platform[i].left) {
				float distance = fabs(player.x - platform[i].x);
				float firstEnitityHeightHalf = player.sprite.sizeX / 2;
				float secondEntityHeightHalf = platform[i].sprite.sizeX / 2;
				float penetration = fabs(distance - firstEnitityHeightHalf - secondEntityHeightHalf);

				if (player.x > platform[i].x) {
					player.x += penetration + 0.01f;
					player.left += penetration + 0.01f;
					player.right += penetration + 0.01f;
				}
				else {
					player.x -= (penetration + 0.01f);
					player.left -= (penetration + 0.01f);
					player.right -= (penetration + 0.01f);
				}
				player.velocityX = 0.0f;
			}
		}
	}

	//Cats movement
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Update(elapsed);
		for (size_t j = 0; j < platform.size(); j++) {
			if (enemies[i].bottom < platform[j].top &&
				enemies[i].top > platform[j].bottom &&
				enemies[i].left < platform[j].right &&
				enemies[i].right > platform[j].left)
			{
				float distance = fabs(enemies[i].y - platform[j].y);
				float firstEnitityHeightHalf = enemies[i].sprite.sizeY / 2;
				float secondEntityHeightHalf = platform[j].sprite.sizeY / 2;
				float penetration = fabs(distance - firstEnitityHeightHalf - secondEntityHeightHalf);

				if (enemies[i].y > platform[j].y) {
					enemies[i].y += penetration + 0.001f;
					enemies[i].top += penetration + 0.001f;
					enemies[i].bottom += penetration + 0.001f;
					enemies[i].velocityY = 0.0f;
				}
				else {
					enemies[i].y -= (penetration + 0.001f);
					enemies[i].top -= (penetration + 0.001f);
					enemies[i].bottom -= (penetration + 0.001f);
					enemies[i].velocityY = 0.0f;
				}
				break;
			}
			//If cats touch player, then the game is over
			if (enemies[i].bottom < player.top &&
				enemies[i].top > player.bottom &&
				enemies[i].left < player.right &&
				enemies[i].right > player.left) {
				alive = 0;
				if (lives <= 1) {
					gameState = STATE_GAME_OVER;
				}
			}
		}
	}

	if (gameLevel >= 1) {
		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].Update(elapsed);
			int shooter;
			bool canShoot = false;
			if (enemyLastShot > 1.5f && enemies.size()) {
				enemyLastShot = 0;
				if (fabs(player.x - enemies[i].x) < 4.0f) {
					shooter = i;
					canShoot = true;
				}
				if (canShoot) {
					fishBones.push_back(Entity(enemies[shooter].x, enemies[shooter].y, -2.0f, 1.0f, boneSheet,
						0.0f / 512.0f, 194.0f / 256.0f, 147.0f / 512.0f, 56.f / 256.0f, .40f, .20f, -2.0f));
				}
			}
		}
	}
	if (gameLevel >= 2) {
		if (enemyLastShot > (2.0f / (enemies.size())) && enemies.size()) {
			enemyLastShot = 0;
			int shooter = rand() % enemies.size();
			fishBones.push_back(Entity(enemies[shooter].x, enemies[shooter].y, -1.5f, 1.0f, boneSheet,
				0.0f / 512.0f, 194.0f / 256.0f, 147.0f / 512.0f, 56.f / 256.0f, .40f, .20f, -2.0f));
		}

	}
	if (gameLevel >= 3) {
		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].Update(elapsed);
			int charge = rand() % 2;
			if (fabs(player.x - enemies[i].x) < 4.0f) {
				int charge = rand() % 2;
				enemies[i].velocityX = -.5;
			}
		}
	}

	//Sets values and makes bones
	if (shootBone) {
		if (playerLastShot > 1.0f) {
			playerLastShot = 0;
			bones.push_back(Entity(player.x, player.y, 4.0f, 1.0, boneSheet, 0.0f / 512.0f,
				0.0f / 256.0f, 384.0f / 512.0f, 192.f / 256.0f, .30f, .15f, -3.0f));
		}
	}

	//Bones hitting cats
	std::vector<int> removeBones;
	for (size_t i = 0; i < bones.size(); i++) {
		bones[i].Update(elapsed);
		for (size_t j = 0; j < platform.size(); j++) {
			if (platform[j].bottom < bones[i].top &&
				platform[j].top > bones[i].bottom &&
				platform[j].left < bones[i].right &&
				platform[j].right > bones[i].left){
				bones[i].velocityY = 1.0f;
			}
		}
		for (size_t j = 0; j < enemies.size(); j++) {
			if (enemies[j].bottom < bones[i].top &&
				enemies[j].top > bones[i].bottom &&
				enemies[j].left < bones[i].right &&
				enemies[j].right > bones[i].left) {
				enemies.erase(enemies.begin() + j);
				bones[i].timeAlive = 2.0f;
			}
		}
	}
	
	//If fish bones hits player, then the game is over
	for (size_t i = 0; i < fishBones.size(); i++) {
		fishBones[i].Update(elapsed);
		if (fishBones[i].bottom < player.top &&
			fishBones[i].top > player.bottom &&
			fishBones[i].left < player.right &&
			fishBones[i].right > player.left) {
			alive = 0;
			if (lives <= 1) {
				gameState = STATE_GAME_OVER;
			}
		}
		if (fishBones[i].timeAlive > 2.0f) {
			fishBones.erase(fishBones.begin() + i);
		}
	}
	
	for (size_t i = 0; i < bones.size(); i++) {
		if (bones[i].timeAlive > 2.0f) {
			bones.erase(bones.begin() + i);
		}
	}

	//Wins game
	if (player.x > goal.right -1.0f) {
		if (gameLevel == 3) {
			playerWins = true;
			gameState = STATE_GAME_OVER;
			backgroundSheet = LoadTexture(RESOURCE_FOLDER"State Screens_03.png");
		} 
		gameLevel += 1;
		nextLevel = true;
	}

	if (alive == 0) {
		lives = lives - 1;
		cleanUpLevel();
	}
}

//Renders background
void RenderMainMenuBackground() {
	Matrix background;
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program->programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	modelMatrix.identity();
	program->setModelMatrix(background);

	if (gameState == STATE_GAME_LEVEL || gameState == STATE_GAME_OVER) {
		float backgroundV[] = { -4.25f + player.x, -2.25f + (player.y + 1.0f), 4.25f + player.x, -2.25f + (player.y + 1.0f), 4.25f + player.x, 2.25f + (player.y + 1.0f),
			4.25f + player.x, 2.25f + (player.y + 1.0f), -4.25f + player.x, 2.25f + (player.y + 1.0f), -4.25f + player.x, -2.25f + (player.y + 1.0f) };
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, backgroundV);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, globalTexCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, backgroundSheet);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
	else {
		float backgroundV[] = { -4.25f, -2.25f, 4.25f, -2.25f, 4.25f, 2.25f, 4.25f, 2.25f, -4.25f, 2.25f, -4.25f, -2.25f, };
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, backgroundV);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, globalTexCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, backgroundSheet);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	playerLastShot += elapsed;
	playerLastJump += elapsed;
	enemyLastShot += elapsed;
}

//Render menu and background
void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	RenderMainMenuBackground();
	switch (gameState) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	case STATE_GAME_OVER:
		RenderGameOver();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

//Loading music.
Mix_Music *levelOneMusic;

void initLevel1() {
	//Loading and playing music.

	//Loading music.
	levelOneMusic = Mix_LoadMUS("Sun (Instrumental) - Sleeping at Last.wav");

	//Playing music.

	//int Mix_PlayMusic(Mix_Music *music, int loops);
	//Plays specified music. Loops can be -1 to loop forever.

	Mix_PlayMusic(levelOneMusic, -1);

	backgroundSheet = LoadTexture(RESOURCE_FOLDER"State Screens_05.png");

	//Initializes player and goal
	player = Entity(0.5f, -2.0f, 0.0, 0.0, dogSheet, 0.0f / 32.0f, 0.0f / 32.0f, 19.0f / 32.0f, 20.0f / 32.0f, 0.5f, 0.5f, -3.0f);
	goal = Entity(26, -2.0f + (3.0f/2), 0.0, 0.0, buildingSheet, 0.0f / 1024.0f, 0.0f / 1024.0f , 1006.0f / 1024.0f, 395.0f / 1024.0f, 6.0f, 3.0f, 0.0f);
	//Initializes enemies
	for (int i = 0; i <= 10; i++) {
		Entity enemy = Entity(5.0f + (i * 2.0f), -1.0f , 0.0, 0.0, catSheet, 0.0f / 32.0f, 0.0f / 32.0f, 25.0f / 32.0f, 22.0f / 32.0f, 0.5f, 0.5f, -1.5f);
		enemies.push_back(enemy);
	}

	//Initalize platforms
	for (int i = 0; i < 60; i++) {
		Entity tile = Entity(0.0f + (i)* 0.5f, -2.25f, 0.0, 0.0, groundSheet, 0.0f /128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	deathRange = player.y - .5f;
}

void initLevel2() {
	//Loading and playing music.

	//Loading music.
	defaultMusic = Mix_LoadMUS("sleeping at last - households.wav");

	//Playing music.

	//int Mix_PlayMusic(Mix_Music *music, int loops);
	//Plays specified music. Loops can be -1 to loop forever.

	Mix_PlayMusic(defaultMusic, -1);

	backgroundSheet = LoadTexture(RESOURCE_FOLDER"State Screens_07.png");
	//Initializes player and goal
	player = Entity(0.5f, -1.75f + (-10.0f), 0.0, 0.0, dogSheet, 0.0f / 32.0f, 0.0f / 32.0f, 19.0f / 32.0f, 20.0f / 32.0f, 0.5f, 0.5f, -3.0f);
	goal = Entity(47.0f, -1.75f + (-10.0f) + (3.0f / 2), 0.0, 0.0, buildingSheet, 0.0f / 1024.0f, 397.0f / 1024.0f, 446.0f / 1024.0f, 468.0f / 1024.0f, 6.0f, 3.0f, 0.0f);
	//Initializes enemies
	for (int i = 0; i < 20; i++) {
		Entity enemy = Entity(5.0f + (i * 2.0f), -1.0f + (-10.0f), 0.0, 0.0, catSheet, 0.0f / 32.0f, 0.0f / 32.0f, 25.0f / 32.0f, 22.0f / 32.0f, 0.5f, 0.5f, -1.5f);
		enemies.push_back(enemy);
	}
	deathRange = player.y - .50f;

	//Initalizes platforms
	for (int i = 0; i < 24; i++) {
		Entity tile = Entity(0.0f + (i)* 0.5f, -2.25f + (-10.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	for (int i = 0; i < 24; i++) {
		Entity tile = Entity(13.0f + (i)* 0.5f, -1.25f + (-10.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	for (int i = 0; i < 24; i++) {
		Entity tile = Entity(26.0f + (i)* 0.5f, -1.75f + (-10.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}

	for (int i = 0; i < 24; i++) {
		Entity tile = Entity(39.0f + (i)* 0.5f, -2.0f + (-10.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
}

//Loading music.
Mix_Music *levelThreeMusic;

void initLevel3() {
	//Loading and playing music.

	//Loading music.
	levelThreeMusic = Mix_LoadMUS("Sleeping at last - Moon.wav");

	//Playing music.

	//int Mix_PlayMusic(Mix_Music *music, int loops);
	//Plays specified music. Loops can be -1 to loop forever.

	Mix_PlayMusic(levelThreeMusic, -1);

	backgroundSheet = LoadTexture(RESOURCE_FOLDER"State Screens_09.png");
	//Initializes player
	player = Entity(.5f, -1.75f + (-20.0f), 0.0, 0.0, dogSheet, 0.0f / 32.0f, 0.0f / 32.0f, 19.0f / 32.0f, 20.0f / 32.0f, 0.5f, 0.5f, -3.0f);
	goal = Entity(55.0f, -2.0f + (-20.0f) + (3.0f / 2), 0.0, 0.0, buildingSheet, 448.0f / 1024.0f, 397.0f / 1024.0f, 212.0f / 1024.0f, 157.0f / 1024.0f, 6.0f, 3.0f, 0.0f);
	//Initialize enemies
	for (int i = 0; i < 25; i++) {
		Entity enemy = Entity(5.0f + (i * 2.0f), -1.0f + (-20.0f), 0.0, 0.0, catSheet, 0.0f / 32.0f, 0.0f / 32.0f, 25.0f / 32.0f, 22.0f / 32.0f, 0.5f, 0.5f, -1.5f);
		enemies.push_back(enemy);
	}
	deathRange = player.y - .50f;

	//Initalize platforms
	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(0.0f + (i)* 0.5f, -2.25f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(10.0f + (i)* 0.5f, -1.25f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(20.0f + (i)* 0.5f, -1.75f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(30.0f + (i)* 0.5f, -1.50f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}

	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(40.0f + (i)* 0.5f, -1.75f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}

	for (int i = 0; i < 18; i++) {
		Entity tile = Entity(50.0f + (i)* 0.5f, -2.25f + (-20.0f), 0.0, 0.0, groundSheet, 0.0f / 128.0f, 0.0f / 128.0f, 70.0f / 128.0f, 70.0f / 128.0f, 0.5f, 0.5f, 0.0f);
		platform.push_back(tile);
	}
}

//Updates game level when active
void Update(float elapsed) {
	switch (gameState) {
	case STATE_GAME_LEVEL:
		if (alive == 0 || nextLevel) {
			animationTime = newTimer;
			runAnimationTimer = newTimer;
			timer += elapsed;
			runAnimationTimer = timer;
			timer += 5.0f;

			alive = 1;
			if (gameLevel == 1) {
				initLevel1();
			}
			if (gameLevel == 2) {
				initLevel2();
			}
			if (gameLevel == 3) {
				initLevel3();
			}
		}
		UpdateGameLevel(elapsed);
		break;
	}
}

//Initializes entities, controls, and starts game
void runGame() {
	SDL_Event event;
	bool done = false;
	while (!done) {
		//Game controls
		while (SDL_PollEvent(&event)) {
			//Closing
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			switch (event.type) {
			case SDL_KEYDOWN:
				//Starts game level or shoots bones
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (gameState == STATE_MAIN_MENU) {
						gameState = STATE_GAME_LEVEL;
					}
					else {
						shootBone = true;
					}
				}
				//Movements
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					moveDown = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					moveLeft = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					moveRight = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					moveUp = true;
				}
			break;
			case SDL_KEYUP:
				//Stop movements
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					moveDown = false;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					moveUp = false;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					moveLeft = false;
					player.velocityX = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					moveRight = false;
					player.velocityX = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shootBone = false;
				}
			break;
			}
		}

		if (gameRunning) {
			Update(elapsed);
			Render();
		}
	}
	return;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Final Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup (before the loop)

	glViewport(0, 0, 1280, 720);

	//Need to use a shader program that supports textures!
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	
	//Load sprite sheets and textures
	backgroundSheet = LoadTexture(RESOURCE_FOLDER"More State Screens_03.png");
	fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	keyboardSheet = LoadTexture(RESOURCE_FOLDER"Keyboard Sheet Sprites.png");
	dogSheet = LoadTexture(RESOURCE_FOLDER"Dog Sheet Sprite.png");
	catSheet = LoadTexture(RESOURCE_FOLDER"Cat Sheet Sprite.png");
	boneSheet = LoadTexture(RESOURCE_FOLDER"Bone Sheet Sprites.png");
	groundSheet = LoadTexture(RESOURCE_FOLDER"slice33_33.png");
	buildingSheet = LoadTexture(RESOURCE_FOLDER"Building Sheet Sprites.png");

	projectionMatrix.setOrthoProjection(-4.25f, 4.25f, -2.25f, 2.25f, -1.0f, 1.0f);

	//Drawing (in your game loop)
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	//int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
	//Initializes SDL_mixer with frequency, format, channel and buffer size.

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	runGame();

	//Cleaning up.

	//Need to clean up music on quit.

	Mix_FreeMusic(defaultMusic);
	Mix_FreeMusic(levelOneMusic);
	Mix_FreeMusic(levelThreeMusic);

	SDL_Quit();
	return 0;
}