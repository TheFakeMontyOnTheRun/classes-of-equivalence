#ifndef ACTOR_H
#define ACTOR_H

struct CActor {
    enum EDirection rotation;
    struct Vec2i position;
    uint8_t hp;
    uint8_t energy;
};

#endif
