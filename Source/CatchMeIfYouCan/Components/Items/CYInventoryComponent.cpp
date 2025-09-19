﻿// CYInventoryComponent.cpp - HeldItem 시스템 구현
#include "Components/Items/CYInventoryComponent.h"
#include "Items/CYItemBase.h"
#include "Items/CYWeaponBase.h"
#include "Items/Traps/CYTrapBase.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/CYPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

UCYInventoryComponent::UCYInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    bIsProcessingUse = false;
    CurrentHeldItem = nullptr;
}

void UCYInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 슬롯 초기화
    WeaponSlots.SetNum(WeaponSlotCount);
    ItemSlots.SetNum(ItemSlotCount);
}

void UCYInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UCYInventoryComponent, WeaponSlots);
    DOREPLIFETIME(UCYInventoryComponent, ItemSlots);
    DOREPLIFETIME(UCYInventoryComponent, CurrentHeldItem);
    DOREPLIFETIME(UCYInventoryComponent, bIsProcessingUse);
}

bool UCYInventoryComponent::AddItem(ACYItemBase* Item)
{
    if (!Item || !GetOwner()->HasAuthority()) return false;
    
    // 타입별로 처리
    if (Item->ItemType == EItemType::Weapon)
    {
        return AddWeapon(Item);
    }
    else
    {
        return AddItemWithStacking(Item);
    }
}

bool UCYInventoryComponent::AddWeapon(ACYItemBase* Weapon)
{
    int32 EmptySlot = FindEmptyWeaponSlot();
    if (EmptySlot == -1) 
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ No empty weapon slot"));
        return false;
    }
    
    WeaponSlots[EmptySlot] = Weapon;
    OnInventoryChanged.Broadcast(EmptySlot + 1, Weapon); // 1~3번 키
    
    // 첫 무기 자동 장착
    if (EmptySlot == 0)
    {
        if (UCYWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UCYWeaponComponent>())
        {
            WeaponComp->EquipWeapon(Cast<ACYWeaponBase>(Weapon));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Added weapon: %s to slot %d"), 
           *Weapon->ItemName.ToString(), EmptySlot + 1);
    return true;
}

bool UCYInventoryComponent::AddItemWithStacking(ACYItemBase* Item)
{
    // 스택 시도
    if (TryStackWithExistingItem(Item))
    {
        return true;
    }
    
    // 빈 슬롯 찾기
    int32 EmptySlot = FindEmptyItemSlot();
    if (EmptySlot == -1) 
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ No empty item slot"));
        return false;
    }
    
    ItemSlots[EmptySlot] = Item;
    OnInventoryChanged.Broadcast(EmptySlot + 4, Item); // 4~9번 키
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Added item: %s to slot %d"), 
           *Item->ItemName.ToString(), EmptySlot + 4);
    return true;
}

ACYItemBase* UCYInventoryComponent::GetItem(int32 SlotIndex) const
{
    if (IsWeaponSlot(SlotIndex))
    {
        int32 Index = WeaponSlotToIndex(SlotIndex);
        if (Index >= 0 && Index < WeaponSlots.Num())
        {
            return WeaponSlots[Index];
        }
    }
    else if (IsItemSlot(SlotIndex))
    {
        int32 Index = ItemSlotToIndex(SlotIndex);
        if (Index >= 0 && Index < ItemSlots.Num())
        {
            return ItemSlots[Index];
        }
    }
    
    return nullptr;
}

