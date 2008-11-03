#ifndef UFOATTACK_GAME_INCLUDED
#define UFOATTACK_GAME_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/engine.h"
#include "../engine/surface.h"
#include "../engine/model.h"

class Engine;
class Surface;


const float FOV = 45.0f;
const int MAX_TEXTURES = 32;
const int MAX_MODELS = 256;


class Game
{
private:
	EngineData engineData;

public:
	Game();
	~Game();

	void DoTick( U32 msec );

	int NumTextures()	{ return nTexture; }

	Texture texture[MAX_TEXTURES];
	ModelResource modelResource[MAX_MODELS];
	Engine engine;
	Surface surface;

private:
	void LoadTextures();
	void FreeTextures();
	void LoadModels();
	void FreeModels();

	int nTexture;
	int nModelResource;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	Model* testModel;
};

#endif
