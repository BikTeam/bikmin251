#include "Game/NaviState.h"
#include "Game/Navi.h"
#include "Game/CameraMgr.h"
#include "Game/MapMgr.h"
#include "Game/NaviParms.h"
#include "Game/Stickers.h"
#include "Game/MoviePlayer.h"
#include "Game/AIConstants.h"
#include "Game/Entities/ItemOnyon.h"
#include "Game/EnemyFunc.h"
#include "Game/PikiMgr.h"
#include "Game/Footmark.h"
#include "Dolphin/rand.h"
#include "PikiAI.h"
#include "Game/Entities/ItemPikihead.h"
#include "Game/Entities/ItemHole.h"
#include "Game/Entities/ItemCave.h"
#include "Game/rumble.h"
#include "PSM/Navi.h"
#include "Game/PikiState.h"
#include "PSSystem/PSSystemIF.h"
#include "KandoLib/Choice.h"
#include "nans.h"
#include "Game/CPlate.h"
#include "Screen/Game2DMgr.h"
#include "utilityU.h"
#include "JSystem/JUT/JUTGamePad.h"
#include "TwoPlayer.h"

namespace Game {

void Navi::findNextThrowPiki()
{
	m_nextThrowPiki = nullptr;
	Iterator<Creature> iterator(m_cPlateMgr);
	f32 minDist = 200.0f;

	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->m_pikiKind != _269) {
			continue;
		}
		Vector3f naviPos = getPosition();
		Vector3f diff    = naviPos - piki->getPosition();
		f32 dist         = diff.qLength2D();
		if (piki->m_navi == this && dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
			m_nextThrowPiki = piki;
			minDist         = dist;
		}
	}
	if (!m_nextThrowPiki) {
		CI_LOOP(iterator)
		{
			Piki* piki       = static_cast<Piki*>(*iterator);
			Vector3f naviPos = getPosition();
			Vector3f diff    = naviPos - piki->getPosition();
			f32 dist         = diff.qLength2D();
			if (piki->m_navi == this && dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
				m_nextThrowPiki = piki;
				minDist         = dist;
			}
		}
	}
}

bool Navi::releasePikis() { return releasePikis(nullptr, false); }

