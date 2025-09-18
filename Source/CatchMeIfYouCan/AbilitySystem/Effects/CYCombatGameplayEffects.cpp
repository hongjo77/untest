// CYCombatGameplayEffects.cpp - 안전한 효과 적용

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

// ✅ 간소화된 슬로우 트랩 - 블루프린트 호환성 고려
UGE_SlowTrap::UGE_SlowTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(5.0f);
    
    // ✅ 기본 설정만 하고 세부 Modifier는 블루프린트에서 설정하도록 유도
    // 또는 PostInitProperties에서 설정
}

void UGE_SlowTrap::PostInitProperties()
{
    Super::PostInitProperties();
    
    // ✅ PostInitProperties에서 안전하게 Modifier 설정
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return; // CDO가 아닌 경우만 설정
    }
    
    // ✅ Modifier 재설정
    Modifiers.Empty();
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    
    FScalableFloat SlowSpeed;
    SlowSpeed.Value = 50.0f; // 느린 속도
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SlowSpeed);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("🧊 SlowTrap Effect: PostInit - Set to 50 speed for 5 seconds"));
}

UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(3.0f);
}

void UGE_ImmobilizeTrap::PostInitProperties()
{
    Super::PostInitProperties();
    
    // ✅ PostInitProperties에서 안전하게 Modifier 설정
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return; // CDO가 아닌 경우만 설정
    }
    
    // ✅ Modifier 재설정
    Modifiers.Empty();
    
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    
    FScalableFloat FreezeSpeed;
    FreezeSpeed.Value = 0.0f; // 완전 정지
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FreezeSpeed);
    
    Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("❄️ FreezeTrap Effect: PostInit - Set to 0 speed for 3 seconds"));
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