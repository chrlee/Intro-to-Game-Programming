// Pong
// by Christian Lee

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Matrix.h"
#include "ShaderProgram.h"


#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

/**********************************************
 **********************************************
 *******       Class Definitons      **********
 **********************************************
 **********************************************/


// Basic Game Object
class Entity {
public:
	Entity(float x, float y, float rotation, float width, float height, float velocity, float direction_x, float direction_y, int textureId) :
		x(x), y(y), rotation(rotation), width(width), height(height), velocity(velocity), direction_x(direction_x), direction_y(direction_y), textureId(textureId) {};
	virtual void Draw(ShaderProgram& program) = 0;
	float x;
	float y;
	float rotation;
	float width;
	float height;
	float velocity;
	float direction_x;
	float direction_y;
	int textureId;
};

class Paddle : public Entity {
public:
	Paddle(float x, float y, float width, float height) :
		Entity(x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f, 0){};
	void Draw(ShaderProgram& program) override {
		//glBindTexture(GL_TEXTURE_2D, textureId);
		float vertices[] = { x-(width/2), y-(height/2), x+(width/2), y-(height/2), x+(width/2), y+(height/2),
							 x-(width/2), y-(height/2), x+(width/2), y+(height/2), x-(width/2), y+(height/2) };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
	};
};

class Ball : public Entity {
public:
	Ball(float x, float y, float width, float height, float velocity) :
		Entity(x, y, 0.0f, width, height, velocity, 0.0f, 0.0f, 0) {};
	void Draw(ShaderProgram& program) override {
		//glBindTexture(GL_TEXTURE_2D, textureId);
		float vertices[] = { x - (width / 2), y - (height / 2), x + (width / 2), y - (height / 2), x + (width / 2), y + (height / 2),
			x - (width / 2), y - (height / 2), x + (width / 2), y + (height / 2), x - (width / 2), y + (height / 2) };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
	};
};

// Static Object
class Border : public Entity {
public:
	Border(float x, float y, float width, float height) :
		Entity(x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f, 0) {};
	void Draw(ShaderProgram& program) override {
		//glBindTexture(GL_TEXTURE_2D, textureId);
		float vertices[] = { x - (width / 2), y - (height / 2), x + (width / 2), y - (height / 2), x + (width / 2), y + (height / 2),
							 x - (width / 2), y - (height / 2), x + (width / 2), y + (height / 2), x - (width / 2), y + (height / 2) };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
	};
};


/**********************************************
 **********************************************
 *******    Function Declarations    **********
 **********************************************
 **********************************************/


GLuint LoadTexture(const char *filePath);

ShaderProgram Setup();

void ProcessEvents(SDL_Event& event, bool& done);

void Update(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, std::vector<Entity*> entities, float elapsed);

void Render(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, std::vector<Entity*> entities);

void Cleanup();

/**********************************************
 **********************************************
 *******            main()           **********
 **********************************************
 **********************************************/

int main(int argc, char *argv[])
{
	ShaderProgram program = Setup();

	// Setup Projection Matrix
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	float lastFrameTicks = 0.0f;

	// Instantiate Objects
	std::vector<Entity*> entities;
	entities.push_back(new Paddle(-3.4f, 0.0f, 0.1f, 0.7f));
	entities.push_back(new Paddle(3.4f, 0.0f, 0.1f, 0.7f));
	entities.push_back(new Ball(0.0f, 0.0f, 0.2f, 0.2f, rand()));
	entities.push_back(new Border(0.0f, 1.9f, 6.9f, 0.2f));
	entities.push_back(new Border(0.0f, -1.9f, 6.9f, 0.2f));
	entities.push_back(new Border(-3.55f, 0.0f, 0.0f, 4.0f));
	entities.push_back(new Border(3.55f, 0.0f, 0.0f, 4.0f));
	// Middle Wall
	for (int i = 0; i < 10; ++i) {
		entities.push_back(new Border(0.0, 0.0f + 0.2f*i, 0.05f, 0.1f));
		entities.push_back(new Border(0.0, 0.0f - 0.2f*i, 0.05f, 0.1f));
	}

	entities[2]->direction_x = rand()*0.00005 - 0.5;
	entities[2]->direction_y = rand()*0.00005 - 0.5;

	SDL_Event event;
	bool done = false;
	while (!done) {
		ProcessEvents(event, done);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		Update(projectionMatrix, modelviewMatrix, program, entities, elapsed);
		
		Render(projectionMatrix, modelviewMatrix, program, entities);
	}

	Cleanup();
	return 0;
}

