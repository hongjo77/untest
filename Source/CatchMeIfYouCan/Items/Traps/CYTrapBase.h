// CYTrapBase.h - 강화된 백업 로직 헤더

#pragma once

#include "CoreMinimal.h"
#include "Items/CYItemBase.h"
#include "Items/CYTrapData.h"
#include "CYTrapBase.generated.h"

class ACYPlayerCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ETrapState : uint8
{
    MapPlaced       UMETA(DisplayName = "Map Placed (Pickupable)"),
    PlayerPlaced    UMETA(DisplayName = "Player Placed (Active)")
};

UCLASS(Abstract, BlueprintType)
class CATCHMEIFYOUCAN_API ACYTrapBase : public ACYItemBase
{
    GENERATED_BODY()

public:
    ACYTrapBase();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap State", Replicated)
    ETrapState TrapState = ETrapState::MapPlaced;

    UPROPERTY(BlueprintReadOnly, Category = "Trap State", Replicated)
    bool bIsArmed = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap Data", Replicated)
    FTrapData TrapData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap", Replicated)
    ETrapType TrapType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap Settings")
    float TriggerRadius = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap Settings")
    float ArmingDelay = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap Settings")
    float TrapLifetime = 60.0f;

    UFUNCTION(BlueprintCallable, Category = "Trap")
    void ConvertToPlayerPlacedTrap(AActor* PlacingPlayer);

    // 이벤트 함수들 (하위 클래스에서 오버라이드)
    UFUNCTION(BlueprintNativeEvent, Category = "Trap Events")
    void OnTrapSpawned();
    virtual void OnTrapSpawned_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Trap Events")
    void OnTrapArmed();
    virtual void OnTrapArmed_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Trap Events")
    void OnTrapTriggered(ACYPlayerCharacter* Target);
    virtual void OnTrapTriggered_Implementation(ACYPlayerCharacter* Target);

    UFUNCTION(BlueprintNativeEvent, Category = "Trap Events")
    void OnTrapDestroyed();
    virtual void OnTrapDestroyed_Implementation();

    // 시각적/오디오 설정 (하위 클래스에서 오버라이드)
    UFUNCTION(BlueprintNativeEvent, Category = "Trap Visuals")
    void SetupTrapVisuals();
    virtual void SetupTrapVisuals_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Trap Audio")
    void PlayTrapSound();
    virtual void PlayTrapSound_Implementation();

    // 커스텀 효과 (하위 클래스에서 오버라이드)
    UFUNCTION(BlueprintNativeEvent, Category = "Trap Effects")
    void ApplyCustomEffects(ACYPlayerCharacter* Target);
    virtual void ApplyCustomEffects_Implementation(ACYPlayerCharacter* Target);

    // 멀티캐스트 함수들
    UFUNCTION(NetMulticast, Reliable, Category = "Trap Events")
    void MulticastOnTrapTriggered(ACYPlayerCharacter* Target);

    UFUNCTION(NetMulticast, Reliable, Category = "Trap Visuals")
    void MulticastUpdateTrapVisuals();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 핵심 트랩 로직
    void InitializeTrapVisuals();
    void SetupTrapForCurrentState();
    void SetupTrapTimers();
    void ArmTrap();

    // 오버랩 이벤트들
    UFUNCTION()
    void OnTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnPickupSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // ✅ 트랩 효과 적용 (강화된 백업 로직 포함)
    UFUNCTION(BlueprintCallable, Category = "Trap")
    void ApplyTrapEffects(ACYPlayerCharacter* Target);

    // ✅ 새로운 백업 함수들
    UFUNCTION(BlueprintCallable, Category = "Trap")
    void VerifyEffectApplication(ACYPlayerCharacter* Target, UAbilitySystemComponent* TargetASC);

    UFUNCTION(BlueprintCallable, Category = "Trap")
    void ApplyDirectMovementControl(ACYPlayerCharacter* Target);

    FActiveGameplayEffectHandle ApplySingleEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass);

private:
    FTimerHandle ArmingTimer;
    FTimerHandle LifetimeTimer;
};