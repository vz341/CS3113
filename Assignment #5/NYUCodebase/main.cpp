/*

Assignment #5

Create a simple Separated Axis Collision demo using colliding rectagles or polygons.
(You will be provided with the SAT collision function).

It must have at least 3 objects colliding with each other and responding to collisions. They must be rotated and scaled!

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

//YOU WILL NEED YOUR OWN VECTOR CLASS that at least has x and y members and it is on you to calculate the WORLD SPACE coordinates for your shape vertices.

//A vector class.
class Vector {
	public:
		
		Vector() {}
		Vector(float x, float y) {
			this->x = x;
			this->y = y;
		}
		
		//Vector length.
		float length() {
			//Use Pythagorean Theorem to get the vector length.
			//The dot product.
			float len = sqrtf(x*x + y*y);
			return len;
		}

		//Done in testSATSeparationForEdge()
		//void normalize();

		float x;
		float y;
};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1,
	const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	//Divide each component by the length
	//(just be careful if the length is 0!).
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

	//Use smallest and largest values on the axis to figure out widths.

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	//How far away are they on X?
	float p = dist - ((e1Width + e2Width) / 2.0);

	//If p >= 0, we are not colliding!
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
It takes two vectors of world space vertex coordinates (of a convex polygon) describing the two shapes you are colliding and a reference to a vector into which to store the minimum penetration vector from the collision. It returns true if the shapes are colliding and false if they are not. If it returns true, the penetration vector you're passing by reference will contain the minimum penetration vector, which you can use to resolve your collision.
*/

/*
An updated version of the SAT collision function which always points that vector in the correct direction (away from the second entity). With this function, you just move the first entity by half of the penetration vector and the second entity by half of the negative of the penetration vector to resolve the collision.
*/
bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
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

//Rotates x and y positions counter-clockwise based on an angle
Vector rotateXandY(float posX, float posY, float angle, Vector position) {
	position.x -= posX;
	position.y -= posY;

	float x = position.x * cos(angle) - position.y * sin(angle);
	float y = position.x * sin(angle) + position.y * cos(angle);

	position.x = x + posX;
	position.y = y + posY;

	return position;
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

	void Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y, float angle);

	float size_x;
	float size_y;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram *program, Matrix modelMatrix, float x, float y, float angle) {
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
	//If the angle is 0.0,
	if (angle == 0.0f) {
		//Rotate it by the center
		modelMatrix.Rotate(angle);
	}
	//Otherwise,
	else {
		//Rotate it by the center plus pi / 1024
		modelMatrix.Rotate(angle += (3.141592653f / 1024));
	}
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

	Entity(float x, float y, float velocity_x, float velocity_y, float size_x, float size_y, bool ang) {
		this->x = x;
		this->y = y;

		this->velocity_x = velocity_x;
		this->velocity_y = velocity_y;

		//Contact flags that will detect collision between different entities based on half heights and widths
		left = x - size_x / 2;
		right = x + size_x / 2;
		top = y + size_y / 2;
		bottom = y - size_y / 2;

		angle = ang;

		//Rotates the four corners based on x and y positions, angle plus pi / 1024, and two intersecting sides
		corners.push_back(rotateXandY(x, y, angle += (3.141592653f / 1024), Vector(left, top)));
		corners.push_back(rotateXandY(x, y, angle += (3.141592653f / 1024), Vector(right, top)));
		corners.push_back(rotateXandY(x, y, angle += (3.141592653f / 1024), Vector(left, bottom)));
		corners.push_back(rotateXandY(x, y, angle += (3.141592653f / 1024), Vector(right, bottom)));
	}

	void Draw(ShaderProgram *program) {
		sprite.Draw(program, entityMatrix, x, y, angle);
	}

	SheetSprite sprite;

	float x;
	float y;

	float left;
	float right;
	float top;
	float bottom;

	float velocity_x;
	float velocity_y;

	Matrix entityMatrix;

	//Initializes the four corners
	std::vector<Vector> corners;

	//Initializes the angle
	float angle;

	//Adjusts the x and y distances of the entity after collision
	void collision(float adjustedDistance_x, float adjustedDistance_y) {
		this->y += adjustedDistance_y;
		this->top += adjustedDistance_y;
		this->bottom += adjustedDistance_y;
		this->x += adjustedDistance_x;
		this->left += adjustedDistance_x;
		this->right += adjustedDistance_x;
		//For every corner
		for (size_t i = 0; i < this->corners.size(); i++) {
			//Adjust its x and y coordinates to match
			this->corners[i].x += adjustedDistance_x;
			this->corners[i].y += adjustedDistance_y;
		}
	}

	void Update(float elapsed) {
		//Contact flags match with entity movement

		left = x - (this->sprite.size_x) / 2;
		right = x + (this->sprite.size_x) / 2;

		//Distance = Speed x Time
		x += velocity_x * elapsed;

		left += velocity_x * elapsed;
		right += velocity_x * elapsed;

		top = y + (this->sprite.size_y) / 2;
		bottom = y - (this->sprite.size_y) / 2;

		//Distance = Speed x Time
		y += velocity_y * elapsed;

		top += velocity_y * elapsed;
		bottom += velocity_y * elapsed;

		//For every corner
		for (size_t i = 0; i < this->corners.size(); i++) {
			//Adjust its x and y coordinates to match with entity movement
			this->corners[i].x += velocity_x * elapsed;
			this->corners[i].y += velocity_y * elapsed;
		}
	}
};

