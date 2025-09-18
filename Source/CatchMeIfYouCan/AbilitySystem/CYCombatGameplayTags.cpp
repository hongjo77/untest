#include "CYCombatGameplayTags.h"

namespace CYGameplayTags
{
	// Combat Ability Tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_WeaponAttack, "Ability.Combat.WeaponAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_PlaceTrap, "Ability.Combat.PlaceTrap");

	// Item Tags
	UE_DEFINE_GAMEPLAY_TAG(Item_Base, "Item.Base");
	UE_DEFINE_GAMEPLAY_TAG(Item_Weapon, "Item.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trap, "Item.Trap");
	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable, "Item.Consumable");

	// State Tags
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Attacking, "State.Combat.Attacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Stunned, "State.Combat.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Dead, "State.Combat.Dead");

	// Cooldown Tags
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Combat_WeaponAttack, "Cooldown.Combat.WeaponAttack");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Combat_TrapPlace, "Cooldown.Combat.TrapPlace");

	// Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Use, "Event.Item.Use");

	// Data Tags
	UE_DEFINE_GAMEPLAY_TAG(Data_Combat_Damage, "Data.Combat.Damage");
}