bool Navi::releasePikis(Piki* discriminator, bool doSplitHalf)
{
	if (!gameSystem->isFlag(2)) {
		return false;
	}

	bool dismissnavi = false;
	Navi* loozy      = naviMgr->getAt(1 - m_naviIndex);
	int id           = loozy->getStateID();
	if (id == NSID_Follow) {
		dismissnavi = true;
	}
	InteractKaisan act(this);
	loozy->stimulate(act);

	u32 pikis = 0;
	Piki* baseBuffer[100];
	Piki** buffer = baseBuffer;

	u32 happaPikis = 0;
	Piki* happaBuffer[100];
	bool isSeenDifferentType = false;

	Iterator<Creature> iterator(m_cPlateMgr);
	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->m_currentState && !piki->m_currentState->releasable() || !piki->isAlive()) {
			continue;
		}

		if (discriminator == nullptr) {
			isSeenDifferentType = true;
			baseBuffer[pikis++] = piki;
			continue;
		}

		if (piki->m_pikiKind == discriminator->m_pikiKind) {
			if (piki->m_happaKind == discriminator->m_happaKind) {
				continue;
			}
			happaBuffer[happaPikis++] = piki;
			continue;
		}

		isSeenDifferentType = true;
		baseBuffer[pikis++] = piki;
	}

	if (!isSeenDifferentType) {
		buffer = happaBuffer;
		pikis  = happaPikis;
	}

	m_soundObj->playKaisanSE();

	if (pikis == 0) {
		return dismissnavi;
	}

	int number[8];
	Vector3f position[8];
	for (int i = 0; i != 8; i++) {
		position[i] = 0;
		number[i]   = 0;
	}

	int sortCount = 0;
	Piki* sortedBuffer[100];
	for (int cColor = 0; cColor < 8; cColor++) {
		for (int i = 0; i < pikis; i++) {
			// // sort by color
			if (doSplitHalf && cColor == buffer[i]->m_pikiKind) {
				sortedBuffer[sortCount++] = buffer[i];
			}

			if (cColor != Yellow) {
				if (cColor == buffer[i]->m_pikiKind) {
					number[cColor]++;
					position[cColor] += buffer[i]->getPosition();
				} // WHY WHAT WHY / WHY IS YELLOW SPECIAL
			} else if (buffer[i]->m_pikiKind == Yellow) {
				number[Yellow]++;
				position[Yellow] += buffer[i]->getPosition();
			}
		}
	}

	if (doSplitHalf) {
		// there can only be at most 50 within this array
		int count = 0;
		Piki* splitBuffer[50];
		for (int i = 0; i < pikis; i++) {
			if (i % 2 == 0) {
				splitBuffer[count++] = sortedBuffer[i];
			}
		}

		pikis /= 2;
		buffer = splitBuffer;
	}

	f32 distList[8];
	for (int cColor = 0; cColor < 8; cColor++) {
		if (number[cColor] > 0) {
			f32 num  = number[cColor];
			f32 mean = 1.0f / number[cColor];
			position[cColor] *= mean;
			distList[cColor] = pikmin2_sqrtf(num) * 6.25f;
		}
	}

	loozy = naviMgr->getAt(1 - m_naviIndex);
	for (int i = 0; i < 4; i++) {
		for (int cColor = 0; cColor < 8; cColor++) {
			if (number[cColor] > 0) {
				Vector3f naviPos = getPosition();
				Vector3f diff    = position[cColor] - naviPos;
				f32 dist         = diff.qNormalise();
				dist             = dist - distList[cColor] - 25.0f;
				if (dist < 20.0f) {
					dist = 20.0f - dist;
					position[cColor] += diff * dist;
				}
				if (loozy->isAlive()) {
					Vector3f naviPos = loozy->getPosition();
					Vector3f diff    = position[cColor] - naviPos;
					f32 dist         = diff.qNormalise();
					dist             = dist - distList[cColor] - 25.0f;
					if (dist < 20.0f) {
						dist = 20.0f - dist;
						position[cColor] += diff * dist;
					}
				}
			}

			for (int j = cColor + 1; j < 8; j++) {
				if (number[cColor] > 0 && number[j] > 0) {
					Vector3f diff = position[cColor] - position[j];
					f32 dist      = diff.qNormalise();
					dist          = dist - distList[cColor] - distList[j];
					if (dist < 20.0f) {
						dist = 20.0f - dist;
						position[cColor] += diff * dist;
						position[j] -= diff * dist;
					}
				}
			}
		}
	}

	for (int i = 0; i < pikis; i++) {
		Piki* piki = buffer[i];
		PikiAI::ActFreeArg arg(distList[piki->m_pikiKind], position[piki->m_pikiKind], true);
		buffer[i]->startSound(PSSE_PK_VC_BREAKUP, false);
		buffer[i]->m_brain->start(1, &arg);
	}
	m_disbandTimer = 60;

	return true;
}

