#pragma once
#include <Preferences.h>
#include <string.h>

#define MAX_MULTI_SCENES  36
#define MAX_MULTI_SUBCMDS  8
#define MULTI_NAME_LEN    12

struct MultiSubCmd {
    uint8_t modeIndex;   // index into modes[]
    uint8_t midiNumber;  // 0-based CC/PC/Note/scene value
    uint8_t channel;     // 0xFF = global, 0–15 = per-sub override
};

struct MultiScene {
    char        name[MULTI_NAME_LEN]; // name[0]=='\0' → empty slot
    uint8_t     count;                // active sub-commands (0–MAX_MULTI_SUBCMDS)
    uint8_t     _pad[3];
    MultiSubCmd subCmds[MAX_MULTI_SUBCMDS];
};
// sizeof = 12+1+3+24 = 40 bytes;  36*40 = 1440 bytes NVS

inline MultiScene multiScenes[MAX_MULTI_SCENES];

inline void saveMultiScenes() {
    Preferences p;
    p.begin("multis", false);
    p.putBytes("ms", multiScenes, sizeof(multiScenes));
    p.end();
}

inline void loadMultiScenes() {
    Preferences p;
    p.begin("multis", true);
    size_t sz = p.getBytesLength("ms");
    if(sz == sizeof(multiScenes))
        p.getBytes("ms", multiScenes, sizeof(multiScenes));
    else
        memset(multiScenes, 0, sizeof(multiScenes));
    p.end();
}

inline uint8_t countMultiScenes() {
    uint8_t n = 0;
    for(uint8_t i = 0; i < MAX_MULTI_SCENES; i++)
        if(multiScenes[i].name[0] != '\0') n++;
    return n;
}

inline uint8_t findEmptyMultiSlot() {
    for(uint8_t i = 0; i < MAX_MULTI_SCENES; i++)
        if(multiScenes[i].name[0] == '\0') return i;
    return 0xFF;
}

// First non-empty slot index, or 0xFF if none
inline uint8_t firstMultiSlot() {
    for(uint8_t i = 0; i < MAX_MULTI_SCENES; i++)
        if(multiScenes[i].name[0] != '\0') return i;
    return 0xFF;
}
