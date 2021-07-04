#include "shared_state.h"
#include "sokoban.h"

struct game_state
{
    b32 initialised;
    int underneathTileIndex;
    int playerX;
    int playerY;
};

struct BlockType {
    enum Enum {
        NOT_WALKABLE,
        WALKABLE,
        PUSHABLE,
    };
};

static BlockType::Enum GetBlockType(int tileIndex)
{
    BlockType::Enum result = BlockType::NOT_WALKABLE;
    switch (tileIndex) {
        case Level::BOX:
        case Level::DESTINATION_BOX: {
            result = BlockType::PUSHABLE;
        } break;
        case Level::DESTINATION:
        case Level::FLOOR: {
            result = BlockType::WALKABLE;
        } break;
    }
    return(result);
}

static game_state MovePlayer(game_state gameStateIn, Level level, int dirX, int dirY)
{
    game_state gameState = gameStateIn;
    int x = gameState.playerX + dirX;
    int y = gameState.playerY + dirY;
    if (x >= 0 && x < level.w && y >= 0 && y < level.h) {
        BlockType::Enum blockType = GetBlockType(level.level[i2(x, y, level.w)]);
        if (blockType == BlockType::WALKABLE) {
            level.level[i2(gameState.playerX, gameState.playerY, level.w)] = gameState.underneathTileIndex;
            gameState.underneathTileIndex = level.level[i2(x, y, level.w)];
            level.level[i2(x, y, level.w)] = Level::PLAYER;
            gameState.playerX = x;
            gameState.playerY = y;
        } else if (blockType == BlockType::PUSHABLE) {
            int newBlockX = x + dirX;
            int newBlockY = y + dirY;
            BlockType::Enum destBlockType = GetBlockType(level.level[i2(newBlockX, newBlockY, level.w)]);
            if (destBlockType == BlockType::WALKABLE) {
                level.level[i2(gameState.playerX, gameState.playerY, level.w)] = gameState.underneathTileIndex;
                int box = level.level[i2(x, y, level.w)];
                if (box == Level::BOX) {
                    level.level[i2(x, y, level.w)] = Level::FLOOR;
                } else {
                    level.level[i2(x, y, level.w)] = Level::DESTINATION;
                }
                gameState.underneathTileIndex = level.level[i2(x, y, level.w)];
                level.level[i2(x, y, level.w)] = Level::PLAYER;
                gameState.playerX = x;
                gameState.playerY = y;
                int newBox = level.level[i2(newBlockX, newBlockY, level.w)];
                if (newBox == Level::DESTINATION) {
                    level.level[i2(newBlockX, newBlockY, level.w)] = Level::DESTINATION_BOX;
                } else {
                    level.level[i2(newBlockX, newBlockY, level.w)] = Level::BOX;
                }
            }
        }
        
    }
    return(gameState);
}

static game_state gameState = {};
static Level GameUpdate(shared_state state, Level loadedLevel)
{
    Level level = loadedLevel;
    if (!gameState.initialised) {
        gameState.playerX = level.startingX;
        gameState.playerY = level.startingY;
        gameState.underneathTileIndex = level.level[i2(gameState.playerX, gameState.playerY, level.w)];
        if (gameState.underneathTileIndex == Level::PLAYER) {
            gameState.underneathTileIndex = Level::FLOOR;
        }
        gameState.initialised = true;
        gameState = MovePlayer(gameState, level, 0, 0);
    }
    
    if (state.input.up && !state.prevInput.up) {
        gameState = MovePlayer(gameState, level, 0, 1);
    }
    if (state.input.down && !state.prevInput.down) {
        gameState = MovePlayer(gameState, level, 0, -1);
    }
    if (state.input.left && !state.prevInput.left) {
        gameState = MovePlayer(gameState, level, -1, 0);
    }
    if (state.input.right && !state.prevInput.right) {
        gameState = MovePlayer(gameState, level, 1, 0);
    }
    
    return(level);
}