void NaviWalkState::exec(Navi* navi)
{
	if (mCollisionTimer) {
		mCollisionTimer--;
	}

	if (navi->isAlive()) {
		navi->control();
		navi->findNextThrowPiki();

		if (!navi->m_padinput && !navi->isMovieActor()) {
			if (mAIState == WALKAI_Control) {
				mAIState   = WALKAI_Wait;
				mIdleTimer = 2.0f;
			}
			execAI(navi);

		} else if (navi->m_padinput && !navi->isMovieActor() && navi->_308 > 9.0f) {
			if (mAIState == WALKAI_Control) {
				initAI_animation(navi);
			}
			execAI(navi);

		} else if (navi->m_padinput && mAIState && navi->_308 <= 9.0f) {
			mAIState = WALKAI_Control;
			navi->startMotion(IPikiAnims::WAIT, IPikiAnims::WAIT, nullptr, nullptr);
		}

		if (!navi->m_padinput || navi->isMovieActor()) {
			// feels like commented out code here.
			navi->isMovieActor();
			return;
		}

		if (moviePlayer->m_demoState == 0) {
			if (navi->m_stick) {
				transit(navi, NSID_Stuck, nullptr);
				return;
			}

			Onyon* onyon = navi->checkOnyon();
			if (onyon && navi->m_padinput->isButtonDown(JUTGamePad::PRESS_A) && onyon->m_onyonType != ONYON_TYPE_POD) {
				NaviContainerArg containerArg(onyon);
				transit(navi, NSID_Container, &containerArg);
				return;
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_B)) {
				transit(navi, NSID_Gather, nullptr);
				return;
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_A)) {
				if (!navi->procActionButton()) {
					if (navi->throwable()) {
						transit(navi, NSID_ThrowWait, nullptr);
					}
					return;
				}
				return;
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_RIGHT)) {
				int color = navi->_269;
				for (int i = 1; i < (PikiColorCount); i++) {
					if ((i + color) > (PikiColorCount - 1)) {
						color = -1 - i;
					}
					if (findNearestColorPiki(navi, (color + i))) {
						PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
						rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
						navi->_269 = color + i;
						sortPikis(navi);
						break;
					}
				}
			} else if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_LEFT)) {
				int color = navi->_269;
				for (int i = 1; i < (PikiColorCount); i++) {
					if ((color - i) < 0) {
						color = PikiColorCount + i;
					}
					if (findNearestColorPiki(navi, (color - i))) {
						PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
						rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
						navi->_269 = color - i;
						sortPikis(navi);
						break;
					}
				}
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_UP)) {
				NaviDopeArg dopeArg(1);
				transit(navi, NSID_Dope, &dopeArg);
				return;
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_DOWN)) {
				NaviDopeArg dopeArg(0);
				transit(navi, NSID_Dope, &dopeArg);
				return;
			}

			if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_X)) {
				if (navi->m_cPlateMgr->_BC < 1) {
					mDismissTimer = 25;
				} else {
					mDismissTimer = 5;
				}
				navi->releasePikis();
			}

			if (navi->m_padinput->isButton(JUTGamePad::PRESS_X)) {
				if (mDismissTimer != 0) {
					mDismissTimer++;

					if (mDismissTimer > 35) {
						mDismissTimer = 0;
						if (playData->m_olimarData->hasItem(OlimarData::ODII_FiveManNapsack) || !gameSystem->isStoryMode()) {
							transit(navi, NSID_Pellet, nullptr);
							return;
						}
					}
				}
			} else {
				mDismissTimer = 0;
			}

			if (navi->m_padinput && navi->m_padinput->isButtonDown(JUTGamePad::PRESS_Y)) {
				if (gameSystem->isMultiplayerMode() || TwoPlayer::twoPlayerActive) {
					navi->releasePikis(nullptr, true);
				} else if (!gameSystem->paused_soft() && moviePlayer->m_demoState == 0
				           && playData->isDemoFlag(DEMO_Unlock_Captain_Switch)) {
					swapNavi(navi);
				}
			}
		}
	}
}

Piki* NaviWalkState::findNearestColorPiki(Navi* navi, int color)
{
	f32 minDist   = 160.0f;
	Piki* retpiki = nullptr;
	Iterator<Creature> iterator(navi->m_cPlateMgr);
	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->m_pikiKind == color) {
			Vector3f diff = piki->getPosition() - navi->getPosition();
			f32 dist      = diff.length();
			if (dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
				retpiki = piki;
				minDist = dist;
			}
		}
	}
	return retpiki;
}

void NaviWalkState::sortPikis(Navi* navi)
{
	Piki* piki2 = findNearestColorPiki(navi, navi->_269);
	if (!piki2) {
		return;
	}
	navi->m_cPlateMgr->sortByColor(piki2, piki2->m_happaKind);

	Vector3f naviPos = navi->getPosition();

	navi->m_cPlateMgr->setPos(naviPos, navi->m_faceDir + PI, navi->m_position2, 1.0f);

	Iterator<Creature> iterator(navi->m_cPlateMgr);
	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->getCurrActionID() == 0) {
			PikiAI::ActFormation* act = static_cast<PikiAI::ActFormation*>(piki->getCurrAction());
			if (act) {
				act->startSort();
			}
		}
	}
}