// 🔥 기존 UseItem을 HoldItem으로 변경
bool UCYInventoryComponent::HoldItem(int32 SlotIndex)
{
    if (bIsProcessingUse) return false;
    
    if (!GetOwner()->HasAuthority())
    {
        ServerHoldItem(SlotIndex);
        return false;
    }
    
    bIsProcessingUse = true;
    
    ACYItemBase* Item = GetItem(SlotIndex);
    bool bSuccess = false;
    
    if (IsWeaponSlot(SlotIndex))
    {
        // 🔥 무기 슬롯 처리 (빈 슬롯 포함)
        UCYWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UCYWeaponComponent>();
        if (!WeaponComp)
        {
            bIsProcessingUse = false;
            return false;
        }
        
        // 들고 있던 아이템 먼저 해제
        if (CurrentHeldItem)
        {
            DetachItemFromHand(CurrentHeldItem);
            ACYItemBase* OldHeldItem = CurrentHeldItem;
            CurrentHeldItem = nullptr;
            OnHeldItemChanged.Broadcast(OldHeldItem, nullptr);
            UE_LOG(LogTemp, Warning, TEXT("🤏 Released held item to handle weapon slot"));
        }
        
        if (Item) // 아이템이 있는 경우 - 무기 장착
        {
            bSuccess = WeaponComp->EquipWeapon(Cast<ACYWeaponBase>(Item));
            UE_LOG(LogTemp, Warning, TEXT("⚔️ Equipped weapon: %s"), *Item->ItemName.ToString());
        }
        else // 빈 슬롯인 경우 - 무기 해제
        {
            if (WeaponComp->CurrentWeapon)
            {
                UE_LOG(LogTemp, Warning, TEXT("⚔️ Unequipped weapon: %s"), *WeaponComp->CurrentWeapon->ItemName.ToString());
                WeaponComp->UnequipWeapon();
                bSuccess = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚔️ Selected empty weapon slot %d"), SlotIndex);
                bSuccess = true; // 이미 빈 상태여도 성공으로 처리
            }
        }
    }
    else if (IsItemSlot(SlotIndex))
    {
        // 🔥 아이템 슬롯 처리 (빈 슬롯 포함)
        
        // 장착된 무기 먼저 해제
        UCYWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UCYWeaponComponent>();
        if (WeaponComp && WeaponComp->CurrentWeapon)
        {
            WeaponComp->UnequipWeapon();
            UE_LOG(LogTemp, Warning, TEXT("⚔️ Unequipped weapon to handle item slot"));
        }
        
        // 기존에 들고 있던 아이템 해제
        ACYItemBase* OldHeldItem = CurrentHeldItem;
        if (CurrentHeldItem)
        {
            DetachItemFromHand(CurrentHeldItem);
        }
        
        if (Item) // 아이템이 있는 경우 - 아이템 들기
        {
            CurrentHeldItem = Item;
            AttachItemToHand(CurrentHeldItem);
            UE_LOG(LogTemp, Warning, TEXT("🤏 Now holding: %s"), *CurrentHeldItem->ItemName.ToString());
            bSuccess = true;
        }
        else // 빈 슬롯인 경우 - 아무것도 들지 않기
        {
            CurrentHeldItem = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("🤏 Selected empty item slot %d - holding nothing"), SlotIndex);
            bSuccess = true;
        }
        
        OnHeldItemChanged.Broadcast(OldHeldItem, CurrentHeldItem);
    }
    
    // 다음 프레임에 플래그 해제
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        bIsProcessingUse = false;
    });
    
    return bSuccess;
}

// 🔥 새로 추가: 들고 있는 아이템 사용
bool UCYInventoryComponent::UseHeldItem()
{
    if (!CurrentHeldItem || !GetOwner()->HasAuthority())
    {
        if (!GetOwner()->HasAuthority())
        {
            ServerUseHeldItem();
        }
        return false;
    }
    
    bool bSuccess = CurrentHeldItem->UseItem(Cast<ACYPlayerCharacter>(GetOwner()));
    
    if (bSuccess && CurrentHeldItem->ItemType == EItemType::Trap)
    {
        // 트랩 사용 시 수량 감소
        CurrentHeldItem->ItemCount--;
        
        if (CurrentHeldItem->ItemCount <= 0)
        {
            // 아이템이 모두 소모되면 슬롯에서 제거하고 손에서도 해제
            for (int32 i = 0; i < ItemSlots.Num(); ++i)
            {
                if (ItemSlots[i] == CurrentHeldItem)
                {
                    ItemSlots[i] = nullptr;
                    OnInventoryChanged.Broadcast(i + 4, nullptr);
                    break;
                }
            }
            
            DetachItemFromHand(CurrentHeldItem);
            ACYItemBase* OldHeldItem = CurrentHeldItem;
            CurrentHeldItem = nullptr;
            OnHeldItemChanged.Broadcast(OldHeldItem, nullptr);
            
            OldHeldItem->Destroy();
        }
        else
        {
            // 수량만 감소한 경우 인벤토리 업데이트
            for (int32 i = 0; i < ItemSlots.Num(); ++i)
            {
                if (ItemSlots[i] == CurrentHeldItem)
                {
                    OnInventoryChanged.Broadcast(i + 4, CurrentHeldItem);
                    break;
                }
            }
        }
    }
    
    return bSuccess;
}

