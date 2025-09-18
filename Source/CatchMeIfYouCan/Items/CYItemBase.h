// CYItemBase.h - 핵심 로직만 남긴 기본 아이템 클래스
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CYItemBase.generated.h"

class ACYPlayerCharacter;
class UStaticMeshComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Base,
    Weapon,
    Trap,
    Consumable
};

UCLASS(Abstract)
class CATCHMEIFYOUCAN_API ACYItemBase : public AActor
{
    GENERATED_BODY()

public:
    ACYItemBase();

    // 기본 아이템 정보
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText ItemName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EItemType ItemType = EItemType::Base;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 MaxStackCount = 10;

    // 현재 수량 (네트워크 동기화)
    UPROPERTY(ReplicatedUsing = OnRep_ItemCount, BlueprintReadOnly, Category = "Item")
    int32 ItemCount = 1;

    // 픽업 상태 (네트워크 동기화)
    UPROPERTY(ReplicatedUsing = OnRep_IsPickedUp, BlueprintReadOnly, Category = "Item")
    bool bIsPickedUp = false;

    // 컴포넌트들
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ItemMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractionSphere;

    // 픽업/사용 함수들
    UFUNCTION(BlueprintCallable, Category = "Item")
    virtual void OnPickup(ACYPlayerCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Item")
    virtual bool UseItem(ACYPlayerCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Item")
    bool CanStackWith(ACYItemBase* OtherItem) const;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 네트워크 동기화 함수들
    UFUNCTION()
    void OnRep_ItemCount();

    UFUNCTION()
    void OnRep_IsPickedUp();

    // 충돌 이벤트
    UFUNCTION()
    void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};