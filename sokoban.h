/* date = December 15th 2020 9:34 am */

#ifndef SOKOBAN_H
#define SOKOBAN_H

struct Level {
    enum Tile_Types {
        WALL,
        FLOOR,
        PLAYER,
        BOX,
        DESTINATION,
        DESTINATION_BOX,
        NULL_TILE,
        NUM_TILES,
    };
    int startingX;
    int startingY;
    int w;
    int h;
    int* level;
};

#endif //SOKOBAN_H
