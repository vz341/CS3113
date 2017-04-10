/*

Assignment

Create a simple Separated Axis Collision demo using colliding rectagles or polygons.
(You will be provided with the SAT collision function).

It must have at least 3 objects colliding with each other and responding to collisions. They must be rotated and scaled!

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
#include <algorithm>

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

GLuint fontTexture;
GLuint meteorTexture;
GLuint distantStar1Texture;
GLuint distantStar2Texture;
GLuint distantStar3Texture;
GLuint distantMeteor1Texture;
GLuint distantMeteor2Texture;
GLuint distantMeteor3Texture;
GLuint distantMeteor4Texture;

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

int state;

bool start = true;

//Keeping time.

//In setup

float lastFrameTicks = 0.0f;
float elapsed;

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

struct Vector {
	Vector() {}
	Vector(float xAxis, float yAxis) {
		x = xAxis;
		y = yAxis;
	}

	//Vector length.
	float length() {
		//Use Pythagorean Theorem to get the vector length.

		//length = sqrt(x*x + y*y);

		//The dot product.
		//(x1*x2) + (y1*y2)
		//Applies one vector to another.

		float length = sqrtf(x*x + y*y);
		return length;
	}

	float x;
	float y;
};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(Vector &p1, Vector &p2) {
	return p1.length() < p2.length();
}

/*
int maxChecks = 10;
while (checkCollision(entity1, entity2) && maxChecks > 0) {

Vector responseVector = Vector(entity1->x, entity2->x, entity1->y, entity2->y);

responseVector.normalize();

entity1->x -= responseVector.x * 0.002;
entity1->y -= responseVector.y * 0.002;

entity2->x += responseVector.x * 0.002;
entity2->y += responseVector.y * 0.002;
maxChecks -= 1;
}
*/

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points) {
	Vector penetration;
	std::vector<Vector> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
}

//Ray/Polygon intersection test.

bool raySegmentIntersect(const Vector &rayOrigin, const Vector &rayDirection, const Vector &linePt1, const Vector &linePt2, float &dist)
{
	Vector seg1 = linePt1;
	Vector segD;
	segD.x = linePt2.x - seg1.x;
	segD.y = linePt2.y - seg1.y;

	float raySlope = rayDirection.y / rayDirection.x;
	float n = ((seg1.x - rayOrigin.x)*raySlope + (rayOrigin.y - seg1.y)) / (segD.y - segD.x*raySlope);

	if (n < 0 || n > 1)
		return false;

	float m = (seg1.x + segD.x * n - rayOrigin.x) / rayDirection.x;
	if (m < 0)
		return false;

	dist = m;
	return true;
}

Vector rotate(float xCoordinate, float yCoordinate, float angle, Vector p) {
	p.x -= xCoordinate;
	p.y -= yCoordinate;

	float newXCoordinate = p.x * cos(angle) - p.y * sin(angle);
	float newYCoordinate = p.x * sin(angle) + p.y * cos(angle);

	p.x = newXCoordinate + xCoordinate;
	p.y = newYCoordinate + yCoordinate;

	return p;
}

class Entity {
public:
	Matrix entityMatrix;

	GLuint texture;

	std::vector<Vector> corners;

	float position[2];
	float size[2];
	float boundaries[4];

	float u;
	float v;
	float width;
	float height;

	float angle;

	float speed[2];
	float acceleration[2];

	bool collided[4];

