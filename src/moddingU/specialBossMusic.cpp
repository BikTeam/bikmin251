#include "Dolphin/string.h"
#include "PSSystem/PSBgm.h"
#include "PSGame/PikScene.h"
#include "PSM/EnemyBoss.h"
#include "PSM/BossBgmFader.h"
#include "Game/EnemyBase.h"

namespace PSGame {

static bool sIsSpecialBoss = false;
static const char* s_boss  = "new_20.bms";

// isSpecialBossBms__6PSGameFPCc
bool isSpecialBossBms(const char* str) { 
	bool out = strcmp(str, s_boss) == 0; 
	OSReport("special bms %i (%s, %s)\n", out, str, s_boss);
	return out;
}

// isSpecialBoss__6PSGameFv
bool isSpecialBoss() { return sIsSpecialBoss; }

// setSpecialBossActive__6PSGameFPQ24Game9EnemyBase
void setSpecialBossActive(Game::EnemyBase* obj)
{
	if (obj) {
		int id         = obj->getEnemyTypeID();
		sIsSpecialBoss = id == Game::EnemyTypeID::EnemyID_DangoMushi || id == Game::EnemyTypeID::EnemyID_Houdai;
	}
}

// createSpecialBossBgm__6PSGameFPQ26PSGame11PikSceneMgrRQ27JAInter9SoundInfoPUc
PSSystem::DirectedBgm* createSpecialBossBgm(PikSceneMgr* mgr, JAInter::SoundInfo& soundInfo, u8* wScene)
{
	OSReport("creation is special %i\n", isSpecialBoss());
	*wScene = 57; // wScene57 (special boss)
	return (PSSystem::DirectedBgm*)mgr->newDirectedBgm("new_20.bms", soundInfo);
}

} // namespace PSGame
