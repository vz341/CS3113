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
#include <algorithm>

//Include stb_image header.
//NOTE: You must define STB_IMAGE_IMPLEMENTATION in one of the files you are including it from!

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ShaderProgram.h"
#include "Matrix.h"

#define MAX_BULLETS 30

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

GLuint spriteSheetTexture;
GLuint fontSheetTexture;

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER};

int state;

bool start = false;

//Keeping time.

//In setup

float lastFrameTicks = 0.0f;
float elapsed;

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

	float texture_size = 1.0/16.0f;

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for(int i=0; i < text.size(); i++) {

		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size+spacing) * i) + (-0.5f * size), 0.5f * size,
			((size+spacing) * i) + (-0.5f * size), -0.5f * size,
			((size+spacing) * i) + (0.5f * size), 0.5f * size,
			((size+spacing) * i) + (0.5f * size), -0.5f * size,
			((size+spacing) * i) + (0.5f * size), 0.5f * size,
			((size+spacing) * i) + (-0.5f * size), -0.5f * size,
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

		void Draw(ShaderProgram *program);

		float size;
		unsigned int textureID;
		float u;
		float v;
		float width;
		float height;
};

void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v+height,
		u+width, v,
		u, v,
		u+width, v,
		u, v+height,
		u+width, v+height
	};

	float aspect = width / height;
	float verticies[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size};

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

		Entity(float x, float y) {
			this->x = x;
			this->y = y;
			modelMatrix.identity();
			modelMatrix.Translate(x, y, 0);
		}

		void Draw(ShaderProgram *program) {
			sprite.Draw(program);
		}

		float x = 0.0f;
		float y = 0.0f;

		float velocity;

		//float rotation;

		float timeAlive = 0.0f;

		SheetSprite sprite;

		void MoveInXDirection(float distance);

		void Update(float elapsed);
};

Entity player;

void Entity::MoveInXDirection(float distance) {
	x + distance;
}

void Entity::Update(float elapsed) {
	if (leftMovement == true) {
		player.x -= velocity * elapsed;
	}
	else if (rightMovement == true) {
		player.x += velocity * elapsed;
	}
	if (projectileMovement = true) {

	}
}

//Entities are a useful way for us to think about objects in the game.

std::vector<Entity> entities;

void Update(float elapsed) {
	for (int i = 0; i < entities.size(); i++) {
		entities[i].Update(elapsed);
	}
}

/*
Object pools.

- Less prone to memory leaks.
- Have a maximum number of objects.
- Allocated all at once.
- Know how fast things will run with maximum objects.
*/

int bulletIndex = 0;
Entity bullets[MAX_BULLETS];

void shootBullet() {
	bullets[bulletIndex].x = -1.2;
	bullets[bulletIndex].y = 0.0;
	bulletIndex++;
	if(bulletIndex > MAX_BULLETS-1) {
		bulletIndex = 0;
	}
}

Entity letter;

void RenderMainMenu() {
	letter.Draw(program);

	modelMatrix.identity();
	modelMatrix.Translate(-0.3f, 0.0f, 0.0f);

	program->setModelMatrix(modelMatrix);

	DrawText(program, fontSheetTexture, "PLAY", 0.2f, 0.0001f);
}

Entity enemySprite;

void RenderGameLevel() {
	player.Draw(program);

	enemySprite.Draw(program);

	for (int i = 0; i < entities.size(); i++) {
		entities[i].sprite.Draw(program);
	}
}

void UpdateGameLevel() {
	player.MoveInXDirection(3.0);
	player.Update(elapsed);

	for(int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].Update(elapsed);
	}
}

void Render() {
	switch(state) {
		case STATE_MAIN_MENU:
			RenderMainMenu();
		break;
		case STATE_GAME_LEVEL:
			RenderGameLevel();
		break;
	}
}
void UpdateState() {
	switch(state) {
		case STATE_GAME_LEVEL:
			UpdateGameLevel();
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

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program->programID);

	//Enabling blending

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Drawing (in your game loop)
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	//S
	//letter.sprite = SheetSprite(spriteSheetTexture, 341.0f/512.0f, 0.0f/512.0f, 44.0f/512.0f, 93.0f/512.0f, 0.5);

	player.sprite = SheetSprite(spriteSheetTexture, 0.0f/512.0f, 130.0f/512.0f, 104.0f/512.0f, 64.0f/512.0f, 0.5);

	//enemySprite.sprite = SheetSprite(spriteSheetTexture, 0.0f/512.0f, 196.0f/512.0f, 96.0f/512.0f, 56.0f/512.0f, 0.5);
	entities.push_back(enemySprite);

	//Bullet
	//bullets.sprite = SheetSprite(spriteSheetTexture, 268.0f/512.0f, 0.0f/512.0f, 24.0f/512.0f, 48.0f/512.0f, 0.5);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				switch (state) {
				case STATE_MAIN_MENU:
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && !start) {
						state = STATE_GAME_LEVEL;
						start = true;
					}
				break;
				case STATE_GAME_LEVEL:
					if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
						leftMovement = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
						rightMovement = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						projectileMovement = true;
					}
				break;
				}
			}
		}
		//In game loop

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;

		//elapsed is how many seconds elapsed since last frame.
		//We will use this value to move everything in our game.

		lastFrameTicks = ticks;

		//Clears the screen to the set clear color.
		glClear(GL_COLOR_BUFFER_BIT);

		Render();

		UpdateState();

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}