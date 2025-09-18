// CYInventoryComponent.h - 핵심 로직만 남긴 인벤토리 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CYInventoryComponent.generated.h"

class ACYItemBase;
class ACYWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, int32, SlotIndex, ACYItemBase*, Item);

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

    // 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInventoryChanged OnInventoryChanged;

    // 공개 인터페이스
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(ACYItemBase* Item);

    // UFUNCTION(BlueprintCallable, Category = "Inventory")
    // bool RemoveItem(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    ACYItemBase* GetItem(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItem(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory")
    void ServerUseItem(int32 SlotIndex);

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

    // 내부 로직
    bool AddWeapon(ACYItemBase* Weapon);
    bool AddItemWithStacking(ACYItemBase* Item);
    
    int32 FindEmptyWeaponSlot() const;
    int32 FindEmptyItemSlot() const;
    int32 FindStackableItemSlot(ACYItemBase* Item) const;
    bool TryStackWithExistingItem(ACYItemBase* Item);

    // 슬롯 인덱스 변환
    bool IsWeaponSlot(int32 SlotIndex) const { return SlotIndex >= 1 && SlotIndex <= 3; }
    bool IsItemSlot(int32 SlotIndex) const { return SlotIndex >= 4 && SlotIndex <= 9; }
    int32 WeaponSlotToIndex(int32 SlotIndex) const { return SlotIndex - 1; } // 1->0, 2->1, 3->2
    int32 ItemSlotToIndex(int32 SlotIndex) const { return SlotIndex - 4; }   // 4->0, 5->1, 6->2...

private:
    // 중복 실행 방지
    UPROPERTY(Replicated)
    bool bIsProcessingUse = false;
};