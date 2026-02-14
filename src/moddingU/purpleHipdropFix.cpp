#include "Game/Piki.h"
#include "Game/PikiState.h"
#include "efx/TPk.h"

namespace Game {

void PikiHipDropState::cleanup(Piki* piki)
{
	piki->m_effectsObj->killBlackDown_();
	piki->m_updateContext._09 = false;

	if (piki->m_sticker != nullptr && !piki->m_sticker->isAlive()) {
		piki->endStick();
	}
}

} // namespace Game
