#include "types.h"
#include "Dolphin/os.h"
#include "Dolphin/rand.h"
#include "Game/AIConstants.h"
#include "Game/gamePlayData.h"
#include "Game/Data.h"
#include "Game/GameSystem.h"
#include "Game/CameraMgr.h"
#include "Game/CollEvent.h"
#include "Game/rumble.h"
#include "Game/EnemyBase.h"
#include "Game/TimeMgr.h"
#include "Game/MoviePlayer.h"
#include "Game/Interaction.h"
#include "Game/enemyInfo.h"
#include "JSystem/JUT/JUTGamePad.h"
#include "SoundID.h"
#include "PSSystem/PSSystemIF.h"
#include "efx/TSimple.h"
#include "efx/TPk.h"
#include "ObjectTypes.h"
#include "mod.h"
#include "og/Screen/PikminCounter.h"
#include "Game/Navi.h"
#include "Game/NaviParms.h"
#include "Game/PikiState.h"
#include "Game/NaviState.h"
#include "TwoPlayer.h"
#include "Game/SingleGameSection.h"
#include "Game/Entities/ItemOnyon.h"
#include "Game/Entities/ItemPikihead.h"
#include "Game/Entities/SnakeWhole.h"
#include "Game/Entities/Hana.h"
#include "Game/MapMgr.h"
#include "Game/PikiMgr.h"
#include "Game/CPlate.h"
#include "PikiAI.h"
#include "efx/TEnemyDive.h"
#include "PSM/Navi.h"

bool isTreasureCutscene;
bool isKeyCheat = false; // set all keylocks to open if true

