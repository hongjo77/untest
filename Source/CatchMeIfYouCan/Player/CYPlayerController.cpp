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

void ACYPlayerController::AttackPressed()
{
    // ✅ ControlledCharacter를 통해 WeaponComponent 접근
    ACYCharacterBase* ControlledCharacter = Cast<ACYCharacterBase>(GetPawn());
    if (ControlledCharacter && ControlledCharacter->WeaponComponent)
    {
        ControlledCharacter->WeaponComponent->PerformAttack();
    }
    
    // ✅ 인벤토리 상태 표시 (종속성 낮게 구현)
    if (IsLocalController())
    {
        DisplayInventoryStatus();
    }
}

void ACYPlayerController::DisplayInventoryStatus()
{
    if (!GEngine) return;

    ACYCharacterBase* ControlledCharacter = Cast<ACYCharacterBase>(GetPawn());
    if (!ControlledCharacter || !ControlledCharacter->InventoryComponent) return;

    UCYInventoryComponent* InventoryComp = ControlledCharacter->InventoryComponent;
    UCYWeaponComponent* WeaponComp = ControlledCharacter->WeaponComponent;

    // 기존 메시지 제거
    GEngine->ClearOnScreenDebugMessages();

    // 인벤토리 상태 표시
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("=== 📦 INVENTORY STATUS ==="));
    
    // 무기 슬롯 (1~3번 키)
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("🗡️ WEAPONS (Keys 1-3):"));
    for (int32 i = 0; i < InventoryComp->WeaponSlots.Num(); ++i)
    {
        FString WeaponInfo;
        if (InventoryComp->WeaponSlots[i])
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 1, 
                *InventoryComp->WeaponSlots[i]->ItemName.ToString(), 
                InventoryComp->WeaponSlots[i]->ItemCount
            );
            
            // ✅ 타입 캐스팅으로 비교 문제 해결
            ACYWeaponBase* SlotWeapon = Cast<ACYWeaponBase>(InventoryComp->WeaponSlots[i]);
            if (WeaponComp && WeaponComp->CurrentWeapon && WeaponComp->CurrentWeapon == SlotWeapon)
            {
                WeaponInfo += TEXT(" ⭐ EQUIPPED");
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, WeaponInfo);
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, WeaponInfo);
            }
        }
        else
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] Empty"), i + 1);
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Silver, WeaponInfo);
        }
    }
    
    // 아이템 슬롯 (4~9번 키)
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("🎒 ITEMS (Keys 4-9):"));
    int32 MaxDisplayItems = FMath::Min(6, InventoryComp->ItemSlots.Num());
    for (int32 i = 0; i < MaxDisplayItems; ++i)
    {
        FString ItemInfo;
        if (InventoryComp->ItemSlots[i])
        {
            ItemInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 4, 
                *InventoryComp->ItemSlots[i]->ItemName.ToString(), 
                InventoryComp->ItemSlots[i]->ItemCount
            );
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, ItemInfo);
        }
        else
        {
            ItemInfo = FString::Printf(TEXT("  [%d] Empty"), i + 4);
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Silver, ItemInfo);
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("=================="));
}