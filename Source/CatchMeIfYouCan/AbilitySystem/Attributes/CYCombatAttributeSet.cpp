#include "CYCombatAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCYCombatAttributeSet::UCYCombatAttributeSet()
{
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitMoveSpeed(600.0f);
    InitAttackPower(50.0f);
}

void UCYCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UCYCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCYCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCYCombatAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCYCombatAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
}

void UCYCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMoveSpeedAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

void UCYCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        HandleHealthChange();
    }
    else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
    {
        HandleMoveSpeedChange();
    }
}

void UCYCombatAttributeSet::HandleHealthChange()
{
    float NewHealth = GetHealth();
    SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));

    if (GetHealth() <= 0.0f)
    {
        if (AActor* Owner = GetOwningActor())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s has died"), *Owner->GetName());
        }
    }
}

void UCYCombatAttributeSet::HandleMoveSpeedChange()
{
    float NewMoveSpeed = GetMoveSpeed();
    
    if (ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            ApplyMovementRestrictions(MovementComp, NewMoveSpeed);
            
            if (Character->HasAuthority())
            {
                Character->ForceNetUpdate();
            }
        }
    }
}

void UCYCombatAttributeSet::ApplyMovementRestrictions(UCharacterMovementComponent* MovementComp, float Speed)
{
    MovementComp->MaxWalkSpeed = Speed;
    
    if (Speed <= 0.0f)
    {
        MovementComp->StopMovementImmediately();
        MovementComp->MaxAcceleration = 0.0f;
        MovementComp->BrakingDecelerationWalking = 10000.0f;
        MovementComp->GroundFriction = 100.0f;
        MovementComp->JumpZVelocity = 0.0f;
    }
    else if (Speed < 200.0f)
    {
        MovementComp->MaxAcceleration = 500.0f;
        MovementComp->BrakingDecelerationWalking = 1000.0f;
        MovementComp->JumpZVelocity = 0.0f;
    }
    else
    {
        MovementComp->MaxAcceleration = 2048.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        MovementComp->GroundFriction = 8.0f;
        MovementComp->JumpZVelocity = 600.0f;
    }
}

void UCYCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, Health, OldHealth);
}

void UCYCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void UCYCombatAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, MoveSpeed, OldMoveSpeed);
    
    if (ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            ApplyMovementRestrictions(MovementComp, GetMoveSpeed());
        }
    }
}

void UCYCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, AttackPower, OldAttackPower);
}