#include "Game/NaviParms.h"
#include "Game/NaviState.h"
#include "Game/MoviePlayer.h"
#include "Game/PikiMgr.h"
#include "Game/PikiState.h"
#include "Game/Entities/ItemPikihead.h"
#include "Game/Entities/ItemOnyon.h"
#include "Game/Entities/Pom.h"
#include "Game/generalEnemyMgr.h"
#include "Game/EnemyIterator.h"
#include "PSM/Navi.h"
#include "TwoPlayer.h"
#include "Game/CPlate.h"

namespace Game {

void NaviState::swapNavi(Navi* navi)
{
	Navi* otherNavi  = naviMgr->getAt(1 - navi->m_naviIndex);
	int otherStateID = otherNavi->getStateID();

	if (otherNavi->isAlive() && otherStateID != NSID_Punch) {
		gameSystem->m_section->pmTogglePlayer();

		playChangeVoice(otherNavi);

		if (otherNavi->m_currentState->needYChangeMotion()) {
			otherNavi->m_fsm->transit(otherNavi, NSID_Change, nullptr);
		}
	}
}

void NaviWalkState::execAI(Navi* navi)
{
	switch (mAIState) {
	case WALKAI_Wait:
		// only do with inactive captains
		if (navi->m_padinput == nullptr) {
			navi->procActionButton();
		}

		execAI_wait(navi);
		checkAI(navi);
		break;

	case WALKAI_Animation:
		execAI_animation(navi);
		checkAI(navi);
		break;

	case WALKAI_Escape:
		execAI_escape(navi);
		break;

	case WALKAI_Attack:
		execAI_attack(navi);
		break;
	}
}

void NaviNukuState::init(Navi* navi, StateArg* stateArg)
{
	if (stateArg != nullptr) {
		mIsFollower = static_cast<NaviNukuArg*>(stateArg)->mIsFollowing;
	} else {
		mIsFollower = false;
	}
	if (navi->mPluckingCounter != 0) {
		mAnimID = IPikiAnims::NUKU3;
	} else {
		mAnimID = IPikiAnims::NUKU;
	}
	navi->startMotion(mAnimID, mAnimID, navi, nullptr);
	mCounter = static_cast<NaviParms*>(navi->m_parms)->m_naviParms.m_p042.m_value;
	navi->m_soundObj->startSound(PSSE_PL_PULLING_PIKI, 0);
	mDidPluckSE  = 0;
	mIsActive    = 0;
	mDidPressA   = 0;
	_15          = 0;
	navi->m_mass = 0.0f;
}

void NaviNukuState::exec(Navi* navi)
{
	if (!mDidPressA) {
		mDidPressA = true;
	}

	if (moviePlayer && moviePlayer->m_demoState != 0) {
		if (mIsFollower) {
			Navi::sIsThereFollower = false;
			NaviFollowArg followArg(false);
			transit(navi, NSID_Follow, &followArg);
			return;
		}
		transit(navi, NSID_Walk, nullptr);
		return;
	}

	navi->m_velocity  = 0.0f;
	navi->m_position2 = 0.0f;
	if (!navi->assertMotion(mAnimID)) {
		if (mIsFollower) {
			Navi::sIsThereFollower = false;
			NaviFollowArg followArg(false);
			transit(navi, NSID_Follow, &followArg);
		} else {
			transit(navi, NSID_Walk, nullptr);
		}
		navi->mPluckingCounter = 0;
	} else {
		mDidPressA = true;
		mIsActive  = true;
		navi->mPluckingCounter++;
	}

	if (navi->m_padinput && navi->m_padinput->getButton() & PAD_BUTTON_B) {
		navi->mPluckingCounter = 0;
		transit(navi, NSID_Walk, nullptr);

		Navi* otherNavi = naviMgr->getAt(1 - navi->m_naviIndex);
		if (otherNavi && otherNavi->isAlive() && Navi::sIsThereFollower) {
			otherNavi->mPluckingCounter = 0;
			Navi::sIsThereFollower      = false;
			transit(otherNavi, NSID_Follow, nullptr);
		}
		return;
	}

	if (navi->m_padinput && navi->m_padinput->isButtonDown(JUTGamePad::PRESS_Y)) {
		if (gameSystem->isMultiplayerMode() || TwoPlayer::twoPlayerActive) {
			navi->releasePikis(nullptr, true);
		} else if (!gameSystem->paused_soft() && moviePlayer->m_demoState == 0 && playData->isDemoFlag(DEMO_Unlock_Captain_Switch)) {
			swapNavi(navi);
		}
	}
}

void NaviNukuState::onKeyEvent(Navi* navi, SysShape::KeyEvent const& key)
{
	switch (key.m_type) {
	case KEYEVENT_2:
		_15 = true;
		mCounter--;
		if (mCounter == 0) {
			navi->m_soundObj->startSound(PSSE_PL_PULLOUT_PIKI, 0);
			mDidPluckSE = true;
		}
		break;
	case KEYEVENT_END:
		if (mIsActive) {
			if (!navi->procActionButton()) {
				mIsActive = false;
				if (mIsFollower) {
					Navi::sIsThereFollower = false;
					NaviFollowArg arg(false); // not new to party
					transit(navi, NSID_Follow, &arg);
				} else {
					transit(navi, NSID_Walk, nullptr);
				}
				navi->mPluckingCounter = 0;
			}
		} else {
			if (mIsFollower) {
				Navi::sIsThereFollower = false;
				NaviFollowArg arg(false); // not new to party
				transit(navi, NSID_Follow, &arg);
			} else {
				transit(navi, NSID_Walk, nullptr);
			}
			navi->mPluckingCounter = 0;
		}
		break;
	}
}

bool NaviNukuState::callable() { return true; }

void NaviNukuAdjustState::exec(Navi* navi)
{
	if (moviePlayer && moviePlayer->m_demoState != 0) {
		if (mIsFollowing) {
			Navi::sIsThereFollower = false;
			transit(navi, NSID_Follow, nullptr);
		} else {
			transit(navi, NSID_Walk, nullptr);
		}
		return;
	}

	if (!mPikiHead->isAlive() && !navi->procActionButton()) {
		if (mIsFollowing) {
			Navi::sIsThereFollower = false;
			transit(navi, NSID_Follow, nullptr);
		} else {
			transit(navi, NSID_Walk, nullptr);
		}
		return;
	}

	if (!mIsFollowing) {
		navi->makeCStick(false);
	}

	if (navi->m_padinput && navi->m_padinput->getButton() & Controller::PRESS_B) {
		navi->mPluckingCounter = 0;
		transit(navi, NSID_Walk, nullptr);

		Navi* otherNavi = naviMgr->getAt(1 - navi->m_naviIndex);
		if (otherNavi && otherNavi->isAlive() && Navi::sIsThereFollower) {
			otherNavi->mPluckingCounter = 0;
			Navi::sIsThereFollower      = false;
			transit(otherNavi, NSID_Follow, nullptr);
		}
		return;
	}

	mNaviPosition = navi->getPosition();

	Vector3f sproutToNavi = mPikiHead->getPosition() - navi->getPosition();
	sproutToNavi.length(); // unused

	Vector3f targetToNavi    = mTargetPosition - navi->getPosition(); // f26, f27, f28
	f32 targetToNaviDistance = targetToNavi.length2D();               // f31
	f32 absoluteDeltaY       = absF(targetToNavi.y);

	f32 normalisedDistance = targetToNavi.normalise(); // f30, why tho

	f32 newFaceDir = mAngleToPiki;
	f32 angle      = angDist(newFaceDir, navi->m_faceDir);

	if (absF(angle) < (PI / 10) && targetToNaviDistance < 2.0f && absoluteDeltaY < 10.0f) {
		navi->m_faceDir     = newFaceDir;
		PikiMgr::mBirthMode = PikiMgr::PSM_Force;
		Piki* piki          = pikiMgr->birth();
		PikiMgr::mBirthMode = PikiMgr::PSM_Normal;

		if (!piki) {
			if (mIsFollowing) {
				Navi::sIsThereFollower = false;
				transit(navi, NSID_Follow, nullptr);
			} else {
				transit(navi, NSID_Walk, nullptr);
			}
			return;
		}

		piki->init(nullptr);
		piki->changeShape(mPikiHead->m_color);
		piki->changeHappa(mPikiHead->m_headType);
		if (Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_RepugnantAppendage)) {
			piki->changeHappa(Game::Bud);
		}

		Vector3f sproutPos = mPikiHead->getPosition();
		piki->setPosition(sproutPos, false);
		mPikiHead->kill(nullptr);
		mPikiHead = nullptr;

		NukareStateArg nukareArg;
		nukareArg.mIsPlucking = navi->mPluckingCounter != 0;
		nukareArg.mNavi       = navi;
		piki->m_fsm->transit(piki, PIKISTATE_Nukare, &nukareArg);

		NaviNukuArg nukuArg;
		nukuArg.mIsFollowing = mIsFollowing;

		transit(navi, NSID_Nuku, &nukuArg);

	} else {
		f32 angleOffset = 0.2f * angle;
		navi->m_faceDir = roundAng(navi->m_faceDir + angleOffset);

		f32 speed = 100.0f;
		if (speed * sys->m_deltaTime > normalisedDistance) {
			speed = 0.5f / sys->m_deltaTime;
		}

		Vector3f vel      = targetToNavi * speed;
		navi->m_position2 = vel;
		navi->m_velocity  = Vector3f(0.0f);
		navi->m_velocity  = vel;
	}

