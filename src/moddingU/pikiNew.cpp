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
#include "TwoPlayer.h"

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

// isPiki2PlayerOwned__4GameFPQ24Game4PikiPQ24Game4Navi
// used in interactPiki.s
bool isPiki2PlayerOwned(Piki* piki, Navi* owner)
{
	// only applies to 2 player modes
	if (!TwoPlayer::twoPlayerActive && !gameSystem->isMultiplayerMode()) {
		return false;
	}

	int action = piki->getCurrActionID();
	if (action == PikiAI::ACT_Attack || action == PikiAI::ACT_Bridge || action == PikiAI::ACT_BreakGate
	    || action == PikiAI::ACT_BreakRock) {
		// unclaimed pikmin can be claimed freely
		if (piki->m_navi == nullptr) {
			return false;
		}

		// if the owner is not the current owner, it cannot be claimed
		if (piki->m_navi != owner) {
			return true;
		}

		// if player is outside search range, it can be claimed by other players
		Vector3f naviPos = owner->getPosition();
		f32 sqrDist      = piki->getPosition().sqrDistance2D(naviPos);

		PikiParms* parms      = static_cast<PikiParms*>(piki->m_parms);
		f32 playerSearchRange = parms->m_pikiParms.m_p036.m_value;
		if (sqrDist > SQUARE(playerSearchRange)) {
			piki->m_navi = owner;
		}
	}

	// pikmin outside selected actions or pikmin already owned by the given navi can be claimed freely
	return false;
}

// I deleted the asm like an idiot so this is here without changes
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
}

// allow pikmin to be whistled out of drinking nectar
void PikiAbsorbState::onFlute(Piki* piki, Navi* navi)
{
	if (mHasAbsorbed && (piki->m_navi == navi || piki->m_navi == nullptr)) {
		if (piki->m_pikiKind != Blue && piki->m_happaKind != Flower) {
			piki->m_happaKind = Flower;
			navi->m_cPlateMgr->changeFlower(piki);
		}
		transit(piki, PIKISTATE_LookAt, nullptr);
	}
}

// I deleted the asm like an idiot so this is here without changes
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
}

// allow pikmin to be whistled out of growup
void PikiGrowupState::onFlute(Piki* piki, Navi* navi)
{
	if (piki->m_navi == navi || piki->m_navi == nullptr) {
		if (piki->m_pikiKind != Blue && piki->m_happaKind != Flower) {
			piki->m_happaKind = Flower;
			navi->m_cPlateMgr->changeFlower(piki);
		}
		transit(piki, PIKISTATE_LookAt, nullptr);
	}
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
