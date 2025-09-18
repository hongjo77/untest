// GA_WeaponAttack.cpp - 쿨다운 확실히 보장

#include "GA_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "Components/Items/CYWeaponComponent.h"

UGA_WeaponAttack::UGA_WeaponAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // ✅ 어빌리티 태그 설정
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(CYGameplayTags::Ability_Combat_WeaponAttack);
    SetAssetTags(AssetTags);
    
    // ✅ 활성화 중 소유 태그 - 중복 실행 방지
    FGameplayTagContainer OwnedTags;
    OwnedTags.AddTag(CYGameplayTags::State_Combat_Attacking);
    ActivationOwnedTags = OwnedTags;
    
    // ✅ 차단 태그 - 쿨다운 중, 공격 중, 스턴 중 차단
    FGameplayTagContainer BlockedTags;
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Stunned);
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Dead);
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Attacking); // ✅ 공격 중 추가 공격 차단
    BlockedTags.AddTag(CYGameplayTags::Cooldown_Combat_WeaponAttack); // ✅ 쿨다운 중 차단
    ActivationBlockedTags = BlockedTags;
}

bool UGA_WeaponAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// ✅ 쿨다운 먼저 체크 - 빠른 리턴
	if (IsOnCooldown(ActorInfo))
	{
		UE_LOG(LogTemp, Verbose, TEXT("🚫 WeaponAttack: On cooldown - blocked"));
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogTemp, Verbose, TEXT("🚫 WeaponAttack: Base CanActivateAbility failed"));
		return false;
	}

	return true;
}

void UGA_WeaponAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("⚔️ WeaponAttack: ActivateAbility START"));

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: No authority or prediction key"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ✅ 최종 쿨다운 체크만 유지 (다른 체크 제거)
	if (IsOnCooldown(ActorInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ WeaponAttack: On cooldown during activation - aborting"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ✅ 즉시 쿨다운 적용 (중복 실행 차단)
	ApplyWeaponCooldown(Handle, ActorInfo, ActivationInfo);

	// ✅ Cost 커밋
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: Failed to commit ability cost"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ✅ 공격 수행
	bool bAttackSuccess = PerformAttack();
    
	if (bAttackSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ WeaponAttack: Attack HIT target"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("🎯 WeaponAttack: Attack missed (no target)"));
	}

	UE_LOG(LogTemp, Warning, TEXT("⚔️ WeaponAttack: ActivateAbility END"));
    
	// ✅ 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_WeaponAttack::PerformAttack()
{
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor) 
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: No owner actor"));
        return false;
    }

    UCYWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UCYWeaponComponent>();
    if (!WeaponComp) 
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: No weapon component"));
        return false;
    }

    // ✅ 라인 트레이스 수행
    FHitResult HitResult;
    if (WeaponComp->PerformLineTrace(HitResult))
    {
        ProcessHitTarget(HitResult);
        return true;
    }
    else
    {
        return false; // 타겟 없음
    }
}

bool UGA_WeaponAttack::IsOnCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
    {
        return false;
    }

    // ✅ 쿨다운 태그 확인
    bool bHasCooldownTag = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CYGameplayTags::Cooldown_Combat_WeaponAttack);
    
    if (bHasCooldownTag)
    {
        UE_LOG(LogTemp, Verbose, TEXT("🚫 WeaponAttack: Has cooldown tag"));
    }
    
    return bHasCooldownTag;
}

void UGA_WeaponAttack::ProcessHitTarget(const FHitResult& HitResult)
{
    AActor* Target = HitResult.GetActor();
    if (!Target) return;

    UE_LOG(LogTemp, Warning, TEXT("🎯 WeaponAttack: Hit target %s"), *Target->GetName());

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (!TargetASC) 
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 WeaponAttack: Target has no ASC"));
        return;
    }

    ApplyDamageToTarget(TargetASC, HitResult);
}

void UGA_WeaponAttack::ApplyDamageToTarget(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult)
{
    FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
    EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
    EffectContext.AddHitResult(HitResult);

    FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(UGE_WeaponDamage::StaticClass(), 1);
    if (DamageSpec.IsValid())
    {
        GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
            *DamageSpec.Data.Get(),
            TargetASC
        );

        UE_LOG(LogTemp, Warning, TEXT("💀 WeaponAttack: Applied damage to %s"), 
               *HitResult.GetActor()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: Failed to create damage spec"));
    }
}

// ✅ 즉시 쿨다운 적용 (중복 실행 완전 차단)
void UGA_WeaponAttack::ApplyWeaponCooldown(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UE_LOG(LogTemp, Log, TEXT("⏰ WeaponAttack: Applying cooldown..."));
    
    FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(UGE_WeaponAttackCooldown::StaticClass(), 1);
    if (CooldownSpec.IsValid())
    {
        FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
        
        if (EffectHandle.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ WeaponAttack: Cooldown effect applied successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: Failed to apply cooldown effect"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WeaponAttack: Failed to create cooldown spec"));
    }
}