	if (mWallHitCounter > 10) {
		if (mIsFollowing) {
			Navi::sIsThereFollower = false;
			transit(navi, NSID_Follow, nullptr);
		} else {
			transit(navi, NSID_Walk, nullptr);
		}

		return;
	}

	if (!mIsMoving) {
		return;
	}

	Vector3f currentVel = navi->m_position2; // f31, f30, f29
	mIsMoving--;
	Vector3f naviPos = navi->getPosition();

	Vector3f pikiToNavi    = mCollidedPikiPosition - naviPos;
	f32 distancePikiToNavi = pikiToNavi.normalise();

	// If the distance is 0, return
	if (!(distancePikiToNavi > 0.0f)) {
		return;
	}

	f32 velocityDifference = pikiToNavi.z * currentVel.z - targetToNavi.x * currentVel.x;
	Vector3f newVel(pikiToNavi.x, 0.0f, -pikiToNavi.z);

	f32 simSpeed = currentVel.length();

	newVel *= simSpeed;
	if (!(velocityDifference < 0.0f)) {
		newVel *= -1.0f;
	}

	// Interpolate 35% current velocity and 65% new velocity
	Vector3f blendedVel = currentVel * 0.35f + newVel * 0.65f;

	f32 speed = blendedVel.normalise();
	if (speed != 0.0f) {
		Vector3f finalVel = blendedVel * simSpeed;
		navi->m_position2 = finalVel;
		navi->m_velocity  = finalVel;
	}
}