	bool isStatic = false;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture) {
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);

		position[0] = x;
		position[1] = y;
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

		speed[0] = dx;
		speed[1] = dy;
	}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, float sizeX, float sizeY, float newAngle) {
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);

		position[0] = x;
		position[1] = y;
		size[0] = sizeX;
		size[1] = sizeY;
		boundaries[0] = y + 0.1f * size[1];
		boundaries[1] = y - 0.1f * size[1];
		boundaries[2] = x - 0.1f * size[0];
		boundaries[3] = x + 0.1f * size[0];

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;

		speed[0] = dx;
		speed[1] = dy;
		acceleration[0] = 0.0f;
		acceleration[1] = 0.0f;

		angle = newAngle;

		corners.push_back(rotate(position[0], position[1], angle, Vector(boundaries[2], boundaries[0])));
		corners.push_back(rotate(position[0], position[1], angle, Vector(boundaries[3], boundaries[0])));
		corners.push_back(rotate(position[0], position[1], angle, Vector(boundaries[3], boundaries[1])));
		corners.push_back(rotate(position[0], position[1], angle, Vector(boundaries[2], boundaries[1])));
	}

	void draw() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);
		entityMatrix.Rotate(angle);
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
			speed[0] += acceleration[0];
			position[0] += speed[0] * elapsed;
			boundaries[2] += speed[0] * elapsed;
			boundaries[3] += speed[0] * elapsed;

			speed[1] += acceleration[1];
			position[1] += speed[1] * elapsed;
			boundaries[0] += speed[1] * elapsed;
			boundaries[1] += speed[1] * elapsed;

			for (size_t i = 0; i < corners.size(); i++) {
				corners[i].x += speed[0] * elapsed;
				corners[i].y += speed[1] * elapsed;
			}
		}
	}

	void updateX(float elapsed) {
		if (!isStatic) {
			speed[0] += acceleration[0];
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

std::vector<Entity> meteors;
std::vector<Entity> distantStars;
std::vector<Entity> distantMeteors;

void RenderMainMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-1.7f, 0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS THE SPACE BAR", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.7f, -0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "TO SEE WHAT HAPPENS IN SPACE", 0.2f, 0.0001f);
}

void RenderGameLevel() {
	viewMatrix.identity();
	program->setViewMatrix(viewMatrix);

	for (size_t i = 0; i < meteors.size(); i++) {
		meteors[i].draw();
	}
	for (size_t i = 0; i < distantStars.size(); i++) {
		distantStars[i].draw();
	}
	for (size_t i = 0; i < distantMeteors.size(); i++) {
		distantMeteors[i].draw();
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

void UpdateGameLevel(float elapsed) {
	for (size_t i = 0; i < meteors.size(); i++) {
		if (meteors[i].corners.size() == 4)
			meteors[i].update(elapsed);

		for (size_t j = i + 1; j < meteors.size(); j++) {
			if (checkSATCollision(meteors[i].corners, meteors[j].corners)) {
				meteors[i].speed[0] = meteors[i].speed[0] * -1;
				meteors[i].speed[1] = meteors[i].speed[1] * -1;
				meteors[j].speed[0] = meteors[j].speed[0] * -1;
				meteors[j].speed[1] = meteors[j].speed[1] * -1;
			}
		}
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
	displayWindow = SDL_CreateWindow("Assignment", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup (before the loop)

	glViewport(0, 0, 640, 360);

	//Need to use a shader program that supports textures!
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.25f, 2.25f, -1.0f, 1.0f);
	program->setProjectionMatrix(projectionMatrix);
	program->setModelMatrix(modelMatrix);
	program->setViewMatrix(viewMatrix);

	fontTexture = LoadTexture("font1.png");

	meteorTexture = LoadTexture("meteorGrey_big3.png");

	distantMeteor1Texture = LoadTexture("meteorGrey_small1.png");
	distantMeteor2Texture = LoadTexture("meteorGrey_small2.png");
	distantMeteor3Texture = LoadTexture("meteorGrey_tiny1.png");
	distantMeteor4Texture = LoadTexture("meteorGrey_tiny2.png");

	distantStar1Texture = LoadTexture("star1.png");
	distantStar2Texture = LoadTexture("star2.png");
	distantStar3Texture = LoadTexture("star3.png");

	meteors.push_back(Entity(-2.5f, -1.6f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, meteorTexture, 4.0f, 4.0f, 0.8f));
	meteors.push_back(Entity(-2.0f, -0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, meteorTexture, 5.0f, 10.0f, 0.4f));
	meteors.push_back(Entity(1.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, meteorTexture, 16.0f, 8.0f, 0.2f));

	distantStars.push_back(Entity(-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar3Texture, 1.0f, 1.0f, 0.0f));
	distantStars.push_back(Entity(-2.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar1Texture, 1.0f, 1.0f, 0.0f));
	distantStars.push_back(Entity(-3.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar2Texture, 1.0f, 1.0f, 0.0f));

	distantStars.push_back(Entity(0.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar2Texture, 1.0f, 1.0f, 0.0f));

	distantStars.push_back(Entity(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar1Texture, 1.0f, 1.0f, 0.0f));
	distantStars.push_back(Entity(2.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar3Texture, 1.0f, 1.0f, 0.0f));
	distantStars.push_back(Entity(3.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantStar2Texture, 1.0f, 1.0f, 0.0f));

	distantMeteors.push_back(Entity(0.0f, -1.9f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantMeteor1Texture, 2.0f, 2.0f, 0.0f));
	distantMeteors.push_back(Entity(1.0f, -1.9f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantMeteor2Texture, 2.0f, 2.0f, 0.0f));
	distantMeteors.push_back(Entity(2.0f, -1.9f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantMeteor3Texture, 1.0f, 1.0f, 0.0f));
	distantMeteors.push_back(Entity(3.0f, -1.9f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, distantMeteor4Texture, 1.0f, 1.0f, 0.0f));

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
						start = false;
					}
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