Entity meteorOne;
Entity meteorTwo;
Entity meteorThree;

std::vector<Entity> meteors;

void RenderMainMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-1.0f, 0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "PRESS SPACE", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-2.7f, -0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheetTexture, "TO SEE WHAT HAPPENS IN SPACE", 0.2f, 0.0001f);
}

void RenderGameLevel() {
	for (size_t i = 0; i < meteors.size(); i++) {
		meteors[i].Draw(program);
	}
}

void initializeEntities() {
	meteorOne = Entity(3.0f, -0.5, -1.0, 0.0, 2.0f, 2.0f, 0.0f);
	meteorOne.sprite = SheetSprite(spriteSheetTexture, 0.0f / 128.0f, 0.0f / 128.0f, 89.0f / 128.0f, 82.0f / 128.0f, 2.0f, 2.0f);

	meteorTwo = Entity(-3.0f, -0.5, 1.0, 0.0, 1.0f, 1.0f, .45f);
	meteorTwo.sprite = SheetSprite(spriteSheetTexture, 0.0f / 128.0f, 0.0f / 128.0f, 89.0f / 128.0f, 82.0f / 128.0f, 1.0f, 1.0f);

	meteorThree = Entity(-1.0f, 3.5, 0.0f, -1.0f, 1.5f, 1.5f, .90f);
	meteorThree.sprite = SheetSprite(spriteSheetTexture, 0.0f / 128.0f, 0.0f / 128.0f, 89.0f / 128.0f, 82.0f / 128.0f, 1.5f, 1.5f);

	meteors.push_back(meteorOne);
	meteors.push_back(meteorTwo);
	meteors.push_back(meteorThree);
}

void UpdateGameLevel(float elapsed) {
	Vector penetration;
	for (size_t i = 0; i < meteors.size(); i++) {
		meteors[i].Update(elapsed);
		for (size_t j = 0; j < meteors.size(); j++) {
			//The checkSATCollision function is what you should be calling.
			//Checks SAT collision between the two colliding meteors based on their corners and penetration
			if (checkSATCollision(meteors[i].corners, meteors[j].corners, penetration)) {
				//Since the collision is true, the meteors will go their separate ways
				meteors[i].collision(penetration.x / 2, penetration.y / 2);
				meteors[j].collision(-penetration.x / 2, -penetration.y / 2);
			}
		}
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
	displayWindow = SDL_CreateWindow("Assignment #5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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