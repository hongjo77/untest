#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "AbilitySystem/CYCombatGameplayTags.h"

// 기본 스탯 초기화 이펙트
UGE_InitialCombatStats::UGE_InitialCombatStats()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    
    // 체력 초기화
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Override;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f)); // 🔥 올바른 방법
    Modifiers.Add(HealthModifier);
    
    // 최대 체력 초기화
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.Attribute = UCYCombatAttributeSet::GetMaxHealthAttribute();
    MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
    MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f)); // 🔥 올바른 방법
    Modifiers.Add(MaxHealthModifier);
    
    // 이동속도 초기화
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
    MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(600.0f)); // 🔥 올바른 방법
    Modifiers.Add(MoveSpeedModifier);
    
    // 공격력 초기화
    FGameplayModifierInfo AttackPowerModifier;
    AttackPowerModifier.Attribute = UCYCombatAttributeSet::GetAttackPowerAttribute();
    AttackPowerModifier.ModifierOp = EGameplayModOp::Override;
    AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(50.0f)); // 🔥 올바른 방법
    Modifiers.Add(AttackPowerModifier);
}

// 무기 데미지 이펙트 (즉시 적용)
UGE_WeaponDamage::UGE_WeaponDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    
    FGameplayModifierInfo HealthModifier;
    HealthModifier.Attribute = UCYCombatAttributeSet::GetHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::Additive;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-50.0f)); // 🔥 올바른 방법, 50 데미지
    Modifiers.Add(HealthModifier);
}

// 무기 공격 쿨다운 이펙트 (2초) - 쿨다운 태그 추가
UGE_WeaponAttackCooldown::UGE_WeaponAttackCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.0f)); // 🔥 올바른 방법
    
    // 쿨다운 태그 설정 - 이것이 핵심!
    FGameplayTagContainer CooldownTags;
    CooldownTags.AddTag(CYGameplayTags::Cooldown_Combat_WeaponAttack);
    InheritableOwnedTagsContainer.Added = CooldownTags;
    
    UE_LOG(LogTemp, Warning, TEXT("🛠️ WeaponAttackCooldown GE created with tag"));
}

// 트랩 배치 쿨다운 이펙트 (3초) - 쿨다운 태그 추가
UGE_TrapPlaceCooldown::UGE_TrapPlaceCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f)); // 🔥 올바른 방법
    
    // 쿨다운 태그 설정
    FGameplayTagContainer CooldownTags;
    CooldownTags.AddTag(CYGameplayTags::Cooldown_Combat_TrapPlace);
    InheritableOwnedTagsContainer.Added = CooldownTags;
}

// 슬로우 트랩 이펙트 (이동속도 50% 감소, 5초 지속)
UGE_SlowTrap::UGE_SlowTrap()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f)); // 🔥 올바른 방법
    
	// 이동속도를 50% 감소
	FGameplayModifierInfo MoveSpeedModifier;
	MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
	MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
	MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(300.0f)); // 🔥 올바른 방법
	Modifiers.Add(MoveSpeedModifier);
    
    // 디버프 태그 설정
    FGameplayTagContainer DebuffTags;
    DebuffTags.AddTag(CYGameplayTags::Effect_Debuff_Slow);
    InheritableOwnedTagsContainer.Added = DebuffTags;
    
    UE_LOG(LogTemp, Warning, TEXT("🛠️ SlowTrap GE created: 600 -> 300"));
}

// 프리즈 트랩 이펙트 (완전 정지, 3초 지속)
UGE_ImmobilizeTrap::UGE_ImmobilizeTrap()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f)); // 🔥 올바른 방법
    
	// 이동속도를 0으로 설정
	FGameplayModifierInfo MoveSpeedModifier;
	MoveSpeedModifier.Attribute = UCYCombatAttributeSet::GetMoveSpeedAttribute();
	MoveSpeedModifier.ModifierOp = EGameplayModOp::Override;
	MoveSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.0f)); // 🔥 올바른 방법
	Modifiers.Add(MoveSpeedModifier);
    
    // 디버프 태그 설정
    FGameplayTagContainer DebuffTags;
    DebuffTags.AddTag(CYGameplayTags::Effect_Debuff_Freeze);
    InheritableOwnedTagsContainer.Added = DebuffTags;
    
    UE_LOG(LogTemp, Warning, TEXT("🛠️ ImmobilizeTrap GE created: 600 -> 0"));
}