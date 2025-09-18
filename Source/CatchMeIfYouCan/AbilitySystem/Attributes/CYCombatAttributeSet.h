#pragma once

#include "CoreMinimal.h"
#include "CYAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CYCombatAttributeSet.generated.h"

class UCharacterMovementComponent;

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
    void ApplyMovementRestrictions(UCharacterMovementComponent* MovementComp, float Speed);
};