namespace Game {

void autopluck(NaviWalkState* walkstate, Navi* captain)
// performs Pikmin 3-style autopluck of pikmin seeds
{
	captain->procActionButton();
	walkstate->execAI(captain);
}

void allowSkipTreasureCutscene(MoviePlayer* movieplayer)
// allows 251-style treasure cutscene skips
{
	if (isTreasureCutscene && movieplayer != nullptr) {
		if (movieplayer->m_demoState == 6) {
			isTreasureCutscene = false;
			// OSReport("Treasure cutscene finished normally!");
		} else if (((JUTGamePad::mPadStatus[0].button & 0x1000) != 0) && (movieplayer->m_demoState == 5)) {
			isTreasureCutscene = false;
			movieplayer->skip();
			// OSReport("Treasure cutscene skipped successfully!);
		}
	}
}

void frogDeathSphere(Game::EnemyBase* frog, Game::CollEvent& event)
{
	if (frog->getEnemyTypeID() == EnemyTypeID::EnemyID_MaroFrog)
		return;

	Vector3f fxPos;
	frog->setEmotionCaution();
	frog->getCommonEffectPos(fxPos);
	fxPos.y -= 85.0f; // effect offset to ground the explosion

	efx::TSimple1 simpleFx(12, nullptr);
	efx::Arg arg(fxPos);
	simpleFx.create(&arg);
	frog->getJAIObject()->startSound(PSSE_PK_SE_BOMB, 0);

	cameraMgr->startVibration(12, fxPos, 2);
	rumbleMgr->startRumble(15, fxPos, 2);

	Creature* collCreature = event.m_collidingCreature;

	Vector3f curPos   = frog->getPosition();
	Vector3f otherPos = collCreature->getPosition();
	Vector3f direction(otherPos.x - curPos.x, 0, otherPos.z - curPos.z);

	direction.normalise();

	direction = Vector3f(direction.x * 50, 0, direction.z * 50);

	InteractBomb bomb(frog, 10000.0f, &direction);
	collCreature->stimulate(bomb);

	// Water can't save the Pikmin
	if (collCreature->isPiki() && collCreature->inWater()) {
		collCreature->kill(nullptr);
	}
}

void doFrogBombParticle(Game::EnemyBase* frog)
{
	if (frog->getEnemyTypeID() == EnemyTypeID::EnemyID_MaroFrog)
		return;

	Vector3f fxPos;
	frog->getCommonEffectPos(fxPos);

	efx::TSimple1 simpleFx(12, nullptr);
	efx::Arg arg(fxPos);
	simpleFx.create(&arg);

	frog->getJAIObject()->startSound(PSSE_PK_SE_BOMB, 0);
}

bool isLowGravity()
{
	if (playData->getCurrentCourseIndex() == 3) {
		if (gameSystem->m_inCave) {
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

void setLowGravity(void)
{
	static float backupGravity = _aiConstants->m_gravity.m_data;
	if (isLowGravity()) {
		_aiConstants->m_gravity.m_data = (backupGravity * 0.4f);
	} else {
		_aiConstants->m_gravity.m_data = backupGravity;
	}
}

void Navi::platCallback(PlatEvent& plat)
{
	Creature* obj = plat.m_item;
	if (plat.m_instance->m_id.match('elec', '*')) {
		if (!playData->m_olimarData->hasItem(OlimarData::ODII_DreamMaterial)) {
			Vector3f origin = plat.m_instance->_B8->getColumn(2);
			Vector3f objPos = obj->getPosition();
			if (((m_shadowParam.m_position.x - objPos.x) * origin.x + ((m_shadowParam.m_position.y - objPos.y) * origin.y)
			     + (m_shadowParam.m_position.z - objPos.z) * origin.z)
			    < 0.0f) {
				origin.x *= -1.0f;
				origin.z *= -1.0f;
			}
			Vector3f mag(200.0f * origin.x, 150.0f, origin.z * 200.0f);
			NaviFlickArg arg(obj, mag, naviMgr->m_naviParms->m_naviParms.m_q010.m_value);
			transit(NSID_Flick, &arg);
		}

		return;
	}

	// shrink on scale blocks and bridges
	if (plat.m_instance->m_id.match('finl', '*') || plat.m_instance->m_id.match('down', '*')) {
		m_cPlateMgr->shrink();
	}
}

}; // namespace Game

#include "Game/Entities/ItemTreasure.h"
#include "Game/pelletMgr.h"
#include "Radar.h"

Radar::Point* getPointFromObj(Game::TPositionObject* obj)
{
	Radar::Point* node = (Radar::Point*)Radar::mgr->m_pointNode1.m_child;
	while (node->m_object != obj) {
		node = (Radar::Point*)node->m_next;
	}
	return node;
}

// no longer used for anything, though I don't feel like unlinking them if needed later
void removeBuriedTreasureRadar(Game::ItemTreasure::Item* treasure) { }
void addPelletUnearthRadar(Game::ItemTreasure::Item* treasure) { }

bool hasTreasure(Game::Pellet* pellet)
{
	Iterator<Game::BaseItem> iTreasure = Game::ItemTreasure::mgr;
	CI_LOOP(iTreasure)
	{
		Game::ItemTreasure::Item* treasure = static_cast<Game::ItemTreasure::Item*>(*iTreasure);
		if (pellet == treasure->m_pellet)
			return true;
	}
	return false;
}

bool shouldDrawTreasure(Radar::Point* point)
{
	if (!Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_PrototypeDetector)) {
		return false;
	}
	if (Radar::mgr->m_fuefukiCount > 0) {
		return false;
	}

	if (!point || !point->m_object)
		return false;
	if (point->m_objType == Radar::MAP_TREASURE || point->m_objType == Radar::MAP_UPGRADE) {
		Game::Creature* obj = (Game::Creature*)point->m_object;
		if (obj->isPellet()) {
			Game::Pellet* pellet = (Game::Pellet*)obj;
			if (hasTreasure(pellet)) {
				return false;
			}
			return true;
		}
		return true;
	}
	if (point->m_objType == Radar::MAP_SWALLOWED_TREASURE) {
		return true;
	}
	return false;
}

void StopHoleTrap(Game::Creature* hole)
{
	float launch = (hole->getObjType() == OBJTYPE_Hole) ? 30 : 75;
	Sys::Sphere boundingSphere;
	hole->getBoundingSphere(boundingSphere);
	Game::CellIteratorArg cellArg = boundingSphere;
	Game::CellIterator iCell      = cellArg;
	CI_LOOP(iCell)
	{

		Game::CellObject* obj = *iCell;
		if (obj->isPiki() || obj->isNavi()) {
			Game::FakePiki* piki = static_cast<Game::FakePiki*>(obj);
			piki->m_collisionPosition.y += launch;
			piki->m_position2.y += launch;
			piki->m_shadowParam.m_position.y += launch;
			piki->m_shadowParam.m_boundingSphere.m_position.y += launch;
		}
		if (obj->getObjType() == OBJTYPE_Pellet) {
			Game::Pellet* pellet = static_cast<Game::Pellet*>(obj);
			pellet->m_pelletPosition.y += launch;
			pellet->m_collisionPosition.y += launch;
			pellet->m_lodSphere.m_position.y += launch;
		}
		if (obj->getObjType() == OBJTYPE_Teki) {
			Game::EnemyBase* enemy = static_cast<Game::EnemyBase*>(obj);
			enemy->m_collisionPosition.y += launch;
			enemy->m_position.y += launch;
		}
	}
}

namespace mod {
int keyLockCount;
bool isExitLocked;
bool isBobuMovieQueued              = false;
og::newScreen::ObjCave* thisObjCave = nullptr;

// adjust treasure culling radius to 50.0f if radius was zero
float adjustBoundingRadius(float radius)
{
	if (radius == 0.0f)
		return 50.0f;
	else
		return radius;
}

bool isKeyUnlock()
{
	if (isKeyCheat) {
		return true;
	}
	bool retval = false;
	if (Game::gameSystem && Game::gameSystem->m_inCave && Game::gameSystem->isStoryMode()) {
		Game::GameSystem* gs           = Game::gameSystem;
		Game::BaseGameSection* section = gs->m_section;
		if (section) {
			switch (section->getCaveID()) {
			case 't_01':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_SphericalAtlas);
				break;
			case 't_02':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_StellarOrb);
				break;
			case 't_03':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_ProfessionalNoisemaker);
				break;
			case 'f_01':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_BruteKnuckles);
				break;
			case 'f_02':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_AmplifiedAmplifier);
				break;
			case 'f_03':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_RepugnantAppendage);
				break;
			case 'f_04':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_GeographicProjection);
				break;
			case 'y_01':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_ForgedCourage);
				break;
			case 'y_02':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_FiveManNapsack);
				break;
			case 'y_03':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_DreamMaterial);
				break;
			case 'y_04':
				retval = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_JusticeAlloy);
				break;
			case 'l_01':
				retval = Game::playData->isPelletZukanVisible(115);
				break;
			case 'l_02':
				retval = Game::playData->isPelletZukanVisible(34);
				break;
			case 'l_03':
				retval = Game::playData->isPelletZukanVisible(68);
				break;
			default:
				break;
			}
		}
	}
	if (retval) {
		isExitLocked = false;
	}
	return retval;
}

