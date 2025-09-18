// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CYPlayerController.generated.h"

class ACYPlayerState;
class UCYAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class CATCHMEIFYOUCAN_API ACYPlayerController : public APlayerController
{
	GENERATED_BODY()
    
public:
    
	ACYPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "CY|PlayerController")
	ACYPlayerState* GetCYPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "CY|PlayerController")
	UCYAbilitySystemComponent* GetCYAbilitySystemComponent() const;

	// ✅ 인벤토리 표시 함수 (임시 디버깅용)
	UFUNCTION(BlueprintCallable, Category = "CY|Inventory")
	void DisplayInventoryStatus();
	UFUNCTION(BlueprintCallable, Category = "CY|Input")
	void AttackPressed();

protected:

	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
};