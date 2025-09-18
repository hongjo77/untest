﻿#include "GA_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "Components/Items/CYWeaponComponent.h"

UGA_WeaponAttack::UGA_WeaponAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(CYGameplayTags::Ability_Combat_WeaponAttack);
	SetAssetTags(AssetTags);
    
	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(CYGameplayTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;
    
	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(CYGameplayTags::State_Combat_Stunned);
	BlockedTags.AddTag(CYGameplayTags::State_Combat_Dead);
	ActivationBlockedTags = BlockedTags;
}

void UGA_WeaponAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 쿨다운 체크
    if (IsOnCooldown(ActorInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 공격 수행
    PerformAttack();

    // 쿨다운 적용
    ApplyWeaponCooldown(Handle, ActorInfo, ActivationInfo);

    UE_LOG(LogTemp, Log, TEXT("Weapon attack completed"));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_WeaponAttack::PerformAttack()
{
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor) return;

    UCYWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UCYWeaponComponent>();
    if (!WeaponComp) return;

    FHitResult HitResult;
    if (WeaponComp->PerformLineTrace(HitResult))
    {
        ProcessHitTarget(HitResult);
    }
}

bool UGA_WeaponAttack::IsOnCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
    const FGameplayTagContainer* CooldownTags = GetCooldownTags();
    return CooldownTags && ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(*CooldownTags);
}

void UGA_WeaponAttack::ProcessHitTarget(const FHitResult& HitResult)
{
    AActor* Target = HitResult.GetActor();
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (!TargetASC) return;

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

        UE_LOG(LogTemp, Log, TEXT("Applied weapon damage to %s"), *HitResult.GetActor()->GetName());
    }
}

void UGA_WeaponAttack::ApplyWeaponCooldown(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(UGE_WeaponAttackCooldown::StaticClass(), 1);
    if (CooldownSpec.IsValid())
    {
        ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
    }
}