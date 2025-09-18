// CYCombatGameplayEffects.cpp - 트랩 효과 제대로 작동하도록 수정

#include "CYCombatGameplayEffects.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "AbilitySystem/CYCombatGameplayTags.h"

// 기본 이동속도 설정 Effect
UGE_InitialCombatStats::UGE_InitialCombatStats()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Override;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(HealthModifier);
    
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.Attribute = UCYCombatAttributeSet::GetMaxHealthAttribute();
    MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
    MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(MaxHealthModifier);
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(600.0f);
    Modifiers.Add(MoveSpeedModifier);
    
    FGameplayModifierInfo AttackPowerModifier;
    AttackPowerModifier.Attribute = UCYCombatAttributeSet::GetAttackPowerAttribute();
    AttackPowerModifier.ModifierOp = EGameplayModOp::Override;
    AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(50.0f);
    Modifiers.Add(AttackPowerModifier);
}

UGE_WeaponAttackCooldown::UGE_WeaponAttackCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(2.0f);
    
    FGameplayTagContainer GrantedTags;
    GrantedTags.AddTag(CYGameplayTags::Cooldown_Combat_WeaponAttack);
    
    InheritableGameplayEffectTags.CombinedTags = GrantedTags;
    InheritableGameplayEffectTags.Added = GrantedTags;
}

UGE_TrapPlaceCooldown::UGE_TrapPlaceCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
    
    FGameplayTagContainer GrantedTags;
    GrantedTags.AddTag(CYGameplayTags::Cooldown_Combat_TrapPlace);
    
    InheritableGameplayEffectTags.CombinedTags = GrantedTags;
    InheritableGameplayEffectTags.Added = GrantedTags;
}

// ✅ 제대로 작동하는 슬로우 트랩
UGE_SlowTrap::UGE_SlowTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    // ✅ 제대로 된 ModifierInfo 설정
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    
    // ✅ Scalable Float 사용
    FScalableFloat SlowSpeed;
    SlowSpeed.Value = 50.0f; // 느린 속도
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SlowSpeed);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("🧊 SlowTrap Effect: Set to 50 speed for 5 seconds"));
}

UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    
    // ✅ Scalable Float 사용
    FScalableFloat FreezeSpeed;
    FreezeSpeed.Value = 0.0f; // 완전 정지
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FreezeSpeed);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("❄️ FreezeTrap Effect: Set to 0 speed for 3 seconds"));
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