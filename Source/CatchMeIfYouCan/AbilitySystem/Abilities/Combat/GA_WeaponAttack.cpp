// GA_WeaponAttack.cpp - CatchMe 방식으로 단순화
#include "AbilitySystem/Abilities/Combat/GA_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Engine/Engine.h"
#include "Items/CYWeaponBase.h"

UGA_WeaponAttack::UGA_WeaponAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 하드코딩된 태그 사용 (안정성)
    FGameplayTag WeaponAttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.Combat.WeaponAttack"));
    FGameplayTag AttackingTag = FGameplayTag::RequestGameplayTag(FName("State.Combat.Attacking"));
    FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag(FName("State.Combat.Stunned"));
    FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Combat.Dead"));
    
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(WeaponAttackTag);
    SetAssetTags(AssetTags);
    
    FGameplayTagContainer OwnedTags;
    OwnedTags.AddTag(AttackingTag);
    ActivationOwnedTags = OwnedTags;
    
    FGameplayTagContainer BlockedTags;
    BlockedTags.AddTag(StunnedTag);
    BlockedTags.AddTag(DeadTag);
    ActivationBlockedTags = BlockedTags;
    
    UE_LOG(LogTemp, Warning, TEXT("GA_WeaponAttack created"));
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

    // 🔥 핵심: 직접 쿨다운 체크 (CatchMe 방식)
    if (IsOnCooldown(ActorInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("Weapon attack on cooldown"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 공격 수행
    PerformAttack();

    // 🔥 쿨다운 직접 적용 (CatchMe 방식)
    ApplyWeaponCooldown(Handle, ActorInfo, ActivationInfo);

    UE_LOG(LogTemp, Warning, TEXT("Weapon attack completed"));
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
        UE_LOG(LogTemp, Warning, TEXT("Weapon attack HIT"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Weapon attack MISS"));
    }
}

bool UGA_WeaponAttack::IsOnCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
    // 🔥 직접 태그 체크 (CatchMe 방식)
    FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Cooldown.Combat.WeaponAttack"));
    return ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
}

void UGA_WeaponAttack::ProcessHitTarget(const FHitResult& HitResult)
{
    AActor* Target = HitResult.GetActor();
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (!TargetASC) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit target has no ASC: %s"), *Target->GetName());
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
        
        UE_LOG(LogTemp, Warning, TEXT("Applied weapon damage to: %s"), 
               *TargetASC->GetOwnerActor()->GetName());
    }
}

void UGA_WeaponAttack::ApplyWeaponCooldown(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 🔥 CatchMe 방식: 동적으로 태그 추가
    FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(UGE_WeaponAttackCooldown::StaticClass(), 1);
    if (CooldownSpec.IsValid())
    {
        // 🔥 핵심: 런타임에 쿨다운 태그 추가
        FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Cooldown.Combat.WeaponAttack"));
        if (CooldownTag.IsValid())
        {
            CooldownSpec.Data->DynamicGrantedTags.AddTag(CooldownTag);
            UE_LOG(LogTemp, Warning, TEXT("Added cooldown tag dynamically: %s"), *CooldownTag.ToString());
        }
        
        ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
        UE_LOG(LogTemp, Warning, TEXT("Weapon cooldown applied"));
    }
}