// Platformer Demo
// by Christian Lee

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include <array>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
// 60 FPS (1.0f/60.0f) (update sixty times a second)
#define FIXED_TIMESTEP 0.0166666f
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

// SpriteSheet
class SheetSprite {
public:
	SheetSprite(unsigned int textureID, std::array<float, 4> coords, float size) :
		textureID(textureID), u(coords[0]), v(coords[1]), width(coords[2]), height(coords[3]), size(size) {};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : 
		textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
	void Draw(ShaderProgram& program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
			u, v + height,
			u + width, v + height,
			u + width, v,
			u, v + height,
			u + width, v,
			u, v
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size ,
			0.5f * size * aspect, -0.5f * size };
		// draw our arrays
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

// 3D Vector
class Vector {
public:
	Vector() : x(0.0f), y(0.0f), z(0.0f), b(1.0f) {}
	Vector(const float x, const float y, const float z) : x(x), y(y), z(z), b(1.0f) {}
	Vector(const float x, const float y, const float z, const float b) : x(x), y(y), z(z), b(b) {}
	Vector& operator=(Vector const& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		b = rhs.b;
		return *this;
	}
	Vector operator+(Vector const& rhs) const {
		Vector out(x + rhs.x, y + rhs.y, z + rhs.z, b + rhs.b);
		return out;
	}
	Vector& operator+=(Vector const& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		b += rhs.b;
		return *this;
	}
	Vector operator*(const float num) const {
		Vector out(x*num, y*num, z*num, b*num);
		return out;
	}
	Vector& operator*=(const float num) {
		x *= num;
		y *= num;
		z *= num;
		b *= num;
		return *this;
	}
	float length() const {
		return sqrt(x*x + y*y + z*z);
	}
	Vector normalize() const {
		const float length = this->length();
		if(length) {
			return Vector(x / length, y / length, z / length, b);
		}
		return Vector();
	}
	float x, y, z, b;
};

// Basic Game Object
class Entity {
public:
	enum ENTITY_TYPE { ENTITY_PLAYER, ENTITY_SNAIL };
	Entity(ENTITY_TYPE type, SheetSprite sprite, float x, float y) : position(x, y, 0.0f), size(1.0f, 1.0f, 1.0f), sprite(sprite), type(type){
		matrix.SetPosition(x, y, 0.0f);
		alive = true;
	};
	void Draw(ShaderProgram& program) {
		//glBindTexture(GL_TEXTURE_2D, textureId);
		float vertices[] = { position.x - (size.x / 2), position.y - (size.y / 2), position.x + (size.x / 2), position.y - (size.y / 2), position.x + (size.x / 2), position.y + (size.y / 2),
			position.x - (size.x / 2), position.y - (size.y / 2), position.x + (size.x / 2), position.y + (size.y / 2), position.x - (size.x / 2), position.y + (size.y / 2) };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		sprite.Draw(program);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}
	Matrix translate(const float x, const float y, const float z) {
		matrix.Translate(x, y, z);
		position += Vector(x, y, z);
		return matrix;
	}

	Matrix matrix;
	Vector position;
	Vector size;
	Vector velocity;
	Vector acceleration;
	SheetSprite sprite;
	float rotation;
	ENTITY_TYPE type;
	bool alive;
	/*
	 *  contact indices clockwise:
	 *		0
	 *	3  obj	1
	 *		2
	 */
	bool* contact[4];

};

// Game States
class GameState {
public:
	enum STATE_TYPE { STATE_MENU, STATE_GAME };
	GameState(STATE_TYPE type) : type(type) {}
	std::vector<Entity*> entities;
	std::vector<Entity*> bgEntities;
	STATE_TYPE type;
};

/**********************************************
 **********************************************
 *******    Function Declarations    **********
 **********************************************
 **********************************************/


GLuint LoadTexture(const char *filePath);

float randf(float a, float b);

std::array<float, 4> pxToUV(int sheetWidth, int sheetHeight, int xPx, int yPx, int width, int height);

bool readHeader(std::ifstream& stream);

bool readLayerData(std::ifstream& stream);

bool readEntityData(std::ifstream& stream);

ShaderProgram Setup();

std::vector<GameState*> Instantiate();

void ProcessEvents(SDL_Event& event, bool& done, GameState*& currentState, std::vector<GameState*>& states);

void Update(GameState* state, float elapsed);

void Render(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, GameState* state);

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

