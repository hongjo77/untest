#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/CYGameplayAbility.h"
#include "Items/CYTrapData.h"
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

	ACYItemBase* GetSourceItemFromAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void LogTrapCreationInfo(ACYItemBase* SourceItem);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	ACYTrapBase* CreateTrapFromSourceItem(ACYItemBase* SourceItem, 
		const FVector& SpawnLocation, const FRotator& SpawnRotation, AActor* OwnerActor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ConfigureNewTrap(ACYTrapBase* NewTrap, ACYItemBase* SourceItem);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ShowTrapPlacementSuccess(ACYTrapBase* NewTrap);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	FVector CalculateSpawnLocation(AActor* OwnerActor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	ACYItemBase* FindValidTrapItemInInventory(AActor* OwnerActor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ConsumeSpecificItemFromInventory(AActor* OwnerActor, ACYItemBase* SourceItem);

	void ApplyTrapCooldown(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo);
};