void NaviThrowWaitState::init(Navi* navi, StateArg* stateArg)
{
	mCurrHappa            = -1;
	mNavi                 = navi;
	NaviParms* parms      = static_cast<NaviParms*>(navi->m_parms);
	f32 minDist           = parms->m_naviParms.m_p021.m_value;
	navi->m_holdPikiTimer = 0.0f;
	Piki* retPiki         = nullptr;
	mHeldPiki             = nullptr;
	mNextPiki             = nullptr;

	if (Piki* piki = findNearestColorPiki(navi, navi->_269)) {
		Vector3f diff        = piki->getPosition() - navi->getPosition();
		Vector3f naviFaceDir = getDirection(navi->m_faceDir);
		f32 dist             = diff.length();
		if (!(absF(diff.y) > 15.0f)) {
			if (diff.dot(naviFaceDir) > -0.1f) {
				dist += 10.0f;
			}
			if (dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
				retPiki    = piki;
				navi->_269 = piki->m_pikiKind;
				minDist    = dist;
			}
		}
	} else {
		Iterator<Creature> iterator(navi->m_cPlateMgr);
		CI_LOOP(iterator)
		{
			Piki* piki = static_cast<Piki*>(*iterator);

			Vector3f diff        = piki->getPosition() - navi->getPosition();
			Vector3f naviFaceDir = getDirection(navi->m_faceDir);
			f32 dist             = diff.length();
			if (!(absF(diff.y) > 15.0f)) {
				if (diff.dot(naviFaceDir) > -0.1f) {
					dist += 10.0f;
				}
				if (dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
					retPiki    = piki;
					navi->_269 = piki->m_pikiKind;
					minDist    = dist;
					break;
				}
			}
		}
	}

	if (minDist <= static_cast<NaviParms*>(navi->m_parms)->m_naviParms.m_p037.m_value) {
		mHeldPiki = retPiki;
	} else {
		mNextPiki = retPiki;
	}

	if (mHeldPiki) {
		navi->m_animSpeed = 30.0f;
		navi->startMotion(IPikiAnims::THROWWWAIT, IPikiAnims::THROWWWAIT, this, nullptr);
		navi->enableMotionBlend();
	} else {
		Piki* piki = mNextPiki;
		if (piki) {
			piki->m_fsm->transit(piki, PIKISTATE_GoHang, 0);
		}
	}
	_20 = false;
	_1C = 0;
	if (mHeldPiki) {
		rumbleMgr->startRumble(RUMBLETYPE_Nudge, mNavi->m_naviIndex);
		mHeldPiki->m_fsm->transit(mHeldPiki, PIKISTATE_Hanged, 0);
		_20 = true;
	}

	navi->_2B4 = _1C / 3.0f * (parms->m_naviParms.m_p010.m_value - parms->m_naviParms.m_p011.m_value) + parms->m_naviParms.m_p011.m_value;
	navi->_2B8 = _1C / 3.0f * (parms->m_naviParms.m_p024.m_value - parms->m_naviParms.m_p025.m_value) + parms->m_naviParms.m_p025.m_value;
	_28        = 1.0f;
	_2C        = 0.05f;
	navi->setDoAnimCallback(mDelegate);
}

Piki* NaviThrowWaitState::getSwitchPiki(Navi* navi)
{
	int currColor = mHeldPiki->m_pikiKind;
	int pikisNext[(PikiColorCount - 1)];
	for (int i = 0; i < (PikiColorCount - 1); i++) {
		pikisNext[i] = ((currColor + i + 1) % PikiColorCount);
	}

	Piki* newPiki = nullptr;
	for (int i = 0; i < (PikiColorCount - 1); i++) {
		Piki* p = findNearestColorPiki(navi, pikisNext[i]);
		if (p) {
			newPiki = p;
			break;
		}
	}

	return newPiki;
}

