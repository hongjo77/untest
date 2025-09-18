// Fill out your copyright notice in the Description page of Project Settings.

#include "CYPlayerController.h"

#include "CYPlayerState.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "Character/CYCharacterBase.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Items/CYItemBase.h"
#include "Items/CYWeaponBase.h" // ✅ WeaponBase 헤더 추가

ACYPlayerController::ACYPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ACYPlayerState* ACYPlayerController::GetCYPlayerState() const
{
	return CastChecked<ACYPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UCYAbilitySystemComponent* ACYPlayerController::GetCYAbilitySystemComponent() const
{
	const ACYPlayerState* CYPS = GetCYPlayerState();
	return (CYPS ? CYPS->GetCYAbilitySystemComponent() : nullptr);
}

void ACYPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UCYAbilitySystemComponent* CYASC = GetCYAbilitySystemComponent())
	{
		CYASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);
}