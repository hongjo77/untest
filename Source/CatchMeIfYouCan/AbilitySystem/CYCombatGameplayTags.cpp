#include "CYCombatGameplayTags.h"

namespace CYGameplayTags
{
	// Combat Ability Tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_WeaponAttack, "Ability.Combat.WeaponAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_PlaceTrap, "Ability.Combat.PlaceTrap");

	// State Tags
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Stunned, "State.Combat.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Dead, "State.Combat.Dead");

	// Cooldown Tags
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Combat_WeaponAttack, "Cooldown.Combat.WeaponAttack");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Combat_TrapPlace, "Cooldown.Combat.TrapPlace");

	// Effect Tags
	UE_DEFINE_GAMEPLAY_TAG(Effect_Debuff_Slow, "Effect.Debuff.Slow");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Debuff_Freeze, "Effect.Debuff.Freeze");
}