void NaviThrowWaitState::exec(Navi* navi)
{
	if (moviePlayer && moviePlayer->m_demoState != 0) {
		transit(navi, NSID_Walk, nullptr);
		return;
	}

	if (!navi->m_padinput) {
		return;
	}

	navi->control();

	if (!mHeldPiki) {
		if (mNextPiki) {
			_28 -= sys->m_deltaTime;
			if (_28 < 0.0f) {
				transit(navi, NSID_Walk, nullptr);
				return;
			}

			if (navi->m_padinput->m_padButton.m_buttonDown & Controller::PRESS_B) {
				transit(navi, NSID_Walk, nullptr);
				return;
			}
			CollPart* part   = navi->m_collTree->getCollPart('rhnd');
			Vector3f handPos = part->m_position;
			Vector3f pikiPos = mNextPiki->getPosition();
			f32 dist         = handPos.distance(pikiPos);
			if (!(dist <= 32.5f))
				return;

			navi->m_animSpeed = 30.0f;
			navi->startMotion(IPikiAnims::THROWWWAIT, IPikiAnims::THROWWWAIT, this, nullptr);
			navi->enableMotionBlend();
			mHeldPiki = mNextPiki;
			mNextPiki = nullptr;
			rumbleMgr->startRumble(RUMBLETYPE_Nudge, mNavi->m_naviIndex);
			mHeldPiki->m_fsm->transit(mHeldPiki, PIKISTATE_Hanged, nullptr);
			_20 = true;
		} else {
			transit(navi, NSID_Punch, nullptr);
			return;
		}
	}
	if (mHeldPiki) {
		// navi->m_cursor->mThrowInfo.activate();
		if (navi->m_padinput->m_padButton.m_buttonDown & Controller::PRESS_B) {
			if (mHeldPiki->m_pikiKind == Bulbmin) {
				navi->m_soundObj->stopSound(PSSE_PK_HAPPA_THROW_WAIT, 0);
			} else {
				navi->m_soundObj->stopSound(PSSE_PK_VC_THROW_WAIT, 0);
			}
			mHeldPiki->m_fsm->transit(mHeldPiki, PIKISTATE_Walk, nullptr);
			mHeldPiki = nullptr;
			mNextPiki = nullptr;
			_20       = false;
			transit(navi, NSID_Walk, nullptr);
			return;
		}
	}
	if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_X)) {
		navi->releasePikis(mHeldPiki, false);
	}

	navi->m_nextThrowPiki = mHeldPiki;

	NaviParms* parms = static_cast<NaviParms*>(mNavi->m_parms);

	f32 min    = parms->m_naviParms.m_p011.m_value;
	f32 max    = parms->m_naviParms.m_p010.m_value;
	navi->_2B4 = _1C / 3.0f * (max - min) + min;

	max        = parms->m_naviParms.m_p024.m_value;
	min        = parms->m_naviParms.m_p025.m_value;
	navi->_2B8 = _1C / 3.0f * (max - min) + min;

	if (mHeldPiki && _20) {
		int stateID = mHeldPiki->getStateID();
		if (stateID != PIKISTATE_Hanged && stateID != PIKISTATE_GoHang) {
			transit(navi, NSID_Walk, nullptr);
			return;
		}
	}

	bool doTypeSwitch = false, doHappaSwitch = false;
	if (navi->m_padinput->isButtonDown(Controller::PRESS_Y)) {
		if (getSwitchPiki(navi) != nullptr) {
			doTypeSwitch = true;
		} else {
			doHappaSwitch = true;
		}
	}

	if (navi->m_padinput->isButtonDown(Controller::PRESS_DPAD_RIGHT) || doTypeSwitch) {
		mCurrHappa    = -1;
		Piki* newPiki = getSwitchPiki(navi);

		if (newPiki) {
			Piki* held = mHeldPiki;
			navi->_269 = newPiki->m_pikiKind;
			if (held->m_navi) {
				if (held->m_pikiKind == Bulbmin) {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_HAPPA_THROW_WAIT, 0);
				} else {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_VC_THROW_WAIT, 0);
				}
			}
			held->m_fsm->transit(held, PIKISTATE_Walk, nullptr);
			mHeldPiki = newPiki;
			newPiki->m_fsm->transit(newPiki, PIKISTATE_Hanged, nullptr);
			sortPikis(navi);
			PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
			rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
			return;
		}

	} else if (navi->m_padinput->isButtonDown(Controller::PRESS_DPAD_LEFT)) {
		mCurrHappa    = -1;
		int currColor = mHeldPiki->m_pikiKind;
		int pikisNext[(PikiColorCount - 1)];
		for (int i = 0; i < (PikiColorCount - 1); i++) {
			pikisNext[i] = ((currColor + ((PikiColorCount - 2) - i) + 1) % PikiColorCount);
		}

		Piki* newPiki = nullptr;
		for (int i = 0; i < (PikiColorCount - 1); i++) {
			Piki* p = findNearestColorPiki(navi, pikisNext[i]);
			if (p) {
				newPiki = p;
				break;
			}
		}
		if (newPiki) {
			Piki* held = mHeldPiki;
			navi->_269 = newPiki->m_pikiKind;
			if (held->m_navi) {
				if (currColor == Bulbmin) {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_HAPPA_THROW_WAIT, 0);
				} else {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_VC_THROW_WAIT, 0);
				}
			}
			held->m_fsm->transit(held, PIKISTATE_Walk, nullptr);
			mHeldPiki = newPiki;
			newPiki->m_fsm->transit(newPiki, PIKISTATE_Hanged, nullptr);
			sortPikis(navi);
			PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
			rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
			return;
		}

	} else if (navi->m_padinput->isButtonDown(Controller::PRESS_DPAD_UPDOWN) || doHappaSwitch) {
		bool isButton = navi->m_padinput->isButtonDown(Controller::PRESS_DPAD_DOWN);
		int currColor = mHeldPiki->m_pikiKind;
		int currHappa = mHeldPiki->m_happaKind;
		Piki* newPiki;
		for (int i = 0; i < 2; i++) {
			if (isButton) {
				mCurrHappa = (mCurrHappa + (PikiGrowthStageCount - 1)) % PikiGrowthStageCount; // leaf->flower, flower->bud, bud->leaf
			} else {
				mCurrHappa = (mCurrHappa + 1) % PikiGrowthStageCount; // leaf->bud, bud->flower, flower->leaf
			}
			newPiki = findNearestColorPiki(navi, currColor);
			if (newPiki) {
				if (newPiki->m_happaKind != currHappa) {
					break;
				}
			}
			newPiki = nullptr;
		}
		if (newPiki) {
			Piki* held = mHeldPiki;
			navi->_269 = newPiki->m_pikiKind;
			if (held->m_navi) {
				if (currColor == Bulbmin) {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_HAPPA_THROW_WAIT, 0);
				} else {
					held->m_navi->m_soundObj->stopSound(PSSE_PK_VC_THROW_WAIT, 0);
				}
			}

			held->m_fsm->transit(held, PIKISTATE_Walk, nullptr);
			mHeldPiki = newPiki;
			newPiki->m_fsm->transit(newPiki, PIKISTATE_Hanged, nullptr);
			sortPikis(navi);
			PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
			rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
			return;
		}
	}

	if (!(navi->m_padinput->getButton() & Controller::PRESS_A)) {
		sortPikis(navi);
		navi->m_holdPikiTimer = _1C / 3.0f * parms->m_naviParms.m_p009.m_value;
		NaviThrowInitArg arg(mHeldPiki);
		transit(navi, NSID_Throw, &arg);
		return;
	}

	navi->m_holdPikiTimer += sys->m_deltaTime;

	if (navi->m_holdPikiTimer > parms->m_naviParms.m_p009.m_value) {
		navi->m_holdPikiTimer = parms->m_naviParms.m_p009.m_value;
	}
	if (_2C > 0.0f) {
		_2C -= sys->m_deltaTime;
		if (_2C <= 0.0f) {
			sortPikis(navi);
		}
		return;
	}

	if (navi->m_cPlateMgr->_BC > 0) {
		Vector3f slotPos = navi->m_cPlateMgr->m_slots->_0C;
		Vector3f naviPos = navi->getPosition();
		if (slotPos.distance(naviPos) > 30.0f) {
			Vector3f naviPos = navi->getPosition();
			Vector3f naviVel = navi->getVelocity();
			navi->m_cPlateMgr->setPos(naviPos, navi->m_faceDir + PI, naviVel, 1.0f);
			sortPikis(navi);
		}
	}
}

