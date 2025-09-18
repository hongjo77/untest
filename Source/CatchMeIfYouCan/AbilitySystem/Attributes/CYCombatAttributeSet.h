// CYCombatAttributeSet.h - 강제 동기화 로직 제거, 단순화

#pragma once

#include "CoreMinimal.h"
#include "CYAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CYCombatAttributeSet.generated.h"

class UCharacterMovementComponent;
class ACharacter;

UCLASS()
class CATCHMEIFYOUCAN_API UCYCombatAttributeSet : public UCYAttributeSet
{
    GENERATED_BODY()

public:
    UCYCombatAttributeSet();

    // Health (VitalSet과 중복이지만 Combat 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Combat", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UCYCombatAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, Category = "Combat", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UCYCombatAttributeSet, MaxHealth)

    // Movement
    UPROPERTY(BlueprintReadOnly, Category = "Combat", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UCYCombatAttributeSet, MoveSpeed)

    // Damage
    UPROPERTY(BlueprintReadOnly, Category = "Combat", ReplicatedUsing = OnRep_AttackPower)
    FGameplayAttributeData AttackPower;
    ATTRIBUTE_ACCESSORS(UCYCombatAttributeSet, AttackPower)

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	// ✅ 단순화된 MoveSpeed 처리
	void HandleMoveSpeedChange();

protected:
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
    UFUNCTION()
    virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
    UFUNCTION()
    virtual void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);

private:
    void HandleHealthChange();
    
    // ✅ 강화된 Character 찾기
    ACharacter* GetOwningCharacter() const;
    
    // ✅ 디버깅용 함수 추가
    void LogOwnershipChain() const;
};