void UCYInventoryComponent::ServerHoldItem_Implementation(int32 SlotIndex)
{
    HoldItem(SlotIndex);
}

void UCYInventoryComponent::ServerUseHeldItem_Implementation()
{
    UseHeldItem();
}

// 🔥 새로 추가: 아이템을 hand_r 소켓에 부착
void UCYInventoryComponent::AttachItemToHand(ACYItemBase* Item)
{
    if (!Item || !GetOwner()) return;
    
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->GetMesh()) return;
    
    // 아이템을 hand_r 소켓에 부착
    Item->AttachToComponent(
        Character->GetMesh(),
        FAttachmentTransformRules::SnapToTargetIncludingScale,
        TEXT("hand_r")
    );
    
    // 충돌 비활성화 (들고 있는 동안)
    if (Item->ItemMesh)
    {
        Item->ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (Item->InteractionSphere)
    {
        Item->InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // 보이게 설정
    Item->SetActorHiddenInGame(false);
    
    UE_LOG(LogTemp, Warning, TEXT("🤏 Attached item to hand: %s"), *Item->ItemName.ToString());
}

// 🔥 새로 추가: 아이템을 손에서 해제
void UCYInventoryComponent::DetachItemFromHand(ACYItemBase* Item)
{
    if (!Item) return;
    
    // 부착 해제
    Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // 숨기기 (인벤토리에 있는 아이템은 보이지 않음)
    Item->SetActorHiddenInGame(true);
    
    UE_LOG(LogTemp, Warning, TEXT("🤏 Detached item from hand: %s"), *Item->ItemName.ToString());
}

void UCYInventoryComponent::ShowInventoryDebug()
{
    if (!GEngine) return;
    
    GEngine->ClearOnScreenDebugMessages();
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("=== 📦 INVENTORY ==="));
    
    // 🔥 현재 상태 요약
    UCYWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UCYWeaponComponent>();
    bool bHasWeapon = WeaponComp && WeaponComp->CurrentWeapon;
    bool bHasHeldItem = CurrentHeldItem != nullptr;
    
    FString CurrentStatus;
    if (bHasWeapon)
    {
        CurrentStatus = FString::Printf(TEXT("⚔️ WEAPON: %s"), *WeaponComp->CurrentWeapon->ItemName.ToString());
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, CurrentStatus);
    }
    else if (bHasHeldItem)
    {
        CurrentStatus = FString::Printf(TEXT("🤏 HOLDING: %s x%d"), 
            *CurrentHeldItem->ItemName.ToString(), CurrentHeldItem->ItemCount);
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, CurrentStatus);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("🙌 HANDS EMPTY"));
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, TEXT(""));
    
    // 무기 슬롯 (1~3번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🗡️ WEAPONS (1-3):"));
    for (int32 i = 0; i < WeaponSlots.Num(); ++i)
    {
        FString WeaponInfo;
        FColor WeaponColor = FColor::Green;
        
        if (WeaponSlots[i])
        {
            bool bIsEquipped = (WeaponComp && WeaponComp->CurrentWeapon == WeaponSlots[i]);
            FString EquippedIndicator = bIsEquipped ? TEXT(" [EQUIPPED]") : TEXT("");
            WeaponInfo = FString::Printf(TEXT("  [%d] %s%s"), 
                i + 1, *WeaponSlots[i]->ItemName.ToString(), *EquippedIndicator);
            WeaponColor = bIsEquipped ? FColor::Red : FColor::White;
        }
        else
        {
            bool bIsSelected = bHasWeapon == false && !bHasHeldItem; // 빈 손 상태일 때는 선택된 것으로 간주하지 않음
            WeaponInfo = FString::Printf(TEXT("  [%d] Empty"), i + 1);
            WeaponColor = FColor::Green;
        }
        
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, WeaponColor, WeaponInfo);
    }
    
    // 아이템 슬롯 (4~9번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🎒 ITEMS (4-9):"));
    for (int32 i = 0; i < ItemSlots.Num(); ++i)
    {
        FString ItemInfo;
        FColor ItemColor = FColor::Green;
        
        if (ItemSlots[i])
        {
            bool bIsHeld = (ItemSlots[i] == CurrentHeldItem);
            FString HeldIndicator = bIsHeld ? TEXT(" [HELD]") : TEXT("");
            ItemInfo = FString::Printf(TEXT("  [%d] %s x%d%s"), 
                i + 4, *ItemSlots[i]->ItemName.ToString(), ItemSlots[i]->ItemCount, *HeldIndicator);
            ItemColor = bIsHeld ? FColor::Orange : FColor::White;
        }
        else
        {
            ItemInfo = FString::Printf(TEXT("  [%d] Empty"), i + 4);
            ItemColor = FColor::Green;
        }
        
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, ItemColor, ItemInfo);
    }
    
    // 조작 방법 안내
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT(""));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("📋 Controls:"));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("  1-3: Select Weapon (Empty = Unequip)"));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("  4-9: Hold Item (Empty = Release)"));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("  Left Click: Attack/Use (if equipped/held)"));
}

