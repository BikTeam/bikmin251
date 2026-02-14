#include "Dolphin/rand.h"
#include "PikiAI.h"
#include "Game/PikiParms.h"
#include "Game/Entities/ItemOnyon.h"
#include "Game/Navi.h"
#include "Game/Footmark.h"

namespace PikiAI {

/**
 * Most stuff in this file is bug fixes, anything related to ActEnter and ActExit
 * is so that pikmin can have a larger navi search range when exiting the onion.
 * Also includes parameterization of previously hardcoded values, such as various speeds.
 */

/**
 * @note Address: 0x801A2B28
 * @note Size: 0x240
 */
int ActEnter::exec()
{
	switch (_10) {
	case ENTER_OnyonBegin:
		if (m_onyonFoot) {
			m_gotoPos->m_position = m_onyonFoot->m_position;
		}
		if (m_gotoPos->exec() == ACTEXEC_Success) {
			ClimbActionArg climbArg(m_onyonLeg, m_parent->getParms()->m_pikiParms.m_p010.m_value, true);
			m_parent->getPosition();
			m_parent->startStick(m_onyon, m_onyonLeg);
			m_parent->getPosition();
			m_climb->init(&climbArg);
			_10 = ENTER_OnyonClimb;
			m_parent->startSound(m_onyon, PSSE_PK_VC_ONY_CLIMB, true);
		}
		break;

	case ENTER_OnyonClimb: {
		int climbResult    = m_climb->exec();
		f32 climbingHeight = m_parent->_104.y;
		if (climbingHeight < 0.25f) {
			climbingHeight /= 0.25f;
			m_baseScale      = m_parent->getBaseScale() * climbingHeight;
			m_parent->m_scale = m_baseScale;
		}
		if (climbResult == ACTEXEC_Success) {
			m_parent->endStick();
			m_parent->startSound(m_onyon, PSSE_PK_SE_ONY_ENTER, true);
			m_onyon->enterPiki(m_parent);
			return ACTEXEC_Success;
		}
	} break;

	case ENTER_ShipGoto:
		if (m_gotoPos->exec() == ACTEXEC_Success) {
			_10 = ENTER_ShipStay;
			initStay();
		}
		break;

	case ENTER_ShipSuck:
		if (execSuck() == ACTEXEC_Success) {
			m_onyon->enterPiki(m_parent);
			return ACTEXEC_Success;
		}
		break;

	case ENTER_ShipStay:
		execStay();
		break;
	}

	return ACTEXEC_Continue;
}

/**
 * @note Address: 0x801A324C
 * @note Size: 0x204
 */
void ActExit::init(ActionArg* arg)
{
	ActCropArg* cropArg = static_cast<ActCropArg*>(arg);
	m_creature           = cropArg->m_creature;

	if (m_creature->m_objectTypeID == OBJTYPE_Onyon) {
		initOnyon();
	} else if (m_creature->m_objectTypeID == OBJTYPE_Ufo) {
		initUfo();
	} else {
		JUT_PANICLINE(__LINE__, "ActExit invalid objtype");
	}
}

void ActExit::initOnyon()
{
	m_parent->startMotion(Game::IPikiAnims::WALK, Game::IPikiAnims::WALK, nullptr, nullptr);
	m_parent->m_velocity = Vector3f(0.0f);
	int randFoot             = 3.0f * randFloat();
	m_onyonLeg                = static_cast<Game::Onyon*>(m_creature)->getLegPart(randFoot);

	ClimbActionArg climbArg(m_onyonLeg, m_parent->getParms()->m_pikiParms.m_p003.m_value, false);
	m_parent->setPosition(m_onyonLeg->m_position, false);
	m_parent->startStick(m_creature, m_onyonLeg);
	m_climb->init(&climbArg);

	m_parent->startSound(m_creature, PSSE_PK_VC_ONY_EXIT, true);
	m_parent->m_scale                              = Vector3f::zero;
	m_parent->m_mainMatrix.m_matrix.structView.xx = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.yx = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.zx = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.xy = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.yy = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.zy = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.xz = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.yz = 0.0f;
	m_parent->m_mainMatrix.m_matrix.structView.zz = 0.0f;

	PSMTXCopy(m_parent->m_mainMatrix.m_matrix.mtxView, m_parent->m_model->m_j3dModel->m_posMtx);
	m_parent->m_model->m_j3dModel->calc();
}

void ActExit::initUfo()
{
	Game::Onyon* onyon = static_cast<Game::Onyon*>(m_creature);

	Vector3f outpos   = onyon->getOutStart_UFO();
	Vector3f onyonpos = onyon->getPosition();
	Vector3f vel      = outpos - onyonpos;
	vel.normalise();
	f32 factor = randFloat() * 30.0f + 100.0f;
	vel        = Vector3f(vel.x * factor, vel.y * factor, vel.z * factor);

	m_parent->setPosition(outpos, false);
	m_parent->setVelocity(vel);
	m_parent->startSound(m_creature, PSSE_PK_VC_ONY_EXIT, true);
	m_parent->m_faceDir = onyon->m_faceDir;
}

/**
 * @note Address: 0x801A3450
 * @note Size: 0xA8
 */
int ActExit::exec()
{
	if (m_creature->m_objectTypeID == OBJTYPE_Ufo) {
		return ACTEXEC_Success;
	}

	int climbResult    = m_climb->exec();
	f32 climbingHeight = m_parent->_104.y;
	if (climbingHeight < 0.25f) {
		climbingHeight /= 0.25f;
		m_baseScale      = m_parent->getBaseScale() * climbingHeight;
		m_parent->m_scale = m_baseScale;
	}
	if (climbResult == ACTEXEC_Success) {
		return ACTEXEC_Success;
	}
	return ACTEXEC_Continue;
}

/**
 * @note Address: 0x801A34F8
 * @note Size: 0xC8
 */
void ActExit::cleanup()
{
	if (m_creature->m_objectTypeID == OBJTYPE_Ufo) {
		return;
	}

	f32 climbY      = m_parent->_104.y;
	m_baseScale      = m_parent->getBaseScale();
	m_parent->m_scale = Vector3f(m_baseScale);
	m_parent->endStick();
	m_parent->setMoveRotation(true);
	f32 y = 240.0f + 60.0f * randFloat();
	y *= climbY;
	m_parent->m_position2 = Vector3f(0.0f, y, 0.0f);
}

Game::Navi* Brain::searchOrima()
{
	f32 searchDist = m_piki->getParms()->m_pikiParms.m_p036.m_value;
	if (m_actionId == ACT_Exit) {
		searchDist = m_piki->getParms()->m_pikiParms.m_p040.m_value;
	}

	f32 minDist              = 128000.0f;
	Game::Navi* targetPlayer = nullptr;
	for (int i = 0; i < 2; i++) {
		Game::Navi* currentPlayer = Game::naviMgr->getAt(i);
		if (!currentPlayer->isAlive() || currentPlayer->m_padinput == nullptr) {
			continue;
		}

		f32 currentDist = m_piki->m_shadowParam.m_position.sqrDistance2D(currentPlayer->m_shadowParam.m_position);
		if (currentDist >= SQUARE(searchDist) || currentDist >= SQUARE(minDist)) {
			continue;
		}

		minDist      = currentDist;
		targetPlayer = currentPlayer;
	}

	return targetPlayer;
}

} // namespace PikiAI
