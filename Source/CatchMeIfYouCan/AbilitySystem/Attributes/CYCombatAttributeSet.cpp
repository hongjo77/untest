// CYCombatAttributeSet.cpp - 트랩 효과 적용 보장

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
        
        // ✅ PreAttributeChange에서 로그 추가
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
		// ✅ 서버에서 MoveSpeed 변경 처리
		float OldValue = Data.EvaluatedData.Attribute.GetNumericValue(this);
		float NewValue = GetMoveSpeed();
        
		UE_LOG(LogTemp, Warning, TEXT("🏃 [SERVER] PostGameplayEffectExecute MoveSpeed changed: %f -> %f"), 
			   OldValue, NewValue);
        
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
    
	UE_LOG(LogTemp, Warning, TEXT("🏃 HandleMoveSpeedChange called with speed: %f"), NewMoveSpeed);
    
	ACharacter* Character = GetOwningCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ HandleMoveSpeedChange: No Character found"));
		LogOwnershipChain();
		return;
	}

	// ✅ 핵심: MovementComponent에 직접 적용
	ApplyMoveSpeedToMovementComponent(NewMoveSpeed);
}

// ✅ 강화된 Character 찾기 로직
ACharacter* UCYCombatAttributeSet::GetOwningCharacter() const
{
    AActor* OwningActor = GetOwningActor();
    
    UE_LOG(LogTemp, Verbose, TEXT("🔍 GetOwningCharacter: OwningActor = %s"), 
           OwningActor ? *OwningActor->GetName() : TEXT("NULL"));
    
    // 1. 직접 Character인지 확인
    if (ACharacter* Character = Cast<ACharacter>(OwningActor))
    {
        UE_LOG(LogTemp, Verbose, TEXT("🔍 Found Character directly: %s"), *Character->GetName());
        return Character;
    }
    
    // 2. PlayerState를 통해 찾기
    if (APlayerState* PlayerState = Cast<APlayerState>(OwningActor))
    {
        APawn* Pawn = PlayerState->GetPawn();
        if (ACharacter* Character = Cast<ACharacter>(Pawn))
        {
            UE_LOG(LogTemp, Verbose, TEXT("🔍 Found Character via PlayerState: %s"), *Character->GetName());
            return Character;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PlayerState has no valid Character Pawn"));
        }
    }
    
    // 3. AbilitySystemComponent을 통해 찾기 (추가 시도)
    if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
    {
        if (AActor* AvatarActor = ASC->GetAvatarActor())
        {
            if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
            {
                UE_LOG(LogTemp, Verbose, TEXT("🔍 Found Character via ASC Avatar: %s"), *Character->GetName());
                return Character;
            }
        }
    }
    
    return nullptr;
}

// ✅ 디버깅용 소유권 체인 로그
void UCYCombatAttributeSet::LogOwnershipChain() const
{
    UE_LOG(LogTemp, Warning, TEXT("🔍 === Ownership Chain Debug ==="));
    
    AActor* OwningActor = GetOwningActor();
    UE_LOG(LogTemp, Warning, TEXT("🔍 OwningActor: %s (Class: %s)"), 
           OwningActor ? *OwningActor->GetName() : TEXT("NULL"),
           OwningActor ? *OwningActor->GetClass()->GetName() : TEXT("NULL"));
    
    if (APlayerState* PlayerState = Cast<APlayerState>(OwningActor))
    {
        APawn* Pawn = PlayerState->GetPawn();
        UE_LOG(LogTemp, Warning, TEXT("🔍 PlayerState->GetPawn(): %s"), 
               Pawn ? *Pawn->GetName() : TEXT("NULL"));
    }
    
    if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
    {
        AActor* AvatarActor = ASC->GetAvatarActor();
        UE_LOG(LogTemp, Warning, TEXT("🔍 ASC->GetAvatarActor(): %s"), 
               AvatarActor ? *AvatarActor->GetName() : TEXT("NULL"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔍 === End Debug ==="));
}

// ✅ 리플리케이션 핸들러들
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
    
	float NewMoveSpeed = GetMoveSpeed();
	UE_LOG(LogTemp, Warning, TEXT("🏃 [CLIENT] OnRep_MoveSpeed: %f -> %f"), 
		   OldMoveSpeed.GetCurrentValue(), NewMoveSpeed);
    
	// ✅ 클라이언트에서도 동일하게 처리
	HandleMoveSpeedChange();
}

void UCYCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UCYCombatAttributeSet, AttackPower, OldAttackPower);
}

void UCYCombatAttributeSet::ApplyMoveSpeedToMovementComponent(float NewMoveSpeed)
{
	ACharacter* Character = GetOwningCharacter();
	if (!Character) return;

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp) return;
    
	UE_LOG(LogTemp, Warning, TEXT("🏃 Applying speed %f to %s"), NewMoveSpeed, *Character->GetName());
    
	// ✅ 직접 적용
	MovementComp->MaxWalkSpeed = NewMoveSpeed;
    
	if (NewMoveSpeed <= 0.0f)
	{
		// 완전 정지
		MovementComp->StopMovementImmediately();
		MovementComp->MaxAcceleration = 0.0f;
		MovementComp->JumpZVelocity = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("❄️ Complete freeze applied"));
	}
	else if (NewMoveSpeed < 100.0f)
	{
		// 슬로우
		MovementComp->MaxAcceleration = 200.0f;
		MovementComp->JumpZVelocity = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("🧊 Slow effect applied"));
	}
	else
	{
		// 정상 속도
		MovementComp->MaxAcceleration = 2048.0f;
		MovementComp->JumpZVelocity = 600.0f;
		UE_LOG(LogTemp, Warning, TEXT("🏃 Normal speed applied"));
	}
    
	// ✅ 강제 업데이트
	MovementComp->UpdateComponentVelocity();
    
	UE_LOG(LogTemp, Warning, TEXT("✅ MoveSpeed successfully applied: %f"), MovementComp->MaxWalkSpeed);
}