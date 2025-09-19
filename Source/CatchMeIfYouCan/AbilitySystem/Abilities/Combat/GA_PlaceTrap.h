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
	// 🔥 새로 추가된 메서드: 소스 오브젝트에서 트랩 아이템 가져오기
	ACYTrapBase* GetTrapItemFromSource() const;
	
	ACYItemBase* FindTrapItemInInventory();
	ACYTrapBase* CreateTrapFromItem(ACYItemBase* TrapItem, const FVector& Location);
	FVector CalculateSpawnLocation();
	void ConsumeItemFromInventory(ACYItemBase* Item);
};