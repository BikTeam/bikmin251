#include "Game/gamePlayData.h"
#include "Game/BaseGameSection.h"
#include "Game/GameSystem.h"
#include "Game/MoviePlayer.h"
#include "Game/Navi.h"
#include "utilityU.h"
#include "PSGame/SceneInfo.h"
#include "og/Screen/PikminCounter.h"
#include "Game/Data.h"
#include "Splitter.h"
#include "Game/SingleGameSection.h"
#include "Screen/Game2DMgr.h"
#include "PSM/ObjCalc.h"
#include "Radar.h"

namespace TwoPlayer {
bool useTwoPlayer       = false;
bool twoPlayerActive    = false;
Game::Navi* pausingNavi = nullptr;
int deadPlayer          = 0;
bool tempData           = false;

void initTwoPlayer()
{
	OSReport("setting two player: %d\n", tempData);
	twoPlayerActive = useTwoPlayer = tempData && Game::gameSystem->isStoryMode();
}

bool isUsingTwoPlayer() { return useTwoPlayer && Game::gameSystem->isStoryMode(); }

void setTwoPlayer(bool set)
{
	useTwoPlayer    = set;
	twoPlayerActive = set;
}

// setModeInCutscene__9TwoPlayerFv
void setModeInCutscene()
{
	OSReport("toggle mode called ---------------------------------\n");
	if (twoPlayerActive) {
		OSReport("set player mode ---------------------------------\n");
		Game::BaseGameSection* section = Game::gameSystem->m_section;
		JUT_ASSERT(Game::gameSystem->m_section, "Section Missing! LMAO\n");
		JUT_ASSERT(Game::naviMgr, "NaviMgr Missing!\n");
		JUT_ASSERT(sys->m_gfx, "gfx Missing!\n");
		int alive = Game::naviMgr->getAliveCount();
		if (section->m_prevNaviIdx == 1 && alive != 2) {
			OSReport("Second Mode\n");
			// section->setPlayerMode(1);
			if (alive == 2 || alive == 0)
				section->setSplitter(false);
			TwoPlayer::twoPlayerActive    = false;
			Game::moviePlayer->m_viewport = sys->m_gfx->getViewport(1);

			Game::moviePlayer->m_actingCamera = section->m_louieCamera;
		} else {
			OSReport("First Mode\n");
			TwoPlayer::twoPlayerActive = false;
			// section->setPlayerMode(0);
			if (alive == 2 || alive == 0)
				section->setSplitter(false);
			Game::moviePlayer->m_viewport     = sys->m_gfx->getViewport(0);
			Game::moviePlayer->m_actingCamera = section->m_olimarCamera;
		}
		JUT_ASSERT(Game::moviePlayer->m_viewport, "Viewport Missing\n");
		JUT_ASSERT(Game::moviePlayer->m_actingCamera, "Camera Missing!\n");
	}
}

// setModeOutCutscene__9TwoPlayerFv
void setModeOutCutscene()
{
	int alive = Game::naviMgr->getAliveCount();
	if (isUsingTwoPlayer() && alive == 2) {
		Game::gameSystem->m_section->TogglePlayer();
	}
}
// loadSound__9TwoPlayerFv
void loadSound()
{
	if (isUsingTwoPlayer()) {
		Game::naviMgr->createPSMDirectorUpdator();
	}
	PSSetCurCameraNo(0);
}

// stopInputsOther__9TwoPlayerFPQ24Game4Navi
void stopInputsOther(Game::Navi* navi)
{
	Game::Navi* other = Game::naviMgr->getAt(1 - navi->m_naviIndex);
	other->disableController();
	other->m_velocity = Vector3f(0.0f);
}

// resumeInputsOther__9TwoPlayerFPQ24Game4Navi
void resumeInputsOther(Game::Navi* navi)
{
	int alive = Game::naviMgr->getAliveCount();
	if (alive == 2 && isUsingTwoPlayer()) {
		Game::Navi* other = Game::naviMgr->getAt(1 - navi->m_naviIndex);
		other->m_padinput = other->m_padinput2;
	}
}

inline bool checkPause(Game::SingleGameSection* section, int player)
{
	Controller* control = Game::naviMgr->getAt(player)->m_padinput;
	if (control && control->isButtonDown(PAD_BUTTON_START)) {
		og::Screen::DispMemberSMenuAll member;
		Screen::gGame2DMgr->setGamePad(control);
		section->setDispMemberSMenu(member);
		bool shouldPause = Screen::gGame2DMgr->open_SMenu(member);
		if (shouldPause) {
			Game::gameSystem->setPause(true, "open-sm", 3);
			Game::gameSystem->setMoviePause(true, "open-sm");
			return true;
		}
	}
	return false;
}

// pauseMap__9TwoPlayerFPQ24Game17SingleGameSection
void pauseMap(Game::SingleGameSection* section)
{
	if (!(Game::gameSystem->m_flags & 0x8) && Game::moviePlayer->m_demoState == 0 && !Game::gameSystem->paused()) {
		bool p1Pause = checkPause(section, 0);
		if (p1Pause) {
			OSReport("p1 paused\n");
			pausingNavi = Game::naviMgr->getAt(0);
			return;
		}
		bool p2Pause = checkPause(section, 1);
		if (p2Pause) {
			pausingNavi = Game::naviMgr->getAt(1);
			OSReport("p2 paused\n");
		}
	}
}

// setController__9TwoPlayerFP10Controller
void setController(Controller* controller) { Screen::gGame2DMgr->setGamePad(controller); }

// setController__9TwoPlayerFi
void setController(int id) { Screen::gGame2DMgr->setGamePad(Game::naviMgr->getAt(id)->m_padinput); }
// getCurrentNavi__9TwoPlayerFv
Game::Navi* getCurrentNavi()
{
	OSReport("Getting Navi\n");
	if (isUsingTwoPlayer()) {
		return pausingNavi;
	}
	return Game::naviMgr->getActiveNavi();
}

} // namespace TwoPlayer

