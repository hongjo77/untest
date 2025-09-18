// CYCombatGameplayEffects.cpp - Deprecated 경고 수정

#include "CYCombatGameplayEffects.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "AbilitySystem/CYCombatGameplayTags.h"

// ✅ 기본 이동속도 설정 Effect (Infinite) - 변경 없음
UGE_InitialCombatStats::UGE_InitialCombatStats()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    
    // Health 설정
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Override;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(HealthModifier);
    
    // MaxHealth 설정
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.Attribute = UCYCombatAttributeSet::GetMaxHealthAttribute();
    MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
    MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(100.0f);
    Modifiers.Add(MaxHealthModifier);
    
    // ✅ 기본 MoveSpeed 설정 (600) - Override 유지
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(600.0f);
    Modifiers.Add(MoveSpeedModifier);
    
    // AttackPower 설정
    FGameplayModifierInfo AttackPowerModifier;
    AttackPowerModifier.Attribute = UCYCombatAttributeSet::GetAttackPowerAttribute();
    AttackPowerModifier.ModifierOp = EGameplayModOp::Override;
    AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(50.0f);
    Modifiers.Add(AttackPowerModifier);
}

UGE_WeaponAttackCooldown::UGE_WeaponAttackCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(1.0f);
    
    // ✅ 코드로 직접 Granted Tags 설정
    FGameplayTagContainer GrantedTags;
    GrantedTags.AddTag(CYGameplayTags::Cooldown_Combat_WeaponAttack);
    
    // ✅ InheritableGameplayEffectTags 사용 (새로운 방식)
    InheritableGameplayEffectTags.CombinedTags = GrantedTags;
    InheritableGameplayEffectTags.Added = GrantedTags;
}

// ✅ 트랩 배치 쿨다운
UGE_TrapPlaceCooldown::UGE_TrapPlaceCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
    
    // ✅ 코드로 직접 Granted Tags 설정
    FGameplayTagContainer GrantedTags;
    GrantedTags.AddTag(CYGameplayTags::Cooldown_Combat_TrapPlace);
    
    InheritableGameplayEffectTags.CombinedTags = GrantedTags;
    InheritableGameplayEffectTags.Added = GrantedTags;
}

// ✅ 슬로우 트랩 - 디버깅 로그 추가
UGE_SlowTrap::UGE_SlowTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    // ✅ Override로 직접 느린 속도 설정
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(50.0f);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("🧊 SlowTrap GameplayEffect constructor: Override to 50 speed"));
}

// ✅ 프리즈 트랩 - 디버깅 로그 추가
UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
    
    // ✅ Override로 완전 정지
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(0.0f);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("❄️ ImmobilizeTrap GameplayEffect constructor: Override to 0 speed"));
}

// ✅ 무기 데미지 - 즉시 적용
UGE_WeaponDamage::UGE_WeaponDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(-50.0f);
    
    Modifiers.Add(HealthModifier);
}

// ✅ 이동속도 조절 효과 - 기존 유지
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