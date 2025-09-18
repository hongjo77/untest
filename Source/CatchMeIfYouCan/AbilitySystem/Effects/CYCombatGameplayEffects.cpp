#include "CYCombatGameplayEffects.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"

UGE_MovementModifier::UGE_MovementModifier()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(0.0f);
    
    Modifiers.Add(MoveSpeedModifier);
}

UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(0.0f);
    
    Modifiers.Add(MoveSpeedModifier);
}

UGE_SlowTrap::UGE_SlowTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(50.0f);
    
    Modifiers.Add(MoveSpeedModifier);
}

UGE_WeaponDamage::UGE_WeaponDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(-50.0f);
    
    Modifiers.Add(HealthModifier);
}

UGE_WeaponAttackCooldown::UGE_WeaponAttackCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(1.0f);
}

UGE_TrapPlaceCooldown::UGE_TrapPlaceCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
}

UGE_InitialCombatStats::UGE_InitialCombatStats()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    
    // Health 설정
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(HealthModifier);
    
    // MaxHealth 설정
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.Attribute = UCYCombatAttributeSet::GetMaxHealthAttribute();
    MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
    MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(MaxHealthModifier);
    
    // MoveSpeed 설정
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Additive;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(600.0f);
    Modifiers.Add(MoveSpeedModifier);
    
    // AttackPower 설정
    FGameplayModifierInfo AttackPowerModifier;
    AttackPowerModifier.Attribute = UCYCombatAttributeSet::GetAttackPowerAttribute();
    AttackPowerModifier.ModifierOp = EGameplayModOp::Additive;
    AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(50.0f);
    Modifiers.Add(AttackPowerModifier);
}