namespace Game {

void BaseGameSection::pmTogglePlayer()
{
	// OSReport("are we two player? %s\n", TwoPlayer::useTwoPlayer);
	if (!(TwoPlayer::tempData && Game::gameSystem->isStoryMode())) {
		TogglePlayer();
	}
}

inline void BaseGameSection::TogglePlayer()
{
	int alive = naviMgr->getAliveCount();
	if (TwoPlayer::isUsingTwoPlayer() && alive == 2) {
		TwoPlayer::twoPlayerActive = true;
		setPlayerMode(2);
		moviePlayer->m_viewport     = sys->m_gfx->getViewport(0);
		moviePlayer->m_actingCamera = m_olimarCamera;
	} else if ((m_prevNaviIdx == 0) || (m_prevNaviIdx == 1 && TwoPlayer::isUsingTwoPlayer())) {
		TwoPlayer::twoPlayerActive = false;
		setPlayerMode(1);
		moviePlayer->m_viewport     = sys->m_gfx->getViewport(1);
		moviePlayer->m_actingCamera = m_louieCamera;
	} else if ((m_prevNaviIdx == 1) || (m_prevNaviIdx == 0 && TwoPlayer::isUsingTwoPlayer())) {
		TwoPlayer::twoPlayerActive = false;
		setPlayerMode(0);
		moviePlayer->m_viewport     = sys->m_gfx->getViewport(0);
		moviePlayer->m_actingCamera = m_olimarCamera;
	}
	onTogglePlayer();
}

void BaseGameSection::setCamController()
{
	Navi* navis[2];

	navis[0] = naviMgr->getAt(0);
	navis[1] = naviMgr->getAt(1);

	switch (m_prevNaviIdx) {
	case 0: {
		PlayCamera* olimarCam        = m_olimarCamera;
		navis[0]->m_camera           = olimarCam;
		navis[0]->m_camera2          = olimarCam;
		Controller* olimarController = m_controllerP1;
		navis[0]->m_padinput         = olimarController;
		navis[0]->m_padinput2        = olimarController;
		navis[1]->disableController();
		moviePlayer->m_targetNavi   = navis[0];
		moviePlayer->m_viewport     = sys->m_gfx->getViewport(0);
		moviePlayer->m_actingCamera = m_olimarCamera;
		if (!gameSystem->isMultiplayerMode()) {
			PSSetCurCameraNo(0);
			// PSPlayerChangeToOrimer();
		}
		break;
	}
	case 1: {
		navis[0]->disableController();
		PlayCamera* louieCam        = m_louieCamera;
		navis[1]->m_camera          = louieCam;
		navis[1]->m_camera2         = louieCam;
		Controller* louieController = (TwoPlayer::isUsingTwoPlayer()) ? m_controllerP2 : m_controllerP1;
		navis[1]->m_padinput        = louieController;
		navis[1]->m_padinput2       = louieController;
		navis[0]->disableController();
		moviePlayer->m_targetNavi   = navis[1];
		moviePlayer->m_viewport     = sys->m_gfx->getViewport(1);
		moviePlayer->m_actingCamera = m_louieCamera;
		if (!gameSystem->isMultiplayerMode()) {
			PSSetCurCameraNo(1);
			// PSPlayerChangeToLugie();
		}
		break;
	}
	case 2: {
		PlayCamera* olimarCam        = m_olimarCamera;
		navis[0]->m_camera           = olimarCam;
		navis[0]->m_camera2          = olimarCam;
		Controller* olimarController = m_controllerP1;
		navis[0]->m_padinput         = olimarController;
		navis[0]->m_padinput2        = olimarController;
		PlayCamera* louieCam         = m_louieCamera;
		navis[1]->m_camera           = louieCam;
		navis[1]->m_camera2          = louieCam;
		Controller* louieController  = m_controllerP2;
		navis[1]->m_padinput         = louieController;
		navis[1]->m_padinput2        = louieController;

		moviePlayer->m_targetNavi   = navis[0];
		moviePlayer->m_actingCamera = m_olimarCamera;
		if (gameSystem->m_mode == GSM_STORY_MODE) {
			PSSetCurCameraNo(0);
		}
		break;
	}
	}
	on_setCamController(m_prevNaviIdx);
}

// "calcNearestTreasureMultiplayer__4GameFPQ25Radar3MgrR10Vector3<f>fR10Vector3<f>Rf"
int calcNearestTreasureMultiplayer(Radar::Mgr* mgr, Vector3f& onePlayerPos, f32 searchDist, Vector3f& treasurePos, f32& treasureDist)
{
	int searchResult = mgr->calcNearestTreasure(onePlayerPos, searchDist, treasurePos, treasureDist);

	if (TwoPlayer::isUsingTwoPlayer()) {
		Navi* navi            = naviMgr->getAt(1); // player two
		Vector3f twoPlayerPos = navi->getPosition();

		f32 treasureDist2;
		int searchResult2 = Radar::mgr->calcNearestTreasure(twoPlayerPos, searchDist, treasurePos, treasureDist2);

		if (treasureDist2 < treasureDist) {
			treasureDist = treasureDist2;
			searchResult = searchResult2;
		}
	}

	return searchResult;
}

} // namespace Game