bool NaviNukuAdjustState::callable() { return true; }

bool Navi::sIsThereFollower = false;

f32 Navi::getActionRadius()
{
	Vector3f naviPos        = getPosition();
	NaviParms::Parms* parms = &naviMgr->m_naviParms->m_naviParms;

	f32 minDistance = parms->m_p060.m_value;
	minDistance *= minDistance;

	f32 horizontalDistance;

	// first iterate through onyons
	Iterator<Onyon> onyonIter(ItemOnyon::mgr);

	CI_LOOP(onyonIter)
	{
		Onyon* onyon       = *onyonIter;
		Vector3f onyonPos  = onyon->getPosition();
		horizontalDistance = sqrDistanceXZ(onyonPos, naviPos);

		// stop as soon as navi is within the autopluck radius of onyon
		if (horizontalDistance < minDistance) {
			return parms->m_p062.m_value;
		}
	}

	return parms->m_p000.m_value;
}

/**
 * @note Address: 0x80140644
 * @note Size: 0x654
 */
bool Navi::procActionButton()
{
	f32 minDist;
	if (mPluckingCounter) {
		minDist = naviMgr->m_naviParms->m_naviParms.m_p060.m_value;
	} else {
		minDist = getActionRadius();
	}

	Iterator<ItemPikihead::Item> iter(ItemPikihead::mgr);
	minDist *= minDist;
	ItemPikihead::Item* targetSprout = nullptr;

	u16 otherNaviIndex = 1 - m_naviIndex;

	// find (closest) pluckable sprout within range
	CI_LOOP(iter)
	{
		BaseItem* item             = *iter;
		ItemPikihead::Item* sprout = (ItemPikihead::Item*)item;
		Vector3f sproutPos         = sprout->getPosition();
		Vector3f naviPos           = getPosition();
		f32 heightDistance         = absF(sproutPos.y - naviPos.y);
		f32 horizontalDistance     = sqrDistanceXZ(sproutPos, naviPos);

		// sprout has to be pluckable, closer than current/within range, not at massive height difference
		if (sprout->canPullout() && horizontalDistance < minDist && heightDistance < 25.0f) {
			minDist      = horizontalDistance;
			targetSprout = sprout;
		}
	}

	// if sprout found, pluck it.
	if (targetSprout) {
		NaviNukuAdjustStateArg nukuAdjustArg;
		setupNukuAdjustArg(targetSprout, nukuAdjustArg);
		if (sIsThereFollower) {
			nukuAdjustArg._18 = true;
		}
		m_fsm->transit(this, NSID_NukuAdjust, &nukuAdjustArg);

		// if there's a captain following us, put them to work.
		Navi* otherNavi = naviMgr->getAt(otherNaviIndex);
		if (otherNavi && otherNavi->isAlive() && otherNavi->getStateID() == NSID_Follow) {
			sIsThereFollower = true;

			f32 actionRadius = naviMgr->m_naviParms->m_naviParms.m_p060.m_value; // following captain uses autopluck range

			ItemPikihead::Item* otherTargetSprout = nullptr;
			minDist                               = actionRadius * actionRadius;

			// find (closest) pluckable sprout within range that -isn't- the same as main captain's target
			CI_LOOP(iter)
			{
				ItemPikihead::Item* sprout = *iter;
				if (sprout != targetSprout) {
					Vector3f sproutPos = sprout->getPosition();
					Vector3f naviPos   = getPosition();
					f32 heightDiff     = FABS(sproutPos.y - naviPos.y);
					f32 sqrXZ          = sqrDistanceXZ(sproutPos, naviPos);

					// sprout has to be pluckable, closer than current/within range, not at massive height difference
					// (we don't care about VS mode now bc can't have a following captain)
					if (sprout->canPullout() && sqrXZ < minDist && heightDiff < 25.0f) {
						minDist           = sqrXZ;
						otherTargetSprout = sprout;
					}
				}
			}

			// if sprout found, pluck it.
			if (otherTargetSprout) {
				NaviNukuAdjustStateArg nukuAdjustArg2;
				setupNukuAdjustArg(otherTargetSprout, nukuAdjustArg2);
				nukuAdjustArg2._18 = true;
				otherNavi->m_fsm->transit(otherNavi, NSID_NukuAdjust, &nukuAdjustArg2);
			}
		}

		// we plucked something.
		return true;
	}

	// we did not pluck something.
	return false;
}

} // namespace Game
