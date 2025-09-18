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

    // 태그 설정
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(CYGameplayTags::Ability_Combat_WeaponAttack);
    SetAssetTags(AssetTags);
    
    // 블로킹 태그 설정
    FGameplayTagContainer BlockedTags;
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Stunned);
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Dead);
    ActivationBlockedTags = BlockedTags;
    
    // 쿨다운 태그 설정
    FGameplayTagContainer CooldownTags;
    CooldownTags.AddTag(CYGameplayTags::Cooldown_Combat_WeaponAttack);
    CancelAbilitiesWithTag = CooldownTags;
}

bool UGA_WeaponAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 쿨다운 체크
    if (IsOnCooldown(ActorInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon attack on cooldown"));
        return false;
    }

    return true;
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

    // 공격 수행
    bool bAttackSuccess = PerformAttack();

    if (bAttackSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon attack HIT"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("⚔️ ATTACK HIT!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon attack MISS"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("❌ NO TARGET"));
        }
    }

    // 쿨다운 적용
    if (GetCooldownGameplayEffect())
    {
        ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, GetCooldownGameplayEffect(), 1);
    }
    else
    {
        // 기본 쿨다운 적용
        FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(UGE_WeaponAttackCooldown::StaticClass(), 1);
        if (CooldownSpec.IsValid())
        {
            ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_WeaponAttack::PerformAttack()
{
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor) return false;

    UCYWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UCYWeaponComponent>();
    if (!WeaponComp || !WeaponComp->CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ No weapon equipped"));
        return false;
    }

    // 라인 트레이스로 타겟 찾기
    FHitResult HitResult;
    float AttackRange = WeaponComp->CurrentWeapon->AttackRange;
    
    if (WeaponComp->PerformLineTrace(HitResult, AttackRange))
    {
        ProcessHitTarget(HitResult);
        return true;
    }
    
    return false;
}

void UGA_WeaponAttack::ProcessHitTarget(const FHitResult& HitResult)
{
    AActor* Target = HitResult.GetActor();
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (!TargetASC) 
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 Hit target has no ASC: %s"), *Target->GetName());
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
        
        UE_LOG(LogTemp, Warning, TEXT("💥 Applied weapon damage to: %s"), 
               *TargetASC->GetOwnerActor()->GetName());
    }
}

bool UGA_WeaponAttack::IsOnCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
    const FGameplayTagContainer* CooldownTags = GetCooldownTags();
    return CooldownTags && ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(*CooldownTags);
}