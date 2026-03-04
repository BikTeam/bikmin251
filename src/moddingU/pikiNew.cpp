#include "Dolphin/rand.h"
#include "Game/Piki.h"
#include "Game/PikiState.h"
#include "Game/Navi.h"
#include "Game/GameSystem.h"
#include "Game/CPlate.h"
#include "Game/Entities/ItemHoney.h"
#include "types.h"
#include "Game/Entities/PelletOtakara.h"
#include "PikiAI.h"
#include "Game/gameStat.h"
#include "efx/TPk.h"
#include "Game/PikiMgr.h"
#include "PSM/Navi.h"

namespace Game {

// make blue pikmin stay leaves, except in 2P-Battle
void leafBluePikmin(Piki& pikmin)
{
	if (gameSystem) {
		if (gameSystem->m_mode == GSM_VERSUS_MODE) {
			return;
		}
	}
	if (pikmin.m_pikiKind == Blue && pikmin.m_happaKind > 0) {
		pikmin.m_happaKind = Leaf;
	}
}

void PikiAbsorbState::init(Piki* piki, StateArg* stateArg)
{
	AbsorbStateArg* absorbArg = static_cast<AbsorbStateArg*>(stateArg);
	P2ASSERTLINE(4210, absorbArg);
	mAbsorbingCreature = absorbArg->mCreature;
	P2ASSERTLINE(4212, mAbsorbingCreature);
	piki->startMotion(IPikiAnims::MIZUNOMI, IPikiAnims::MIZUNOMI, piki, nullptr);
	mState             = 0;
	mHasAbsorbed       = 0;
	Vector3f targetPos = mAbsorbingCreature->getPosition();
	piki->turnTo(targetPos);

	P2ASSERTLINE(4219, mAbsorbingCreature->getJAIObject());
	piki->m_soundObj->startPikiSound(mAbsorbingCreature->getJAIObject(), PSSE_PK_VC_DRINK, 0);
	mAbsorbTimer = 0;

	// assign piki ref
	mPiki = piki;
}

// allow pikmin to be whistled out of drinking nectar
bool PikiAbsorbState::callable()
{
	if (mHasAbsorbed == true) {
		if (mPiki->m_happaKind != Flower) {
			mPiki->m_happaKind = Flower;
			mPiki->m_navi->m_cPlateMgr->changeFlower(mPiki);
		}
		transit(mPiki, PIKISTATE_LookAt, nullptr);
	}

	return mHasAbsorbed;
}

void PikiGrowupState::init(Piki* piki, StateArg* stateArg)
{
	if (randFloat() > 0.5f) {
		mAnimIdx = IPikiAnims::GROWUP1;
	} else {
		mAnimIdx = IPikiAnims::GROWUP2;
	}

	piki->startMotion(mAnimIdx, mAnimIdx, piki, nullptr);

	if (!piki->assertMotion(mAnimIdx)) {
		transit(piki, PIKISTATE_Walk, nullptr);
	}

	// assign piki reference
	mPiki = piki;
}

// allow pikmin to be whistled out of growup
bool PikiGrowupState::callable()
{
	if (mPiki->m_happaKind != Flower) {
		mPiki->m_happaKind = Flower;
		mPiki->m_navi->m_cPlateMgr->changeFlower(mPiki);
	}

	transit(mPiki, PIKISTATE_LookAt, nullptr);
	return true;
}

// canAbsorbHoney__4GameFPQ24Game7PikiFSMPQ24Game4PikiiPQ24Game14AbsorbStateArg
void canAbsorbHoney(PikiFSM* fsm, Piki* piki, int state, AbsorbStateArg* arg)
{
	ItemHoney::Item* honey = static_cast<ItemHoney::Item*>(arg->mCreature);
	if (honey->isShrinking() && piki->m_pikiKind == Blue) {
		return;
	}

	fsm->transit(piki, state, arg);
}

void PikiPanicState::exec(Piki* piki)
{
	if (m_panicType == PIKIPANIC_Panic || m_panicType == PIKIPANIC_Other) {
		piki->m_velocity = Vector3f(0.0f);
		switch (_22) {
		case 0:
			m_dramaTimer -= sys->m_deltaTime;
			if (m_dramaTimer <= 0.0f) {
				_22 = 1;
				piki->startMotion(IPikiAnims::KIZUKU, IPikiAnims::KIZUKU, piki, nullptr);
			}
			return;

		case 1:
			if (!piki->assertMotion(IPikiAnims::KIZUKU)) {
				_22 = 2;
			}
			return;
		}
	}

	if (m_panicType == PIKIPANIC_Gas) {
		panicLobster(piki);
	} else {
		panicRun(piki);
	}
}

// this is the funniest shit ever, note to anyone who sees this
// this is the only thing standing between bobu and the end of the universe
// navi_demoCheck.s: demoCheck__Q24Game4NaviFv @8022001C
bool forceBobuPelletCutscene() { return true; }

} // namespace Game
