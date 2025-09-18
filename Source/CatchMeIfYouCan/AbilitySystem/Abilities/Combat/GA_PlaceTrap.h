#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/CYGameplayAbility.h"
#include "GA_PlaceTrap.generated.h"

class ACYTrapBase;
class ACYItemBase;

UCLASS()
class CATCHMEIFYOUCAN_API UGA_PlaceTrap : public UCYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlaceTrap();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	ACYItemBase* FindTrapItemInInventory();
	ACYTrapBase* CreateTrapFromItem(ACYItemBase* TrapItem, const FVector& Location);
	FVector CalculateSpawnLocation();
	void ConsumeItemFromInventory(ACYItemBase* Item);
};