// 내부 유틸리티 함수들 (기존 그대로)
int32 UCYInventoryComponent::FindEmptyWeaponSlot() const
{
    for (int32 i = 0; i < WeaponSlots.Num(); ++i)
    {
        if (!WeaponSlots[i]) return i;
    }
    return -1;
}

int32 UCYInventoryComponent::FindEmptyItemSlot() const
{
    for (int32 i = 0; i < ItemSlots.Num(); ++i)
    {
        if (!ItemSlots[i]) return i;
    }
    return -1;
}

int32 UCYInventoryComponent::FindStackableItemSlot(ACYItemBase* Item) const
{
    if (!Item) return -1;
    
    for (int32 i = 0; i < ItemSlots.Num(); ++i)
    {
        if (ItemSlots[i] && ItemSlots[i]->CanStackWith(Item))
        {
            return i;
        }
    }
    return -1;
}

bool UCYInventoryComponent::TryStackWithExistingItem(ACYItemBase* Item)
{
    int32 StackableSlot = FindStackableItemSlot(Item);
    if (StackableSlot == -1) return false;

    ACYItemBase* ExistingItem = ItemSlots[StackableSlot];
    int32 AddableCount = FMath::Min(Item->ItemCount, 
                                    ExistingItem->MaxStackCount - ExistingItem->ItemCount);
    
    ExistingItem->ItemCount += AddableCount;
    Item->ItemCount -= AddableCount;
    
    OnInventoryChanged.Broadcast(StackableSlot + 4, ExistingItem);
    
    if (Item->ItemCount <= 0)
    {
        Item->Destroy();
        return true;
    }
    
    return false;
}

void UCYInventoryComponent::OnRep_WeaponSlots()
{
    for (int32 i = 0; i < WeaponSlots.Num(); ++i)
    {
        OnInventoryChanged.Broadcast(i + 1, WeaponSlots[i]);
    }
}

void UCYInventoryComponent::OnRep_ItemSlots()
{
    for (int32 i = 0; i < ItemSlots.Num(); ++i)
    {
        OnInventoryChanged.Broadcast(i + 4, ItemSlots[i]);
    }
}

// 🔥 새로 추가: CurrentHeldItem 네트워크 동기화
void UCYInventoryComponent::OnRep_CurrentHeldItem()
{
    if (CurrentHeldItem)
    {
        AttachItemToHand(CurrentHeldItem);
        UE_LOG(LogTemp, Log, TEXT("Client: Now holding %s"), *CurrentHeldItem->ItemName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Client: No longer holding any item"));
    }
}