void updateDispMember()
{
	if (!Game::gameSystem->m_inCave) {
		return;
	}
	if (!thisObjCave) {
		return;
	}
	if (!thisObjCave->m_disp) {
		return;
	}
	if (keyLockCount < 0) {
		keyLockCount = 0;
	}
	thisObjCave->m_disp->m_keyCount = keyLockCount;
	if (TwoPlayer::twoPlayerActive) {
		thisObjCave->m_keyCounter2p->update();
		if (!isExitLocked || keyLockCount == 0 || isKeyUnlock()) {
			thisObjCave->m_pikiCounterOlimar->search('Nkeys')->hide();
		} else {
			thisObjCave->m_pikiCounterOlimar->search('Nkeys')->show();
		}
	} else {
		thisObjCave->m_keyCounter->update();
		if (!isExitLocked || keyLockCount == 0 || isKeyUnlock()) {
			thisObjCave->m_pikiCounter->search('Nkeys')->hide();
		} else {
			thisObjCave->m_pikiCounter->search('Nkeys')->show();
		}
	}
}

}; // namespace mod

bool Game::InteractTroll::actPiki(Game::Piki* piki)
{
	if (piki->m_currentState->invincible(piki)) {
		return false;
	}
	int pikiKind = piki->m_pikiKind;
	if (pikiKind != Blue) {
		efx::createSimpleChiru(*piki->m_effectsObj->_0C, piki->m_effectsObj->_08);
		piki->startSound(PSSE_PK_FLOWER_VOICE, true);
		piki->m_happaKind = Flower;
		return false;
	}
	if (piki->m_currentState->dead()) {
		return false;
	}
	BlowStateArg witherArg(m_direction, 1.0f, false, 6, m_creature);
	piki->m_fsm->transit(piki, PIKISTATE_Blow, &witherArg);
	return true;
}

