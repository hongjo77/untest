// CYInventoryComponent.h - HeldItem 시스템 추가
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CYInventoryComponent.generated.h"

class ACYItemBase;
class ACYWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, int32, SlotIndex, ACYItemBase*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeldItemChanged, ACYItemBase*, OldItem, ACYItemBase*, NewItem);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CATCHMEIFYOUCAN_API UCYInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCYInventoryComponent();

    // 슬롯 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 WeaponSlotCount = 3;  // 1~3번 키

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 ItemSlotCount = 6;    // 4~9번 키

    // 슬롯 배열 (네트워크 동기화)
    UPROPERTY(ReplicatedUsing = OnRep_WeaponSlots, BlueprintReadOnly, Category = "Inventory")
    TArray<ACYItemBase*> WeaponSlots;

    UPROPERTY(ReplicatedUsing = OnRep_ItemSlots, BlueprintReadOnly, Category = "Inventory")
    TArray<ACYItemBase*> ItemSlots;

    // 🔥 새로 추가: 현재 들고 있는 아이템 (무기가 아닌)
    UPROPERTY(ReplicatedUsing = OnRep_CurrentHeldItem, BlueprintReadOnly, Category = "Inventory")
    ACYItemBase* CurrentHeldItem;

    // 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHeldItemChanged OnHeldItemChanged;

    // 공개 인터페이스
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(ACYItemBase* Item);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    ACYItemBase* GetItem(int32 SlotIndex) const;

    // 🔥 기존 UseItem을 HoldItem으로 변경
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool HoldItem(int32 SlotIndex);

    // 🔥 새로 추가: 들고 있는 아이템 사용
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseHeldItem();

    UFUNCTION(Server, Reliable, Category = "Inventory")
    void ServerHoldItem(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory")
    void ServerUseHeldItem();

    // 디버그 함수
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ShowInventoryDebug();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 네트워크 동기화
    UFUNCTION()
    void OnRep_WeaponSlots();

    UFUNCTION()
    void OnRep_ItemSlots();

    UFUNCTION()
    void OnRep_CurrentHeldItem();

    // 내부 로직
    bool AddWeapon(ACYItemBase* Weapon);
    bool AddItemWithStacking(ACYItemBase* Item);
    
    int32 FindEmptyWeaponSlot() const;
    int32 FindEmptyItemSlot() const;
    int32 FindStackableItemSlot(ACYItemBase* Item) const;
    bool TryStackWithExistingItem(ACYItemBase* Item);

    // 🔥 새로 추가: 아이템 부착/해제 로직
    void AttachItemToHand(ACYItemBase* Item);
    void DetachItemFromHand(ACYItemBase* Item);

    // 슬롯 인덱스 변환
    bool IsWeaponSlot(int32 SlotIndex) const { return SlotIndex >= 1 && SlotIndex <= 3; }
    bool IsItemSlot(int32 SlotIndex) const { return SlotIndex >= 4 && SlotIndex <= 9; }
    int32 WeaponSlotToIndex(int32 SlotIndex) const { return SlotIndex - 1; }
    int32 ItemSlotToIndex(int32 SlotIndex) const { return SlotIndex - 4; }

private:
    // 중복 실행 방지
    UPROPERTY(Replicated)
    bool bIsProcessingUse = false;
};