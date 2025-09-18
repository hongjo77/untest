#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/CYGameplayAbility.h"
#include "GA_WeaponAttack.generated.h"

class UAbilitySystemComponent;

UCLASS()
class CATCHMEIFYOUCAN_API UGA_WeaponAttack : public UCYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void PerformAttack();

	bool IsOnCooldown(const FGameplayAbilityActorInfo* ActorInfo) const;
	void ProcessHitTarget(const FHitResult& HitResult);
	void ApplyDamageToTarget(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult);
	void ApplyWeaponCooldown(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo);
};