/**********************************************
 **********************************************
 *******    Function Definitions     **********
 **********************************************
 **********************************************/

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	// LINEAR
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return retTexture;
}

bool isCollidingRect(Entity* r1, Entity* r2){
	// Is r1 bottom > r2 top?
	if (r1->y - r1->height / 2 > r2->y + r2->height / 2) return false;
	// Is r1 top < r2 bottom?
	if (r1->y + r1->height / 2 < r2->y - r2->height / 2) return false;
	// Is r1 left > r2 right?
	if (r1->x - r1->width / 2 > r2->x + r2->width / 2) return false;
	// Is r1 right < r2 left?
	if (r1->x + r1->width / 2 < r2->x - r2->width / 2) return false;

	return true;
}

/**********************************************
 **********************************************
 *******        Main Functions       **********
 *******          for main()         **********
 **********************************************
 **********************************************/

ShaderProgram Setup() {
	// Setup SDL
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	// Setup OpenGL
	glViewport(0, 0, 640, 360);

	// Untextured
	ShaderProgram* program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment.glsl");
	// Textured
	//ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	return *program;
}

void ProcessEvents(SDL_Event & event, bool& done){
	// SDL Event Loop
	while (SDL_PollEvent(&event)) {
		// Quit or Close Event
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
}

void Update(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, std::vector<Entity*> entities, float elapsed) {
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W] && entities[0]->y + entities[0]->height / 2 < entities[3]->y-entities[3]->height / 2) {
		//entities[0]->velocity = elapsed;
		entities[0]->y += elapsed*2;
	}
	if (keys[SDL_SCANCODE_S] && entities[0]->y - entities[0]->height / 2 > entities[4]->y + entities[4]->height / 2) {
		//entities[0]->velocity = -elapsed;
		entities[0]->y -= elapsed*2;
	}
	if (keys[SDL_SCANCODE_UP] && entities[1]->y + entities[1]->height / 2 < entities[3]->y - entities[3]->height / 2) {
		//entities[1]->velocity = elapsed;
		entities[1]->y += elapsed*2;
	}
	if (keys[SDL_SCANCODE_DOWN] && entities[1]->y - entities[1]->height / 2 > entities[4]->y + entities[4]->height / 2) {
		//entities[1]->velocity = -elapsed;
		entities[1]->y -= elapsed*2;
	}
	if (isCollidingRect(entities[2], entities[0]) || isCollidingRect(entities[2], entities[1])) entities[2]->direction_x *= -1;
	if (isCollidingRect(entities[2], entities[3]) || isCollidingRect(entities[2], entities[4])) entities[2]->direction_y *= -1;

	if (isCollidingRect(entities[2], entities[5])){
		//Game Over, Resets Ball
		entities[2]->x = 0.0f;
		entities[2]->y = 0.0f;

		//Player 2 Wins
		// whateverHappensAfterP2WinsFunction();
	}
	if (isCollidingRect(entities[2], entities[6])) {
		//Game Over, Resets Ball
		entities[2]->x = 0.0f;
		entities[2]->y = 0.0f;

		//Player 1 Wins
		// whateverHappensAfterP1WinsFunction();
	}

	entities[2]->x += entities[2]->direction_x*elapsed*5;
	entities[2]->y += entities[2]->direction_y*elapsed*5;

}

void Render(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, std::vector<Entity*> entities) {
	glClear(GL_COLOR_BUFFER_BIT);
	for (Entity*& ent : entities) {
		modelviewMatrix.Identity();
		//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
		program.SetModelviewMatrix(modelviewMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		glUseProgram(program.programID);
		ent->Draw(program);
	}

	SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
	SDL_Quit();
}