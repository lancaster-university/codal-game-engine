#ifndef CODAL_PLAYER_H
#define CODAL_PLAYER_H

#include "GameEngine.h"

namespace codal
{
    class Player
    {
        public:

        static int playerNumber;

        static int setX(int x);

        static int setY(int y);

        static int getX();

        static int getY();
    };
}

#endif