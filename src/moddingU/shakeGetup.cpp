#include "Game/Navi.h"
#include "Game/NaviState.h"
#include "Game/MoviePlayer.h"

namespace Game {

void NaviKokeDamageState::exec(Navi* navi)
{
	if (moviePlayer && moviePlayer->m_demoState != 0) {
		transit(navi, NSID_Walk, nullptr);
	} else if (gameSystem && !gameSystem->isFlag(32)) {
		transit(navi, NSID_Walk, nullptr);
	} else {
		if (navi->m_padinput && navi->m_padinput->isButtonDown(0xF000000)) {
			mEscapeInputs++;
		}

		navi->m_velocity  = 0.0f;
		navi->m_position2 = 0.0f;
		if (mState == 1) {
			f32 dt = sys->m_deltaTime;
			if (mEscapeInputs > 0) {
				dt *= mEscapeInputs;
			}

			mTimer -= dt;
			if (mTimer <= 0.0f) {
				navi->startMotion(IPikiAnims::GETUP, IPikiAnims::GETUP, navi, nullptr);
				mState = 2;
			}
		}
		if (mState == 0 && !navi->assertMotion(IPikiAnims::JKOKE)) {
			if (static_cast<NaviFSM*>(m_stateMachine)->_1C == -1) {
				transit(navi, NSID_Walk, nullptr);
			} else {
				transit(navi, static_cast<NaviFSM*>(m_stateMachine)->_1C, nullptr);
			}
		}
		if (mState == 2 && !navi->assertMotion(IPikiAnims::GETUP)) {
			if (static_cast<NaviFSM*>(m_stateMachine)->_1C == -1) {
				transit(navi, NSID_Walk, nullptr);
			} else {
				transit(navi, static_cast<NaviFSM*>(m_stateMachine)->_1C, nullptr);
			}
		}
	}
}

} // namespace Game