void NaviThrowState::exec(Navi* navi)
{
	if (navi->m_padinput) {
		navi->control();
		if (navi->m_padinput->getButton() & Controller::PRESS_B) {
			_15 = true;
		}
		navi->findNextThrowPiki();
		if (_14 && navi->m_padinput->m_padButton.m_buttonDown & Controller::PRESS_A && navi->throwable()) {
			transit(navi, NSID_ThrowWait, nullptr);
		}
		if (_14 && navi->m_padinput->m_padButton.m_buttonDown & Controller::PRESS_B) {
			transit(navi, NSID_Gather, nullptr);
		}

		if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_RIGHT)) {
			int color = navi->_269;
			for (int i = 1; i < (PikiColorCount); i++) {
				if ((i + color) > (PikiColorCount - 1)) {
					color = -1 - i;
				}
				if (findNearestColorPiki(navi, (color + i))) {
					PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
					rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
					navi->_269 = color + i;
					sortPikis(navi);
					break;
				}
			}
		} else if (navi->m_padinput->isButtonDown(JUTGamePad::PRESS_DPAD_LEFT)) {
			int color = navi->_269;
			for (int i = 1; i < (PikiColorCount); i++) {
				if ((color - 1) < 0) {
					color = PikiColorCount + i;
				}
				if (findNearestColorPiki(navi, (color - i))) {
					PSSystem::spSysIF->playSystemSe(PSSE_SY_THROW_PIKI_CHANGE, 0);
					rumbleMgr->startRumble(RUMBLETYPE_Nudge, navi->m_naviIndex);
					navi->_269 = color - i;
					sortPikis(navi);
					break;
				}
			}
		}
	}
}

