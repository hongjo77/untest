// CYCombatGameplayEffects.h - PostInitProperties 선언

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "CYCombatGameplayEffects.generated.h"

// 이동속도 조절 효과
UCLASS()
class CATCHMEIFYOUCAN_API UGE_MovementModifier : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_MovementModifier();
};

// ✅ 완전 정지 트랩 - PostInitProperties 추가
UCLASS()
class CATCHMEIFYOUCAN_API UGE_ImmobilizeTrap : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ImmobilizeTrap();
	
	// ✅ PostInitProperties에서 안전한 Modifier 설정
	virtual void PostInitProperties() override;
};

// ✅ 슬로우 트랩 - PostInitProperties 추가
UCLASS()
class CATCHMEIFYOUCAN_API UGE_SlowTrap : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_SlowTrap();
	
	// ✅ PostInitProperties에서 안전한 Modifier 설정
	virtual void PostInitProperties() override;
};

// 무기 데미지
UCLASS()
class CATCHMEIFYOUCAN_API UGE_WeaponDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_WeaponDamage();
};

// ✅ 무기 공격 쿨다운 - 단순화
UCLASS(BlueprintType)
class CATCHMEIFYOUCAN_API UGE_WeaponAttackCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_WeaponAttackCooldown();
};

// ✅ 트랩 배치 쿨다운 - 단순화
UCLASS(BlueprintType)
class CATCHMEIFYOUCAN_API UGE_TrapPlaceCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_TrapPlaceCooldown();
};

// 초기 스탯
UCLASS()
class CATCHMEIFYOUCAN_API UGE_InitialCombatStats : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_InitialCombatStats();
};