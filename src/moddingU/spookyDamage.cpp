#include "Game/Piki.h"
#include "Game/EnemyBase.h"

#define SPOOKY_DAMAGE_MULT (2.5f)

namespace Game {

// getAttackDamageModifier__Q24Game4PikiFPQ24Game8Creature
f32 Piki::getAttackDamageModifier(Creature* creature)
{
	f32 attackDamage = getAttackDamage();
	if (!creature || !creature->isTeki()) {
		return attackDamage;
	}

	if (m_pikiKind == Red) {
		EnemyBase* enemy               = static_cast<EnemyBase*>(creature);
		EnemyTypeID::EEnemyTypeID type = enemy->getEnemyTypeID();
		if (type == EnemyTypeID::EnemyID_Mar || type == EnemyTypeID::EnemyID_BlueChappy || type == EnemyTypeID::EnemyID_BlueKochappy) {
			return attackDamage * SPOOKY_DAMAGE_MULT;
		}
	}

	return attackDamage;
}

} // namespace Game
