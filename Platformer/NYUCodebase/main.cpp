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
	SheetSprite() {};
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
	Entity(ENTITY_TYPE type, SheetSprite sprite, float x, float y) : position(x, y, 0.0f), size(1.0f, 1.0f, 1.0f), sprite(sprite), rotation(0.0f), type(type){
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
	int** levelData;
	int levelWidth;
	int levelHeight;
	std::map<int, bool> solids;
	STATE_TYPE type;
	std::map<int, SheetSprite> sprites;
};

/**********************************************
 **********************************************
 *******    Function Declarations    **********
 **********************************************
 **********************************************/


GLuint LoadTexture(const char *filePath);

float randf(float a, float b);

std::array<float, 4> pxToUV(int sheetWidth, int sheetHeight, int xPx, int yPx, int width, int height);

bool readHeader(std::ifstream &stream, GameState* state);

bool readLayerData(std::ifstream &stream, GameState* state);

bool readEntityData(std::ifstream &stream, GameState* state, int TILE_SIZE);

void placeEntity(std::string type, GameState* state, float placeX, float placeY);

ShaderProgram Setup();

std::vector<GameState*> Instantiate();

void ProcessEvents(SDL_Event& event, bool& done, GameState* currentState, std::vector<GameState*>& states);

void Update(GameState* state, float elapsed);

void Render(Matrix& projectionMatrix, Matrix& modelMatrix, Matrix& viewMatrix, ShaderProgram& program, GameState* state);

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
	projectionMatrix.SetOrthoProjection(-7.1f, 7.1f, -4.0f, 4.0f, -1.0f, 1.0f);
	Matrix modelMatrix;
	Matrix viewMatrix;

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
		Render(projectionMatrix, modelMatrix, viewMatrix, program, currentState);
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

bool isCollidingRect(Entity* r1, Vector position, Vector size) {
	// Is r1 bottom > r2 top?
	if (r1->position.y - r1->size.y / 2 > position.y + size.y / 2) return false;
	// Is r1 top < r2 bottom?
	if (r1->position.y + r1->size.y / 2 < position.y - size.y / 2) return false;
	// Is r1 left > r2 right?
	if (r1->position.x - r1->size.y / 2 > position.x + size.y / 2) return false;
	// Is r1 right < r2 left?
	if (r1->position.x + r1->size.y / 2 < position.x - size.y / 2) return false;

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

bool readHeader(std::ifstream &stream, GameState* state) {
	std::string line;
	state->levelWidth = -1;
	state->levelHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		std::istringstream sStream(line);
		std::string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			state->levelWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			state->levelHeight = atoi(value.c_str());
		}
	}
	if (state->levelWidth == -1 || state->levelHeight == -1) {
		return false;
	}
	else { // allocate our map data
		state->levelData = new int*[state->levelHeight];
		for (int i = 0; i < state->levelHeight; ++i) {
			state->levelData[i] = new int[state->levelWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream &stream, GameState* state) {
	std::string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		std::istringstream sStream(line);
		std::string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < state->levelHeight; y++) {
				getline(stream, line);
				std::istringstream lineStream(line);
				std::string tile;
				for (int x = 0; x < state->levelWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						state->levelData[y][x] = val - 1;
					}
					else {
						state->levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(std::ifstream &stream, GameState* state, int TILE_SIZE) {
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
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, state, placeX, placeY);
		}
	}
	return true;
}

void placeEntity(std::string type, GameState* state, float placeX, float placeY) {
	Entity::ENTITY_TYPE entityType;
	if (type == "Player") entityType = Entity::ENTITY_PLAYER;
	else if (type == "Snail") entityType = Entity::ENTITY_SNAIL;
	state->entities.push_back(new Entity(entityType, state->sprites[entityType], placeX, placeY));
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
	glClearColor(94.0f/256, 129.0f/256, 162.0f/256, 0.0f);

	return *program;
}

std::vector<GameState*> Instantiate() {
	std::vector<GameState*> states;
	states.push_back(new GameState(GameState::STATE_GAME));
	states[0]->sprites[Entity::ENTITY_PLAYER] = SheetSprite(LoadTexture("spritesheet.png"), pxToUV(694, 372, 3 + 21 * 19 + 19 * 2, 3 + 21 * 0, 21, 21), 1.0f);
	states[0]->sprites[Entity::ENTITY_SNAIL] = SheetSprite(LoadTexture("spritesheet.png"), pxToUV(694, 372, 3 + 21 * 14 + 2 * 14, 3 + 21 * 15 + 2*15, 21, 21), 1.0f);
	states[0]->solids[124 - 1] = true;
	states[0]->solids[127 - 1] = true;
	states[0]->solids[126 - 1] = true;
	states[0]->solids[156 - 1] = true;
	states[0]->solids[125 - 1] = true;
	states[0]->solids[159 - 1] = true;
	states[0]->solids[160 - 1] = true;
	states[0]->solids[71 - 1] = true;
	states[0]->solids[130 - 1] = true;
	states[0]->solids[279 - 1] = true;
	states[0]->solids[244 - 1] = true;
	states[0]->solids[278 - 1] = true;

	// Load background and textures
	std::string const levelFile = ("MapFlare.txt");

	int mapWidth = 40;
	int mapHeight = 15;
	std::ifstream infile(levelFile);
	std::string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile, states[0])) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile, states[0]);
		}
		else if (line == "[Object Layer]") {
			readEntityData(infile, states[0], 1);
		}
	}
	return states;
}