	//Instantiate Objects, Textures, and States
	
	std::vector<GameState*> states = Instantiate();

	GameState* currentState = states[0];
	SDL_Event event;
	bool done = false;
	float accumulator = 0.0f;
	while (!done) {
		ProcessEvents(event, done, currentState, states);

		float ticks = static_cast<float>(SDL_GetTicks()) / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		// get elapsed time
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(currentState, FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		Render(projectionMatrix, modelviewMatrix, program, currentState);
	}


	Cleanup();
	return 0;
}

/**********************************************
 **********************************************
 *******      Helper Functions       **********
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
	if (r1->position.y - r1->size.y / 2 > r2->position.y + r2->size.y / 2) return false;
	// Is r1 top < r2 bottom?
	if (r1->position.y + r1->size.y / 2 < r2->position.y - r2->size.y / 2) return false;
	// Is r1 left > r2 right?
	if (r1->position.x - r1->size.y / 2 > r2->position.x + r2->size.y / 2) return false;
	// Is r1 right < r2 left?
	if (r1->position.x + r1->size.y / 2 < r2->position.x - r2->size.y / 2) return false;

	return true;
}

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

float randf(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

std::array<float, 4> pxToUV(int sheetWidth, int sheetHeight, int xPx, int yPx, int width, int height)
{
	std::array<float, 4> out;
	out[0] = float(xPx) / sheetWidth;
	out[1] = float(yPx) / sheetHeight;
	out[2] = float(width) / sheetWidth;
	out[3] = float(height) / sheetHeight;
	return out;
}

bool readHeader(std::ifstream &stream) {
	std::string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		std::istringstream sStream(line);
		std::string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new int[mapWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream &stream) {
	std::string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		std::istringstream sStream(line);
		std::string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				std::istringstream lineStream(line);
				std::string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(std::ifstream &stream) {
	std::string line;
	std::string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		std::istringstream sStream(line);
		std::string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			std::istringstream lineStream(value);
			std::string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ‘, ');
				float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
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
	//ShaderProgram* program = new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	// Textured
	ShaderProgram* program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	return *program;
}

std::vector<GameState*> Instantiate() {
	// Load background and textures
	unsigned int spriteSheetTexture = LoadTexture("spritesheet.png");
	int sheetWidth = 694;
	int sheetHeight = 372;
	Vector bgStarPosition(0.0f, 2.0f, 0.0f);
	Vector const bgStarVelocity(0.0f, -1.0f, 0.0f);
	Vector const bgStarSize(0.01f, 0.01f, 0.0f);
	Vector const bgStarAcceleration(0.0f, 0.0f, 0.0f);
	SheetSprite const starSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 0, 240, 1, 1), 1.0f);
	SheetSprite const logoSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 437, 500, 99, 59), 1.0f);
	std::vector<SheetSprite> sprites;
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 131, 624, 73, 52), 1.0f)); // 0. Player
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 340, 618, 96, 58), 1.0f)); // 1. Explosion
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 468, 380, 28, 51), 1.0f)); // 2. Enemy Projectile
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 293, 0, 80, 80), 1.0f));   // 3. Enemies Row 1
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 146, 0, 110, 80), 1.0f));  // 4. Enemies Row 2
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 0, 0, 110, 80), 1.0f));    // 5. Enemies Row 3
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 0, 120, 120, 80), 1.0f));  // 6. Enemies Row 4
	sprites.push_back(SheetSprite(spriteSheetTexture, pxToUV(sheetWidth, sheetHeight, 141, 120, 120, 80), 1.0f));// 7. Enemies Row 5

	std::vector<GameState*> states;
	states.push_back(new GameState(GameState::STATE_MENU));
	states.push_back(new GameState(GameState::STATE_GAME));

	for (int i = 0; i < 30; ++i) {
		bgStarPosition = Vector(randf(-3.55f, 3.55f), randf(-2.0f, 2.0f), 0.0f);
		states[0]->bgEntities.push_back(new Entity(bgStarPosition, bgStarSize, bgStarVelocity, bgStarAcceleration, starSprite, Entity::ENTITY_STAR, true));
	}
	states[0]->entities.push_back(new Entity(0.0f, 0.0f, 0.0f, 2.0f, 1.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, logoSprite, Entity::ENTITY_LETTER, true));

	states[1]->entities.push_back(new Entity(0.0f, -1.7f, 0.0f, 0.3f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, sprites[0], Entity::ENTITY_PLAYER, true));
	float enemyWidth = 0.3f;
	float enemyHeight = 0.2f;
	float spacing = 0.5f;
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 8; ++j) {
			Vector enemyPosition(-1.75f + spacing*j, 1.7f - spacing*i, 0.0f);
			Vector enemySize(enemyWidth, enemyHeight, 0.0f);
			Vector enemyVelocity(0.3f, 0.0f, 0.0f);
			Vector zero;
			states[1]->entities.push_back(new Entity(enemyPosition, enemySize, enemyVelocity, zero, sprites[i + 3], Entity::ENTITY_ENEMY, true));
		}
	}
	for (int i = 0; i < 10; ++i) {
		Vector offScreen(4.0f, 0.0f, 0.0f);
		Vector bulletSize(0.025f, 0.15f, 0.0f);
		Vector zero;
		states[1]->entities.push_back(new Entity(offScreen, bulletSize, zero, zero, starSprite, Entity::ENTITY_BULLET, false));
	}
	for (int i = 0; i < 10; ++i) {
		Vector offScreen(0.0f, 3.0f, 0.0f);
		Vector bulletSize(0.07f, 0.2f, 0.0f);
		Vector zero;
		states[1]->entities.push_back(new Entity(offScreen, bulletSize, zero, zero, sprites[2], Entity::ENTITY_BULLET, false));
	}

	std::string const levelFile = ("MapFlare.txt");

	std::ifstream infile(levelFile);
	std::string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[ObjectsLayer]") {
			readEntityData(infile);
		}
	}

	return states;
}

void ProcessEvents(SDL_Event & event, bool& done, GameState*& currentState, std::vector<GameState*>& states){
	// SDL Event Loop
	while (SDL_PollEvent(&event)) {
		// Quit or Close Event
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event.type == SDL_KEYUP) {
			if (currentState->type == GameState::STATE_MENU) {
				currentState = states[1];
				currentState->bgEntities = states[0]->bgEntities;
			}
			else if (currentState->type == GameState::STATE_GAME && currentState->entities[0]->alive) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					int i = 41;
					while (currentState->entities[i]->alive && i <= 51) ++i;
					if (i < 51) {
						Entity* bullet = currentState->entities[i];
						bullet->alive = true;
						bullet->position = Vector(currentState->entities[0]->position.x, currentState->entities[0]->position.y + 0.2f, 0.0f);
						bullet->velocity = Vector(0.0f, 1.5f, 0.0f);
					}
				}
			}
			
		}
	}

	// Keyboard Polling
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RIGHT] && currentState->entities[0]->position.x + currentState->entities[0]->size.x / 2 < 3.55) {
		currentState->entities[0]->velocity.x = 2;
	}
	if (keys[SDL_SCANCODE_LEFT] && currentState->entities[0]->position.x - currentState->entities[0]->size.x / 2 > -3.55) {
		currentState->entities[0]->velocity.x = -2;
	}
	
	// Collisions and other events
	if (currentState->type == GameState::STATE_GAME) {
		// Turn the enemies
		bool turn = false;
		for (int i = 0; i < 8; ++i) {
			if (turn) break;
			for (int j = 0; j < 5; ++j) {
				if (turn = (currentState->entities[1 + i * 5 + j]->position.x > 3.4 && currentState->entities[1 + i * 5 + j]->velocity.x > 0)
					|| (currentState->entities[1 + i * 5 + j]->position.x < -3.4 && currentState->entities[1 + i * 5 + j]->velocity.x < 0)) break;
			}
		}
		if (turn) {
			for (int i = 1; i < 41; ++i) {
				currentState->entities[i]->velocity.x *= -1;
			}
		}

		// Player Bullet Collision
		for(int i = 41; i < 51; ++i) {
			for(int j = 1; j < 41; ++j) {
				if (currentState->entities[i]->alive && currentState->entities[j]->alive &&isCollidingRect(currentState->entities[i], currentState->entities[j])) {
					currentState->entities[i]->alive = currentState->entities[j]->alive = false;
				}
			}
		}

		// Enemy Bullet Generation
		if (randf(0.0f, 1.0f) < 0.0005) {
			int i = 51;
			while (currentState->entities[i]->alive && i <= 57) ++i;
			if (i < 57) {
				Entity* bullet = currentState->entities[i];
				bullet->alive = true;
				int randIdx = rand() % 40 + 1;
				while(!currentState->entities[randIdx]->alive) randIdx = rand() % 40 + 1;
				bullet->position = Vector(currentState->entities[randIdx]->position.x, currentState->entities[randIdx]->position.y - 0.2f, 0.0f);
				bullet->velocity = Vector(0.0f, -1.5f, 0.0f);
			}
		}

		// Enemy Bullet Collision
		for(int i = 51; i < 57; ++i) {
			if (done) break;
			if (isCollidingRect(currentState->entities[0], currentState->entities[i])) {
				currentState->entities[0]->alive = currentState->entities[i]->alive = false;
				done = true;
			}
		}

		// Check if all enemies are dead
		if (!done) {
			bool allDead = true;
			for (int i = 1; i < 41; ++i) {
				if (!allDead) break;
				if (currentState->entities[i]->alive) allDead = false;
			}
			done = allDead;
		}
	}
}

void Update(GameState* state, float elapsed) {
	switch(state->type)
	{
	case GameState::STATE_MENU :
		for (Entity*& ent : state->entities) {
			//ent->velocity += ent->acceleration;
			//ent->position += ent->velocity * elapsed;

		}
		for (Entity*& ent : state->bgEntities) {
			ent->position += ent->velocity * elapsed;
			if(ent->position.y < -2.0f) {
				ent->position = Vector(randf(-3.55f, 3.55f), 2.0f + randf(0.0f, 1.0f), 0.0f);
			}
		}
	case GameState::STATE_GAME :
		for (Entity*& ent : state->entities) {
			if (ent->alive) {
				ent->position += ent->velocity * elapsed;
			}
			if (ent->type == Entity::ENTITY_PLAYER) ent->velocity = Vector(0.0f, 0.0f, 0.0f);
			else if (ent->type == Entity::ENTITY_BULLET) {
				if (ent->position.y > 3.0f || ent->position.y < -3.0f) ent->alive = false;
			}
		}
		for (Entity*& ent : state->bgEntities) {
			ent->position += ent->velocity * elapsed;
			if (ent->position.y < -2.0f) {
				ent->position = Vector(randf(-3.55f, 3.55f), 2.0f + randf(0.0f, 1.0f), 0.0f);
			}
		}
	}


}

void Render(Matrix& projectionMatrix, Matrix& modelviewMatrix, ShaderProgram& program, GameState* state) {
	glClear(GL_COLOR_BUFFER_BIT);
	switch(state->type)
	{
	case GameState::STATE_MENU :
		for (Entity*& ent : state->entities) {
			modelviewMatrix.Identity();
			//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
			program.SetModelviewMatrix(modelviewMatrix);
			program.SetProjectionMatrix(projectionMatrix);
			glUseProgram(program.programID);
			ent->Draw(program);
		}
		for (Entity*& ent : state->bgEntities) {
			modelviewMatrix.Identity();
			//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
			program.SetModelviewMatrix(modelviewMatrix);
			program.SetProjectionMatrix(projectionMatrix);
			glUseProgram(program.programID);
			ent->Draw(program);
		}
	case GameState::STATE_GAME :
		for (Entity*& ent : state->entities) {
			if (ent->alive) {
				modelviewMatrix.Identity();
				//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
				program.SetModelviewMatrix(modelviewMatrix);
				program.SetProjectionMatrix(projectionMatrix);
				glUseProgram(program.programID);
				ent->Draw(program);
			}
		}
		for (Entity*& ent : state->bgEntities) {
			modelviewMatrix.Identity();
			//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
			program.SetModelviewMatrix(modelviewMatrix);
			program.SetProjectionMatrix(projectionMatrix);
			glUseProgram(program.programID);
			ent->Draw(program);
		}
	}

	SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
	SDL_Quit();
}