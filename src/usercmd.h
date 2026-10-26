#pragma once

#include "types.h"

constexpr u64 inforward = 1ull << 3;
constexpr u64 inback = 1ull << 4;
constexpr u64 inmoveleft = 1ull << 9;
constexpr u64 inmoveright = 1ull << 10;
constexpr u64 inmove = inforward | inback | inmoveleft | inmoveright;

template <class t>
struct repeatedfield {
    struct storage {
        int allocated;
        t* elements[32];
    };

    void* arena;
    int size;
    int capacity;
    storage* data;
};

struct protobuf {
    void* vtable;
    u64 metadata;
    u32 bits;
    u32 cachedsize;
};

struct subtickstep : protobuf {
    u64 button;
    bool pressed;
    float when;
    float forward;
    float left;
    float pitch;
    float yaw;
};

struct basecmd : protobuf {
    repeatedfield<subtickstep> moves;
    void* movecrc;
    void* buttons;
    void* viewangles;
    void* notes;
    int commandnumber;
    int clienttick;
    float forwardmove;
    float leftmove;
    float upmove;
    u8 rest[0x24];
};

struct csgocmd : protobuf {
    repeatedfield<void> history;
    basecmd* base;
    u8 rest[0x10];
};

struct buttonstate {
    void* vtable;
    u64 value;
    u64 changed;
    u64 scroll;
};

struct usercmd {
    void* vtable;
    int sequence;
    csgocmd pb;
    buttonstate buttons;
};

struct csgoinput {
    u8 pad0[0x284];
    int subtickcount;
    u8 pad1[0x400];
    vec3 viewangles;
};
