// CYWeaponComponent.h - CatchMe 방식으로 실제 공격 로직 포함
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CYWeaponComponent.generated.h"

class ACYWeaponBase;
class UCYAbilitySystemComponent;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, ACYWeaponBase*, OldWeapon, ACYWeaponBase*, NewWeapon);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CATCHMEIFYOUCAN_API UCYWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCYWeaponComponent();

	// 현재 장착된 무기 (네트워크 동기화)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Weapon")
	ACYWeaponBase* CurrentWeapon;

	// 무기 장착 소켓
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSocketName = TEXT("hand_r");

	// 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponChanged OnWeaponChanged;

	// 핵심 기능
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeapon(ACYWeaponBase* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool UnequipWeapon();

	// 🔥 CatchMe 방식: 실제 공격 로직 포함
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool PerformAttack();

	// 라인 트레이스 (공격 범위 확인)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool PerformLineTrace(FHitResult& OutHit, float Range = 1000.0f);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentWeapon();

	// 🔥 CatchMe 방식: 실제 공격 실행 함수
	bool ExecuteWeaponAttack();

	// 헬퍼 함수들
	UCYAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	USkeletalMeshComponent* GetOwnerMesh() const;
	void AttachWeaponToOwner(ACYWeaponBase* Weapon);
};