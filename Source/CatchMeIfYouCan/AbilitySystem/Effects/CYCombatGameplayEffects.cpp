// CYCombatGameplayEffects.cpp - CatchMe 방식으로 단순화
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "AbilitySystem/CYCombatGameplayTags.h"

// 🔥 기본 스탯 초기화 이펙트 - CatchMe 방식 (600)
UGE_InitialCombatStats::UGE_InitialCombatStats()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    
    // 체력 초기화
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f));
    Modifiers.Add(HealthModifier);
    
    // 최대 체력 초기화
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.Attribute = UCYCombatAttributeSet::GetMaxHealthAttribute();
    MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
    MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f));
    Modifiers.Add(MaxHealthModifier);
    
    // 🔥 핵심: CatchMe처럼 600으로 초기화
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Additive;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(600.0f));
    Modifiers.Add(MoveSpeedModifier);
    
    // 공격력 초기화
    FGameplayModifierInfo AttackPowerModifier;
    AttackPowerModifier.Attribute = UCYCombatAttributeSet::GetAttackPowerAttribute();
    AttackPowerModifier.ModifierOp = EGameplayModOp::Additive;
    AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(50.0f));
    Modifiers.Add(AttackPowerModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("InitialCombatStats GE created (MoveSpeed=600)"));
}

// 무기 데미지 이펙트 (즉시 적용)
UGE_WeaponDamage::UGE_WeaponDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-50.0f));
    Modifiers.Add(HealthModifier);
}

// 🔥 CatchMe 방식: 단순한 쿨다운 (태그 없음)
UGE_WeaponAttackCooldown::UGE_WeaponAttackCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f)); // 1초
    
    // 🔥 CatchMe 방식: 태그를 런타임에 추가
    UE_LOG(LogTemp, Warning, TEXT("WeaponAttackCooldown GE created"));
}

// 트랩 배치 쿨다운 이펙트 (3초)
UGE_TrapPlaceCooldown::UGE_TrapPlaceCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
    
    UE_LOG(LogTemp, Warning, TEXT("TrapPlaceCooldown GE created"));
}

// 🔥 CatchMe 방식: 슬로우 트랩 (50으로 Override)
UGE_SlowTrap::UGE_SlowTrap()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
    
	FGameplayModifierInfo MoveSpeedModifier;
	MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
	MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
	MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(50.0f)); // 느림
	Modifiers.Add(MoveSpeedModifier);
    
    // 🔥 CatchMe 방식: 동적 태그 추가는 적용 시점에서
    UE_LOG(LogTemp, Warning, TEXT("SlowTrap GE created: 600->50"));
}

// 🔥 CatchMe 방식: 프리즈 트랩 (0으로 Override)
UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f)); // 🔥 3초로 단축
    
	FGameplayModifierInfo MoveSpeedModifier;
	MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
	MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
	MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.0f)); // 완전 정지
	Modifiers.Add(MoveSpeedModifier);
    
    UE_LOG(LogTemp, Warning, TEXT("ImmobilizeTrap GE created: 600->0"));
}