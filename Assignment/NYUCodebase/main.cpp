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

GLuint meteorTexture;

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

//A vector class.

class Vector {
	public:

		Vector();
		Vector(float x, float y, float z);
		
		//Vector length.
		float length() {
			//Use Pythagorean Theorem to get the vector length.
			
			//length = sqrt(x*x + y*y);

			//The dot product.
			//(x1*x2) + (y1*y2)
			//Applies one vector to another.
			
			z = sqrt(x*x + y*y);
			return z;
		}
		//Normalizing a vector.
		void normalize() {
			//Divide each vector component by the vector length (just be careful if the length is 0!).
			
			//Now, normalize the normal vectors.
			//x /= length;
			//y /= length;
			x /= length();
			y /= length();
		}
		
		float x;
		float y;
		float z;

		//Multiplying matrices and vectors.
		Vector operator * (const Vector &v);
};

class Entity {
public:

	Matrix matrix;

	Vector position;
	Vector scale;
	float rotation;


};

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

//sat.cpp

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

bool penetrationSort(const Vector &p1, const Vector &p2) {
	return p1.length() < p2.length();
}

//newSAT.cpp

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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Assignment", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	meteorTexture = LoadTexture("meteorGrey_big3.png");

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}