// CYInventoryComponent.cpp - 핵심 로직만 남긴 인벤토리 컴포넌트 구현
#include "Components/Items/CYInventoryComponent.h"
#include "Items/CYItemBase.h"
#include "Items/CYWeaponBase.h"
#include "Items/Traps/CYTrapBase.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/CYPlayerCharacter.h"
#include "Engine/Engine.h"

UCYInventoryComponent::UCYInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    bIsProcessingUse = false;
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

bool UCYInventoryComponent::UseItem(int32 SlotIndex)
{
    if (bIsProcessingUse) return false;
    
    if (!GetOwner()->HasAuthority())
    {
        ServerUseItem(SlotIndex);
        return false;
    }
    
    bIsProcessingUse = true;
    
    ACYItemBase* Item = GetItem(SlotIndex);
    if (!Item)
    {
        bIsProcessingUse = false;
        return false;
    }
    
    bool bSuccess = false;
    
    if (IsWeaponSlot(SlotIndex))
    {
        // 무기 장착
        if (UCYWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UCYWeaponComponent>())
        {
            bSuccess = WeaponComp->EquipWeapon(Cast<ACYWeaponBase>(Item));
        }
    }
    else if (IsItemSlot(SlotIndex))
    {
        // 아이템 사용 (트랩 설치 등)
        if (Item->ItemType == EItemType::Trap)
        {
            // 트랩 설치는 GA_PlaceTrap 어빌리티가 처리
            bSuccess = Item->UseItem(Cast<ACYPlayerCharacter>(GetOwner()));
            
            // 트랩 사용 시 수량 감소
            if (bSuccess)
            {
                Item->ItemCount--;
                if (Item->ItemCount <= 0)
                {
                    int32 Index = ItemSlotToIndex(SlotIndex);
                    ItemSlots[Index] = nullptr;
                    OnInventoryChanged.Broadcast(SlotIndex, nullptr);
                    Item->Destroy();
                }
                else
                {
                    OnInventoryChanged.Broadcast(SlotIndex, Item);
                }
            }
        }
        else
        {
            bSuccess = Item->UseItem(Cast<ACYPlayerCharacter>(GetOwner()));
        }
    }
    
    // 다음 프레임에 플래그 해제
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        bIsProcessingUse = false;
    });
    
    return bSuccess;
}

void UCYInventoryComponent::ServerUseItem_Implementation(int32 SlotIndex)
{
    UseItem(SlotIndex);
}

void UCYInventoryComponent::ShowInventoryDebug()
{
    if (!GEngine) return;
    
    GEngine->ClearOnScreenDebugMessages();
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("=== 📦 INVENTORY ==="));
    
    // 무기 슬롯 (1~3번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🗡️ WEAPONS (1-3):"));
    for (int32 i = 0; i < WeaponSlots.Num(); ++i)
    {
        FString WeaponInfo;
        if (WeaponSlots[i])
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] %s"), 
                i + 1, *WeaponSlots[i]->ItemName.ToString());
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, WeaponInfo);
        }
        else
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] Empty"), i + 1);
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, WeaponInfo);
        }
    }
    
    // 아이템 슬롯 (4~9번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🎒 ITEMS (4-9):"));
    for (int32 i = 0; i < ItemSlots.Num(); ++i)
    {
        FString ItemInfo;
        if (ItemSlots[i])
        {
            ItemInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 4, *ItemSlots[i]->ItemName.ToString(), ItemSlots[i]->ItemCount);
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, ItemInfo);
        }
        else
        {
            ItemInfo = FString::Printf(TEXT("  [%d] Empty"), i + 4);
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, ItemInfo);
        }
    }
}

// 내부 유틸리티 함수들
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