bool Game::InteractPress::actPiki(Game::Piki* piki)
{
	if (!(gameSystem->isFlag(32)) && gameSystem->m_inCave) {
		return false;
	}

	if (piki->m_currentState->invincible(piki)) {
		return false;
	}

	if (piki->m_pikiKind == Bulbmin) {
		InteractFallMeck bury(m_creature, m_damage);
		piki->stimulate(bury);
		return false;
	}

	if (piki->m_currentState->pressable()) {

		if (m_creature->isTeki()) {
			EnemyBase* teki = static_cast<EnemyBase*>(m_creature);
			piki->setTekiKillID(teki->getEnemyTypeID());
		} else {
			piki->m_tekiKillID = -1;
		}

		piki->m_fsm->transit(piki, PIKISTATE_Pressed, nullptr);
		piki->startSound(PSSE_PK_VC_PRESSED, false); // why isn't this in PikiPressedState's init?
		return true;
	}

	return false;
}

bool Game::InteractBomb::actNavi(Game::Navi* navi)
{
	if (!(gameSystem->m_flags & GAMESYS_Unk6)) {
		return false;
	}

	bool isTeki          = m_creature->isTeki();
	bool isHoudai        = static_cast<EnemyBase*>(m_creature)->getEnemyTypeID() == EnemyTypeID::EnemyID_Houdai;
	bool hasJusticeAlloy = Game::playData->m_olimarData[0].hasItem(Game::OlimarData::ODII_JusticeAlloy);
	if (hasJusticeAlloy && (!isTeki || !isHoudai)) {
		return false;
	}

	Game::NaviFlickArg flickArg(m_creature, m_direction, m_damage);
	navi->transit(Game::NSID_Flick, &flickArg);
	return true;
}

bool Game::InteractMattuan::actPellet(Pellet* pellet)
{
	// if its an upgrade and not the key
	int itemIndex = pellet->getConfigIndex();
	if (pellet->getKind() == 4 && itemIndex != OlimarData::ODII_TheKey) {
		pellet->startDiscoverDisable(mWaitTimer / sys->m_deltaTime);
	} else {
		pellet->clearDiscoverDisable();
	}
	return true;
}

bool canEnterCave(Game::Piki* piki, u32 id)
{
	if (id == 'f_03') {
		return piki->m_pikiKind == Game::Blue;
	}
	if (id == 'y_04') {
		return piki->m_pikiKind == Game::Red;
	}
	if (id == 't_02') {
		return piki->m_pikiKind == Game::White;
	}
	return true;
}