Piki* NaviThrowState::findNearestColorPiki(Navi* navi, int color)
{
	f32 minDist   = 160.0f;
	Piki* retpiki = nullptr;
	Iterator<Creature> iterator(navi->m_cPlateMgr);
	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->m_pikiKind == color) {
			Vector3f diff = piki->getPosition() - navi->getPosition();
			f32 dist      = diff.length();
			if (dist < minDist && piki->getStateID() == PIKISTATE_Walk && piki->isThrowable()) {
				retpiki = piki;
				minDist = dist;
			}
		}
	}
	return retpiki;
}

void NaviThrowState::sortPikis(Navi* navi)
{
	Piki* piki2 = findNearestColorPiki(navi, navi->_269);
	if (!piki2) {
		return;
	}
	navi->m_cPlateMgr->sortByColor(piki2, piki2->m_happaKind);

	Vector3f naviPos = navi->getPosition();

	navi->m_cPlateMgr->setPos(naviPos, navi->m_faceDir + PI, navi->m_position2, 1.0f);

	Iterator<Creature> iterator(navi->m_cPlateMgr);
	CI_LOOP(iterator)
	{
		Piki* piki = static_cast<Piki*>(*iterator);
		if (piki->getCurrActionID() == 0) {
			PikiAI::ActFormation* act = static_cast<PikiAI::ActFormation*>(piki->getCurrAction());
			if (act) {
				act->startSort();
			}
		}
	}
}

/**
 * @note Address: 0x801800B4
 * @note Size: 0xAC
 */
void NaviChangeState::exec(Navi* navi)
{
	if (TwoPlayer::twoPlayerActive) {
		transit(navi, NSID_Walk, nullptr);
		return;
	}

	if (navi->isMovieActor()) {
		transit(navi, NSID_Walk, nullptr);
	}
	navi->m_velocity = Vector3f(0.0f);

	if (mIsFinished == true) {
		transit(navi, NSID_Walk, nullptr);
	}
}

} // namespace Game