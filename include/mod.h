#ifndef _MOD_H
#define _MOD_H

#include "og/newScreen/Cave.h"

namespace mod {
extern int keyLockCount;
extern og::newScreen::ObjCave* thisObjCave;
extern bool isExitLocked;
extern bool isBobuMovieQueued;

// adjust treasure culling radius to 50.0f if radius was zero
float adjustBoundingRadius(float radius);

void updateDispMember();

}; // namespace mod

#endif
