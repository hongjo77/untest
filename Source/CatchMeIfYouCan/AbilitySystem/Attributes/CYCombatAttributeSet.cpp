﻿// CYCombatAttributeSet.cpp - 속도 동기화 문제 해결
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
    // 🔥 기본 속도 400으로 설정
    InitMoveSpeed(400.0f);
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
        // 🔥 최소값을 0으로 유지 (음수 방지)
        NewValue = FMath::Max(NewValue, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("PreAttributeChange MoveSpeed: %f -> %f"), 
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
        // 🔥 서버와 클라이언트 모두에서 처리 (원래대로)
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
    
    UE_LOG(LogTemp, Warning, TEXT("HandleMoveSpeedChange: %f"), NewMoveSpeed);
    
    // 🔥 Character 찾기 로직 (원래대로)
    ACharacter* TargetCharacter = nullptr;
    
    // 직접 Character인 경우
    TargetCharacter = Cast<ACharacter>(GetOwningActor());
    
    // PlayerState가 Owner인 경우
    if (!TargetCharacter)
    {
        if (APlayerState* PS = Cast<APlayerState>(GetOwningActor()))
        {
            TargetCharacter = Cast<ACharacter>(PS->GetPawn());
        }
    }
    
    // Instigator를 통해 찾기
    if (!TargetCharacter)
    {
        if (APawn* Pawn = GetOwningActor() ? GetOwningActor()->GetInstigator() : nullptr)
        {
            TargetCharacter = Cast<ACharacter>(Pawn);
        }
    }
    
    // Character를 찾았으면 이동 속도 적용
    if (TargetCharacter)
    {
        ApplyMovementRestrictions(TargetCharacter, NewMoveSpeed);
        
        if (TargetCharacter->HasAuthority())
        {
            TargetCharacter->ForceNetUpdate();
        }
    }
}

void UCYCombatAttributeSet::ApplyMovementRestrictions(ACharacter* Character, float Speed)
{
    if (!Character) return;
    
    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
    if (!MovementComp) return;
    
    // 🔥 원래대로: 즉시 설정
    MovementComp->MaxWalkSpeed = Speed;
    
    if (Speed <= 0.0f)
    {
        // 완전 정지
        MovementComp->StopMovementImmediately();
        MovementComp->MaxAcceleration = 0.0f;
        MovementComp->BrakingDecelerationWalking = 10000.0f;
        MovementComp->GroundFriction = 100.0f;
        MovementComp->JumpZVelocity = 0.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("IMMOBILIZED: %s"), *Character->GetName());
    }
    else if (Speed < 200.0f)
    {
        // 느림 상태
        MovementComp->MaxAcceleration = 500.0f;
        MovementComp->BrakingDecelerationWalking = 1000.0f;
        MovementComp->JumpZVelocity = 0.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("SLOWED: %s to %f"), *Character->GetName(), Speed);
    }
    else
    {
        // 정상 복구
        MovementComp->MaxAcceleration = 2048.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        MovementComp->GroundFriction = 8.0f;
        MovementComp->JumpZVelocity = 600.0f;
        
        UE_LOG(LogTemp, Warning, TEXT("MOVEMENT RESTORED: %s"), *Character->GetName());
    }
}

// OnRep 함수들
void UCYCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	if (GetOwningAbilitySystemComponent())
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, Health, OldHealth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRep_Health: No valid AbilitySystemComponent"));
	}
}

void UCYCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	if (GetOwningAbilitySystemComponent())
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, MaxHealth, OldMaxHealth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRep_MaxHealth: No valid AbilitySystemComponent"));
	}
}

void UCYCombatAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	if (GetOwningAbilitySystemComponent())
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, MoveSpeed, OldMoveSpeed);
		
		UE_LOG(LogTemp, Warning, TEXT("OnRep_MoveSpeed: %f -> %f"), 
			   OldMoveSpeed.GetCurrentValue(), GetMoveSpeed());
		
		// 🔥 클라이언트에서도 즉시 이동 속도 적용 (원래대로)
		HandleMoveSpeedChange();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRep_MoveSpeed: No valid AbilitySystemComponent"));
	}
}

void UCYCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	if (GetOwningAbilitySystemComponent())
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, AttackPower, OldAttackPower);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRep_AttackPower: No valid AbilitySystemComponent"));
	}
}