void Game::SingleGameSection::playMovie_helloPikmin(Game::Piki* piki)
{
	switch (piki->m_pikiKind) {
	case Red: {
		MoviePlayArg arg("g03_meet_redpikmin", nullptr, _C8, 0);
		Onyon* onyon = ItemOnyon::mgr->getOnyon(ONYON_TYPE_RED);
		JUT_ASSERTLINE(1481, onyon, "No RED ONYON");
		arg.m_origin                = onyon->getPosition();
		arg.m_angle                 = onyon->getFaceDir();
		moviePlayer->m_targetObject = piki;
		moviePlayer->play(arg);
		playData->setMeetPikmin(piki->m_pikiKind);
		playData->setContainer(piki->m_pikiKind);
		disableTimer(1);
		break;
	}
	case Yellow: {
		MoviePlayArg arg("g1F_meet_yellowpikmin", nullptr, _C8, 0);
		arg.m_origin                = piki->getPosition();
		arg.m_angle                 = piki->getFaceDir();
		moviePlayer->m_targetObject = piki;
		moviePlayer->play(arg);
		playData->setMeetPikmin(piki->m_pikiKind);
		playData->setContainer(piki->m_pikiKind);
		break;
	}
	case Blue: {
		MoviePlayArg arg("g21_meet_bluepikmin", nullptr, _C8, 0);
		arg.m_origin                = piki->getPosition();
		arg.m_angle                 = piki->getFaceDir();
		moviePlayer->m_targetObject = piki;
		moviePlayer->play(arg);
		playData->setMeetPikmin(piki->m_pikiKind);
		playData->setContainer(piki->m_pikiKind);
		break;
	}
	case Purple: {
		MoviePlayArg arg("g24_meet_blackpikmin", nullptr, _C8, 0);
		arg.m_origin                = piki->getPosition();
		arg.m_angle                 = piki->getFaceDir();
		moviePlayer->m_targetObject = piki;
		moviePlayer->play(arg);
		playData->setMeetPikmin(piki->m_pikiKind);
		playData->setContainer(piki->m_pikiKind);
		break;
	}
	case White: {
		MoviePlayArg arg("g27_meet_whitepikmin", nullptr, _C8, 0);
		arg.m_origin                = piki->getPosition();
		arg.m_angle                 = piki->getFaceDir();
		moviePlayer->m_targetObject = piki;
		moviePlayer->play(arg);
		playData->setMeetPikmin(piki->m_pikiKind);
		playData->setContainer(piki->m_pikiKind);
		break;
	}
	case Bulbmin: {
		if (!playData->isDemoFlag(DEMO_Discover_Bulbmin)) {
			MoviePlayArg arg("x13_exp_leafchappy", nullptr, _C8, 0);
			arg.m_origin                = piki->getPosition();
			arg.m_angle                 = piki->getFaceDir();
			moviePlayer->m_targetObject = piki;
			moviePlayer->play(arg);

			playData->setDemoFlag(DEMO_Discover_Bulbmin);
		}
		break;
	}
	}
}

void Game::PikiNukareState::exec(Piki* piki)
{
	if (!piki->assertMotion(m_animIdx)) {
		_14 = true;
	}

	if (_14) {
		Vector3f position = piki->m_collTree->m_part->m_position;
		transit(piki, PIKISTATE_Walk, nullptr);
		position.y = mapMgr->getMinY(position);
		piki->setPosition(position, false);

		PikiAI::ActFormationInitArg initArg(piki->m_navi);
		piki->m_brain->start(0, &initArg);

		int pikiType = piki->m_pikiKind;
		if (pikiType >= 0 && pikiType <= Bulbmin) {
			if (!playData->hasMetPikmin(piki->m_pikiKind) || pikiType == Bulbmin) {
				gameSystem->m_section->playMovie_helloPikmin(piki);
			}
		}
	}
}

void Game::PikiAutoNukiState::exec(Piki* piki)
{
	switch (m_state) {
	case 0:
		m_timer -= sys->m_deltaTime;
		if (m_timer < 0.0f) {
			m_timer = 0.0f;
			piki->startMotion(IPikiAnims::KAIFUKU2, IPikiAnims::KAIFUKU2, piki, nullptr);
			m_state = 1;

			Vector3f position = piki->getPosition();
			Sys::Sphere sphere(position, 10.0f);
			WaterBox* wbox = piki->checkWater(nullptr, sphere);

			if (wbox) {
				efx::TEnemyDive diveFx;
				efx::ArgScale fxArg(position, 1.2f);
				diveFx.create(&fxArg);

				if (piki->m_navi) {
					piki->startSound(piki->m_navi, PSSE_EV_ITEM_LAND_WATER1_S, true);
					piki->startSound(piki->m_navi, PSSE_PL_PULLOUT_PIKI, false);
				}

			} else if (piki->m_navi) {
				efx::createSimplePkAp(position);
				piki->startSound(piki->m_navi, PSSE_PL_PULLOUT_PIKI, false);
			}
		}
		break;

	case 1:
		if (!piki->assertMotion(IPikiAnims::KAIFUKU2)) {
			m_state = 2;
		}
		break;

	case 2:
		if (piki->m_navi) {
			PikiAI::ActFormationInitArg initArg(piki->m_navi);
			initArg.m_doUseTouchCooldown = true;
			piki->m_brain->start(0, &initArg);

		} else {
			piki->m_brain->start(1, nullptr);
		}

		transit(piki, PIKISTATE_Walk, nullptr);

		int type = piki->m_pikiKind;
		if (type >= 0 && type <= Bulbmin) {
			if (!playData->hasMetPikmin(type) || type == Bulbmin) {
				gameSystem->m_section->playMovie_helloPikmin(piki);
			}
		}

		break;
	}
}

