#include "shared_state.h"
#include "sokoban.h"
#include "game.cpp"

struct vec2 {
    float x;
    float y;
};

struct camera {
    vec2 pos;
    float zoom;
};

static inline void DrawQuad(shared_state state, vec2 pos, camera cam, int index, b32 selected);
static inline void* AllocateMemory(size_t size);
static inline void FreeMemory(void* p);
static inline void FileWrite(char* filename, void* data, u32 size);
static inline void* FileRead(char* filename);
static void PlatformSprintfInteger(char* string, char* buffer, int value);

static Level currentLevel = {};

#define VALID_LEVEL_NUM 50

struct engine_state 
{
    int x;
    int y;
    b32 editorMode;
    int selectedTile;
    b32 loadLevel;
    int loadedLevelIndex;
};

#pragma pack(push, 4)
struct level_file_header {
    u32 width;
    u32 height;
};
#pragma pack(pop)

static engine_state EngineInit(shared_state state, engine_state prevEngineState)
{
    engine_state engineState = prevEngineState;
    if (state.reloadProgram || engineState.loadLevel) {
        if (currentLevel.level) {
            FreeMemory(currentLevel.level - sizeof(level_file_header));
        }
        char buffer[64] = {};
        PlatformSprintfInteger("../release/data/%d.level", buffer, engineState.loadedLevelIndex);
        void* data = FileRead(buffer);
        if (!data) {
            data = AllocateMemory(4096);
        }
        if (data) {
            currentLevel.w = (int)((level_file_header*)data)->width;
            currentLevel.h = (int)((level_file_header*)data)->height;
            currentLevel.level = (int*)((char*)data + sizeof(level_file_header));
            for (int x = 0; x < currentLevel.w; ++x) {
                for (int y = 0; y < currentLevel.h; ++y) {
                    int index = currentLevel.level[i2(x, y, currentLevel.w)];
                    if (index == Level::PLAYER) {
                        currentLevel.startingX = x;
                        currentLevel.startingY = y;
                    }
                }
            }
        }
        engineState.loadLevel = false;
        gameState.initialised = false;
    }
    return(engineState);
}

//TODO: Undo function
static engine_state EngineUpdateDraw(shared_state state, engine_state prevEngineState)
{
    engine_state engineState = prevEngineState;
    
    if (engineState.editorMode) {
        if (state.input.up && !state.prevInput.up)  {
            engineState.y = Clamp(engineState.y + 1, 0, currentLevel.h-1);
        }
        if (state.input.down && !state.prevInput.down)  {
            engineState.y = Clamp(engineState.y - 1, 0, currentLevel.h-1);
        }
        if (state.input.left && !state.prevInput.left)  {
            engineState.x = Clamp(engineState.x - 1, 0, currentLevel.w-1);
        }
        if (state.input.right && !state.prevInput.right)  {
            engineState.x = Clamp(engineState.x + 1, 0, currentLevel.w-1);
        }
        if (state.input.space && !state.prevInput.space) {
            currentLevel.level[i2(engineState.x, engineState.y, currentLevel.w)] = engineState.selectedTile;
        }
        if (state.input.decimal && !state.prevInput.decimal) {
            engineState.selectedTile = Clamp(engineState.selectedTile + 1, 0, Level::NUM_TILES-1);
        }
        if (state.input.comma && !state.prevInput.comma) {
            engineState.selectedTile = Clamp(engineState.selectedTile - 1, 0, Level::NUM_TILES-1);
        }
    } else {
        currentLevel = GameUpdate(state, currentLevel);
    }
    
    if (engineState.editorMode && (state.input.f2 && !state.prevInput.f2)) {
        //TODO: Write file header which describes dimensions
        u32 levelSize = (currentLevel.w * currentLevel.h * sizeof(u32));
        u32 dataSize = levelSize + sizeof(level_file_header);
        void* data = AllocateMemory(dataSize);
        level_file_header* headerPointer = (level_file_header*)data;
        headerPointer->width = (u32)currentLevel.w;
        headerPointer->height = (u32)currentLevel.h;
        for (u32 i = 0; i < levelSize; ++i) {
            ((char*)((char*)data + sizeof(level_file_header)))[i] = ((char*)currentLevel.level)[i];
        }
        
        char buffer[64] = {};
        PlatformSprintfInteger("../release/data/%d.level", buffer, engineState.loadedLevelIndex);
        FileWrite(buffer, data, dataSize);
    }
    
    if (state.input.f1 && !state.prevInput.f1)  {
        engineState.editorMode = !engineState.editorMode;
    }
    
    if (state.input.rightBrace && !state.prevInput.rightBrace)  {
        engineState.loadedLevelIndex = Clamp(engineState.loadedLevelIndex+1, 0, VALID_LEVEL_NUM);
        engineState.loadLevel = true;
    }
    
    if (state.input.leftBrace && !state.prevInput.leftBrace)  {
        engineState.loadedLevelIndex = Clamp(engineState.loadedLevelIndex-1, 0, VALID_LEVEL_NUM);
        engineState.loadLevel = true;
    }
    
    camera c = {};
    c.pos.x = currentLevel.w/2.0f - 0.5f;
    c.pos.y = currentLevel.h/2.0f - 0.5f;
    for (int x = 0; x < currentLevel.w; ++x) {
        for (int y = 0; y < currentLevel.h; ++y) {
            int index = currentLevel.level[i2(x, y, currentLevel.w)];
            b32 selected = (engineState.editorMode) ? (b32)((engineState.x==x) && (engineState.y==y)) : (b32)false;
            if (index != Level::NULL_TILE || selected) {
                vec2 p = vec2{ (float)x, (float)y };
                c.zoom = (float)(1.0f / Max((currentLevel.w*9.0/16.0f), currentLevel.h)) * 1.8f;
                if (index!=Level::NULL_TILE && index!=Level::WALL && index!=Level::FLOOR) {
                    DrawQuad(state, p, c, Level::FLOOR, 0);
                }
                if (engineState.editorMode && selected) {
                    index = engineState.selectedTile;
                }
                DrawQuad(state, p, c, index, selected);
            }
        }
    }
    return(engineState);
}