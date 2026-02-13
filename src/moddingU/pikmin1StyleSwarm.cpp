#include "Game/Piki.h"
#include "Game/Navi.h"
#include "PikiAI.h"
#include "Game/PikiState.h"

namespace Game {

bool Piki::ignoreAtari(Creature* creature)
{
	if (creature->isPiki() && getCurrActionID() == 0 && m_navi && m_navi->commandOn()) {
		return true;
	}

	bool result = false;
	if (m_currentState) {
		result = m_currentState->ignoreAtari(this, creature);
	}

	return result;
}


} // namespace Game