#include "Dolphin/rand.h"
#include "Game/Entities/ItemOnyon.h"
#include "Game/gamePlayData.h"
#include "Game/pelletMgr.h"
#include "Game/gameStat.h"

namespace Game {

Onyon* Pellet::getPelletGoal()
{
	// All pellet types except number pellets and carcasses should default to the ship, if available.
	// Otherwise, they should go for the pod.
	if (!gameSystem->isVersusMode() && (gameSystem->m_inCave || getKind() != 0 && getKind() != 1)) {
		if (ItemOnyon::mgr->m_ufo == nullptr) {
			return ItemOnyon::mgr->m_pod;
		}

		return ItemOnyon::mgr->m_ufo;
	}

	u32 maxCount   = 0;
	int minCount   = 100;
	u8 minimumType = ONYON_TYPE_RED;

	for (u8 i = 0; i < OnyonCount; i++) {
		// Ignore unbooted onions.
		if (gameSystem->isStoryMode() && !playData->hasBootContainer(i)) {
			continue;
		}

		// Valid pellets should go to the onion of the pikmin with the most carriers.
		if (maxCount < m_pikminCount[i]) {
			maxCount = m_pikminCount[i];
		}

		// Get the onion with the least pikmin for later use.
		int containerPikis = GameStat::getAllPikmins(i);
		if (minCount > containerPikis) {
			minimumType = i;
			minCount    = containerPikis;
		}
	}

	u8 selectedType = selectRandomGoalWinner(maxCount);

	// In story mode, if maxCount is still zero, there are no carriers that have an onion.
	if (gameSystem->isStoryMode() && (maxCount == 0 || !playData->hasBootContainer(selectedType))) {
		// Onion-less pikmin should carry to the onion with the least pikmin.
		selectedType = minimumType;

		// Number pellets should try and use the pellet color
		if (getKind() == 0 && m_pelletColor <= PELCOLOR_YELLOW && playData->hasBootContainer(m_pelletColor)) {
			// For number pellets, onion-less pikmin should carry to the respective onion color of the pellet.
			selectedType = m_pelletColor;
		}
	}

	Onyon* goalOnyon = ItemOnyon::mgr->getOnyon(selectedType);
	if (goalOnyon == nullptr) {
		// Default to the ship/pod.
		if (ItemOnyon::mgr->m_ufo == nullptr) {
			return ItemOnyon::mgr->m_pod;
		}

		return ItemOnyon::mgr->m_ufo;
	}

	return goalOnyon;
}

/**
 * Since the iteration goes by pikmin type order, there is bias in which type wins the "tug of war"
 * To avoid this, we store all the pikmin types that win the tug of war and randomly pick between them.
 */
u8 Pellet::selectRandomGoalWinner(u32 maxCount)
{
	u8 i       = 0;
	u8 counter = 0;

	u8 onyonType[OnyonCount];
	for (u8 j = 0; j < OnyonCount; j++) {
		// One pellets have no randomization.
		// They should always go to the corresponding onion color if 1 of the respective pikmin type are carrying it.
		if (getKind() == 0 && _43C == 0 && m_pikminCount[j] >= 1 && m_pelletColor == j) {
			return j;
		}

		if (maxCount == m_pikminCount[j]) {
			onyonType[i++] = j;
			counter++;
		}
	}

	u8 idx = (u8)(counter * randFloat());
	if (idx >= counter) {
		idx = ONYON_TYPE_BLUE;
	}

	return onyonType[idx];
}

} // namespace Game
