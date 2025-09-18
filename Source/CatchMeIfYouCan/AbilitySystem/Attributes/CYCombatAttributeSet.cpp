// CYCombatAttributeSet.cpp - Character 변수 충돌 해결

#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

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
        UE_LOG(LogTemp, Warning, TEXT("🏃 PreAttributeChange MoveSpeed: %f -> %f"), 
               GetMoveSpeed(), NewValue);
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
    
    UE_LOG(LogTemp, Warning, TEXT("🏃 HandleMoveSpeedChange: %f"), NewMoveSpeed);
    
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwningActor()))
    {
        if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
        {
            ApplyMovementRestrictions(MovementComp, NewMoveSpeed);
            
            // 서버에서 변경된 경우 네트워크 업데이트
            if (OwnerCharacter->HasAuthority())
            {
                OwnerCharacter->ForceNetUpdate();
            }
        }
    }
    else
    {
        // PlayerState를 통한 접근 시도
        if (APlayerState* PlayerState = Cast<APlayerState>(GetOwningActor()))
        {
            if (APawn* OwnerPawn = PlayerState->GetPawn())
            {
                if (ACharacter* PawnCharacter = Cast<ACharacter>(OwnerPawn))
                {
                    if (UCharacterMovementComponent* MovementComp = PawnCharacter->GetCharacterMovement())
                    {
                        ApplyMovementRestrictions(MovementComp, NewMoveSpeed);
                        
                        if (PawnCharacter->HasAuthority())
                        {
                            PawnCharacter->ForceNetUpdate();
                        }
                    }
                }
            }
        }
    }
}

void UCYCombatAttributeSet::ApplyMovementRestrictions(UCharacterMovementComponent* MovementComp, float Speed)
{
    if (!MovementComp) return;
    
    MovementComp->MaxWalkSpeed = Speed;
    
    if (Speed <= 0.0f)
    {
        // 완전 정지
        MovementComp->StopMovementImmediately();
        MovementComp->MaxAcceleration = 0.0f;
        MovementComp->BrakingDecelerationWalking = 10000.0f;
        MovementComp->GroundFriction = 100.0f;
        MovementComp->JumpZVelocity = 0.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("❄️ IMMOBILIZED: %s"), *GetOwningActor()->GetName());
    }
    else if (Speed < 200.0f)
    {
        // 느림 상태
        MovementComp->MaxAcceleration = 500.0f;
        MovementComp->BrakingDecelerationWalking = 1000.0f;
        MovementComp->JumpZVelocity = 0.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("🧊 SLOWED: %s to %f"), *GetOwningActor()->GetName(), Speed);
    }
    else
    {
        // 정상 복구
        MovementComp->MaxAcceleration = 2048.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        MovementComp->GroundFriction = 8.0f;
        MovementComp->JumpZVelocity = 600.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("🏃 MOVEMENT RESTORED: %s"), *GetOwningActor()->GetName());
    }
    
    MovementComp->UpdateComponentVelocity();
}

// OnRep 함수들
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
    
    UE_LOG(LogTemp, Warning, TEXT("🏃 [CLIENT] OnRep_MoveSpeed: %f -> %f"), 
           OldMoveSpeed.GetCurrentValue(), GetMoveSpeed());
    
    HandleMoveSpeedChange();
}

void UCYCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, AttackPower, OldAttackPower);
}