#include "og/Screen/ScaleMgr.h"
#include "og/Screen/PikminCounter.h"
#include "og/Screen/ogScreen.h"
#include "og/Screen/callbackNodes.h"

namespace og {
namespace Screen {

void PikminCounter::setCallBackCommon(JKRArchive* arc, bool flag)
{
	og::Screen::setAlphaScreen(this);
	m_callBackCatchPiki = new og::Screen::CallBack_CatchPiki;
	m_callBackCatchPiki->init(this, 'piki', &m_dataNavi.m_nextThrowPiki, arc);
	addCallBack('piki', m_callBackCatchPiki);
	og::Screen::setCallBack_CounterRV(this, 'c_mr', 'c_mc', 'c_ml', &m_dataNavi.m_followPikis, 4, 3, 1, arc);
	m_isTotalPokoActive = flag;
}

void PikminCounter::setCallBackNoDay(JKRArchive* arc)
{
	setCallBackCommon(arc, false);

	setCallBack_CounterRV(this, 'c_lr', 'c_lc', 'c_ll', &m_dataGame.m_mapPikminCount, 10, 3, 1, arc);
	CallBack_CounterRV* counter = setCallBack_CounterRV(this, 'c_s1', 'c_sr', 'c_sl', &m_dataGame.m_totalPikminCount, 10, 4, 1, arc);
	counter->m_scaleUpSoundID   = PSSE_SY_PIKI_INCREMENT;
	counter->m_scaleDownSoundID = PSSE_SY_PIKI_DECREMENT;

	search('c_sc')->removeFromParent();
}

void PikminCounterCave::setCallBackNoDay(JKRArchive* arc)
{
	setCallBackCommon(arc, false);

	CallBack_CounterRV* counter = setCallBack_CounterRV(this, 'c_lr', 'c_lc', 'c_ll', &m_dataGame.m_mapPikminCount, 10, 3, 1, arc);
	counter->m_scaleUpSoundID   = PSSE_SY_PIKI_INCREMENT;
	counter->m_scaleDownSoundID = PSSE_SY_PIKI_DECREMENT;
}

} // namespace Screen

} // namespace og

namespace PSM {

u8 ObjCalc_SingleGame::getPlayerNo(PSM::Creature* obj)
{
	if (!TwoPlayer::twoPlayerActive || obj == nullptr) {
		return m_currCameraIndex;
	}
	// cursed typecast
	return reinterpret_cast<ObjCalc_2PGame*>(this)->ObjCalc_2PGame::getPlayerNo(obj);
}

u8 ObjCalc_SingleGame::getPlayerNo(Vec& vec)
{
	if (!TwoPlayer::twoPlayerActive) {
		return m_currCameraIndex;
	}
	// cursed typecast 2
	return reinterpret_cast<ObjCalc_2PGame*>(this)->ObjCalc_2PGame::getPlayerNo(vec);
}

} // namespace PSM