// init and exec are used here to change the mass of the idle leader,
// so that it does not get in the way of the active leader
void Game::NaviFollowState::init(Navi* navi, StateArg* stateArg)
{
	NaviFollowArg* followArg = static_cast<NaviFollowArg*>(stateArg);
	if (followArg && followArg->mIsNewToParty) {

		navi->startMotion(IPikiAnims::KIZUKU, IPikiAnims::KIZUKU, navi, nullptr);
		mFollowState = FOLLOW_AlertJump;

		if (navi->m_naviIndex == 0) {
			navi->m_soundObj->startSound(PSSE_PL_BIKU_ORIMA, 0);

		} else if (playData->isStoryFlag(STORY_DebtPaid)) {
			navi->m_soundObj->startSound(PSSE_PL_BIKU_SHACHO, 0);

		} else {
			navi->m_soundObj->startSound(PSSE_PL_BIKU_LUGI, 0);
		}

	} else {
		mFollowState = FOLLOW_Normal;
		navi->startMotion(IPikiAnims::WALK, IPikiAnims::WALK, nullptr, nullptr);
	}

	mTargetNavi = naviMgr->getAt(1 - navi->m_naviIndex);
	navi->setMoveRotation(true);
	mIdleCounter           = 0;
	mTargetEnemy           = nullptr;
	navi->m_mass           = 100.0f;
	navi->mPluckingCounter = 0;
}

void Game::NaviFollowState::cleanup(Navi* navi) { navi->m_mass = 1.0f; }

// separate seed growth stage timers
void Game::ItemPikihead::WaitState::init(Item* item, StateArg*)
{
	item->m_efxTane->createKourin_(item->m_efxTane->mEfxPos);
	item->m_animator.startAnim(0, item);
	if (item->m_headType == Flower) {
		mTimer = 2.0f * randFloat() + pikiMgr->m_parms->m_pikiParms.m_p052.m_value; // wither time
		return;
	}

	if (item->m_headType == Bud) {
		mTimer = 2.0f * randFloat() + pikiMgr->m_parms->m_pikiParms.m_p045.m_value; // seed bud to flower time
		return;
	}

	mTimer = 2.0f * randFloat() + pikiMgr->m_parms->m_pikiParms.m_p051.m_value; // growup time
}

bool Game::Hana::Obj::bombCallBack(Creature* creature, Vector3f& vec, f32 damage)
{
	m_health = 0.0f;
	return true;
}

void Game::SnakeWhole::Obj::collisionCallback(CollEvent& event)
{
	Creature* creature = event.m_collidingCreature;
	if (!mIsJumping || creature == nullptr || isEvent(0, EB_Bittered)) {
		return;
	}

	if (creature->m_curTriangle == nullptr || !creature->isPiki()) {
		return;
	}

	if (event.m_hitPart->m_currentID == 'foot') {
		InteractFallMeck fallMeck(this, C_GENERALPARMS.m_attackDamage.m_value);
		creature->stimulate(fallMeck);
	}
}

void setKeyCheat()
{
	if (sys->getPlayCommonData()->isChallengeGamePlayable()) {
		if (JUTGamePad::mPadStatus[0].button & JUTGamePad::PRESS_L) {
			if (!isKeyCheat) {
				PSSystem::spSysIF->playSystemSe(PSSTR_GET_BEEDAMA, 0);
			}
			isKeyCheat = true; // set all keylocks open
		}
	}
}