void ProcessEvents(SDL_Event & event, bool& done, GameState* currentState, std::vector<GameState*>& states){
	// SDL Event Loop
	while (SDL_PollEvent(&event)) {
		// Quit or Close Event
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event.type == SDL_KEYDOWN) {
			if (currentState->type == GameState::STATE_MENU) {
				currentState = states[1];
			}
			else if (currentState->type == GameState::STATE_GAME && currentState->entities[0]->alive) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					currentState->entities[0]->velocity.y = 5;
				}
			}
			
		}
	}

	// Keyboard Polling
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RIGHT] && currentState->entities[0]->position.x + currentState->entities[0]->size.x / 2 < 40) {
		currentState->entities[0]->velocity.x = 2;
	}
	if (keys[SDL_SCANCODE_LEFT] && currentState->entities[0]->position.x - currentState->entities[0]->size.x / 2 > 0.0f) {
		currentState->entities[0]->velocity.x = -2;
	}
	


}

void Update(GameState* state, float elapsed) {
	state->entities[1]->velocity.x = -1;
	for (Entity*& ent : state->entities) {
		ent->acceleration.y = -5.0f;
		if (ent->alive) {
			
			ent->velocity.x = lerp(ent->velocity.x, 0.0f, elapsed);
			ent->velocity.x += ent->acceleration.x*elapsed;
			ent->position += ent->velocity * elapsed;

			ent->velocity.y += ent->acceleration.y * elapsed;
			for (int y = 0; y < state->levelHeight; ++y) {
				for (int x = 0; x < state->levelWidth; ++x) {
					if (state->solids[state->levelData[y][x]]) {
						Vector pos(x + 0.5f, -y - 0.5f, 0.0f);
						Vector size(1.0f, 1.0f, 0.0f);
						if (isCollidingRect(ent, pos, size)) {
							ent->position.y += ((pos.y + size.y / 2) - (ent->position.y - ent->size.y / 2))*elapsed;
							//ent->position.x += ((pos.x + size.x / 2) - (ent->position.x - ent->size.x / 2))*elapsed;
							ent->velocity.y = 0;
							if (state->levelData[y][x] == 70) ent->alive = false;
						}
					}
				}
			}
			Entity* player = state->entities[0];
			if(ent != state->entities[0] && isCollidingRect(player, ent)) {
				if(player->position.y - (player->size.y/2) >= ent->position.y + (ent->size.y/2) - 0.1f && player->velocity.y < 0) {
					ent->alive = false;
					player->velocity.y = 5;
				}
				else {
					player->alive = false;
				}
				//else if(ent1 == state->e)
			}
			
		}
	}
}



void Render(Matrix& projectionMatrix, Matrix& modelMatrix, Matrix& viewMatrix, ShaderProgram& program, GameState* state) {
	glClear(GL_COLOR_BUFFER_BIT);
	modelMatrix.Identity();
	viewMatrix.Identity();
	if(state->entities[0]->position.x < 7.1) viewMatrix.Translate(-7.1f, 0.0f, 0.0f);
	else if (state->entities[0]->position.x > 40-7.1) viewMatrix.Translate(-(40-7.1), 0.0f, 0.0f);
	else viewMatrix.Translate(-state->entities[0]->position.x, 0.0f, 0.0f);
	if (state->entities[0]->position.y < -9.0) viewMatrix.Translate(0.0f, 9.0f, 0.0f);
	else viewMatrix.Translate(0.0f, -state->entities[0]->position.y, 0.0f);

	//viewMatrix.Translate(0.0f, 0.1f, 0.0f);
	program.SetModelviewMatrix(modelMatrix*viewMatrix);
	program.SetProjectionMatrix(projectionMatrix);
	glUseProgram(program.programID);
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	float TILE_SIZE = 1.0f;
	int SPRITE_COUNT_X = 30;
	int SPRITE_COUNT_Y = 16;
	// if index 0 is an empty tile
	for (int y = 0; y < state->levelHeight; y++) {
		for (int x = 0; x < state->levelWidth; x++) {
			if (state->levelData[y][x]) {
				// add vertices
				float spriteWidth = 21.0f / 694.0f;
				float spriteHeight = 21.0f / 372.0f;
				float u = (3 + (23 * ((int)state->levelData[y][x] % SPRITE_COUNT_X))) / 694.0f;
				float v = (3 + (23 * ((int)state->levelData[y][x] / SPRITE_COUNT_X))) / 372.0f;
				
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}


	float* vertices = &vertexData[0];
	float* tex = &texCoordData[0];

	//program.SetModelviewMatrix(modelMatrix*viewMatrix);
	//program.SetProjectionMatrix(projectionMatrix);
	//glUseProgram(program.programID);
	glBindTexture(GL_TEXTURE_2D, 1);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, tex);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	for (Entity*& ent : state->entities) {
		if (ent->alive) {
			//modelviewMatrix.Translate(ent->direction_x, ent->direction_y, 0.0f);
			ent->Draw(program);
		}
	}

	SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
	SDL_Quit();
}