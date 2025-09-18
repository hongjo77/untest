// CYTrapBase.cpp - 개선된 시각적 설정 시스템
#include "CYTrapBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "Character/CYPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/CYTrapFactory.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"

ACYTrapBase::ACYTrapBase()
{
    ItemName = FText::FromString("Base Trap");
    ItemDescription = FText::FromString("A base trap class");
	ItemTag = CYGameplayTags::Item_Trap;

    MaxStackCount = 5;
    ItemCount = 1;
    TrapType = ETrapType::Slow;
    TrapState = ETrapState::MapPlaced;

    // ✅ 네트워킹 설정 강화
    bReplicates = true;
    SetReplicateMovement(true);
    bAlwaysRelevant = true;

    // ✅ 기본 메시 설정을 하위 클래스에 위임
    if (ItemMesh)
    {
        // 기본 설정만 해두고, 구체적인 메쉬는 하위 클래스에서 설정
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ItemMesh->SetVisibility(true);
        ItemMesh->SetHiddenInGame(false);
        ItemMesh->SetCastShadow(true);
        
        // ✅ TrapBase에서는 기본 원통 메쉬 설정 (하위 클래스에서 오버라이드)
        static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseTrapMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        if (BaseTrapMesh.Succeeded())
        {
            ItemMesh->SetStaticMesh(BaseTrapMesh.Object);
            ItemMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.1f));
            UE_LOG(LogTemp, Warning, TEXT("🎨 TrapBase: Set default cylinder mesh in constructor"));
        }
    }

    // 기본 트랩 데이터 초기화 (하위 클래스에서 덮어씀)
    TrapData.TrapType = TrapType;
    TrapData.TrapName = ItemName;
    TrapData.TrapDescription = ItemDescription;
    TrapData.TriggerRadius = TriggerRadius;
    TrapData.ArmingDelay = ArmingDelay;
    TrapData.TrapLifetime = TrapLifetime;
}

void ACYTrapBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACYTrapBase, TrapState);
    DOREPLIFETIME(ACYTrapBase, bIsArmed);
    DOREPLIFETIME(ACYTrapBase, TrapType);
    DOREPLIFETIME(ACYTrapBase, TrapData);
}

void ACYTrapBase::BeginPlay()
{
    Super::BeginPlay();

    // ✅ 초기 시각적 설정 - 서버와 클라이언트 모두에서
    InitializeTrapVisuals();
    
    // ✅ 상태별 트랩 설정
    SetupTrapForCurrentState();
    
    // ✅ 서버에서만 로직 처리
    if (HasAuthority())
    {
        OnTrapSpawned();
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯 Trap BeginPlay: %s (State: %s, Authority: %s)"), 
           *ItemName.ToString(), 
           TrapState == ETrapState::MapPlaced ? TEXT("MapPlaced") : TEXT("PlayerPlaced"),
           HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void ACYTrapBase::InitializeTrapVisuals()
{
    UE_LOG(LogTemp, Warning, TEXT("🎨 InitializeTrapVisuals called for %s"), *GetClass()->GetName());
    
    // ✅ 하위 클래스의 SetupTrapVisuals 호출
    SetupTrapVisuals();
    
    // ✅ 추가 초기화 (필요시)
    if (ItemMesh)
    {
        // 기본 트랜스폼 보정
        FVector CurrentLocation = ItemMesh->GetRelativeLocation();
        if (CurrentLocation.Z < -10.0f || CurrentLocation.Z > 10.0f)
        {
            ItemMesh->SetRelativeLocation(FVector(0, 0, 0));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🎨 Trap visuals initialized: %s"), 
               ItemMesh->GetStaticMesh() ? *ItemMesh->GetStaticMesh()->GetName() : TEXT("No Mesh"));
    }
}

void ACYTrapBase::SetupTrapForCurrentState()
{
    if (!InteractionSphere) return;

    if (TrapState == ETrapState::MapPlaced)
    {
        // ✅ 맵 배치 상태: 픽업 가능하도록 설정
        InteractionSphere->SetSphereRadius(150.0f);
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
        
        // ✅ 서버에서만 픽업 바인딩
        if (HasAuthority())
        {
            InteractionSphere->OnComponentBeginOverlap.Clear();
            InteractionSphere->OnComponentEndOverlap.Clear();
            InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACYTrapBase::OnPickupSphereOverlap);
            InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ACYTrapBase::OnPickupSphereEndOverlap);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🎯 Trap set as PICKUPABLE: %s"), *ItemName.ToString());
    }
    else if (TrapState == ETrapState::PlayerPlaced)
    {
        // ✅ 플레이어 배치 상태: 트리거 모드
        if (HasAuthority())
        {
            SetupTrapTimers();
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🎯 Trap set as ACTIVE: %s"), *ItemName.ToString());
    }
}

void ACYTrapBase::ConvertToPlayerPlacedTrap(AActor* PlacingPlayer)
{
    if (!HasAuthority()) return;

    TrapState = ETrapState::PlayerPlaced;
    SetOwner(PlacingPlayer);
    bIsPickedUp = true;
    
    SetupTrapForCurrentState();
    
    // ✅ 클라이언트들에게 즉시 시각적 업데이트 알림
    MulticastUpdateTrapVisuals();
    
    // ✅ 네트워크 업데이트 강제 실행
    ForceNetUpdate();
    
    UE_LOG(LogTemp, Warning, TEXT("🎯 Trap converted to PlayerPlaced by %s"), 
           PlacingPlayer ? *PlacingPlayer->GetName() : TEXT("Unknown"));
}

void ACYTrapBase::SetupTrapTimers()
{
    if (TrapState != ETrapState::PlayerPlaced) return;

    GetWorld()->GetTimerManager().SetTimer(ArmingTimer, this, &ACYTrapBase::ArmTrap, 
                                          TrapData.ArmingDelay, false);
    
    GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, [this]()
    {
        Destroy();
    }, TrapData.TrapLifetime, false);
}

void ACYTrapBase::ArmTrap()
{
    if (!HasAuthority() || TrapState != ETrapState::PlayerPlaced) return;

    bIsArmed = true;

    if (InteractionSphere)
    {
        InteractionSphere->SetSphereRadius(TrapData.TriggerRadius);
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        
        InteractionSphere->OnComponentBeginOverlap.Clear();
        InteractionSphere->OnComponentEndOverlap.Clear();
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACYTrapBase::OnTriggerSphereOverlap);
    }

    ForceNetUpdate();
    OnTrapArmed();

    UE_LOG(LogTemp, Log, TEXT("✅ Trap armed and ready: %s"), *ItemName.ToString());
}

void ACYTrapBase::OnTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult)
{
    if (TrapState != ETrapState::PlayerPlaced || !bIsArmed || !HasAuthority()) return;

    if (OtherActor == GetOwner())
    {
        UE_LOG(LogTemp, Log, TEXT("🚫 Trap owner stepped on own trap - ignoring"));
        return;
    }

    ACYPlayerCharacter* Target = Cast<ACYPlayerCharacter>(OtherActor);
    if (!Target) return;

    UE_LOG(LogTemp, Warning, TEXT("💥 TRAP TRIGGERED! %s stepped on %s's trap"), 
           *Target->GetName(), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));

    OnTrapTriggered(Target);
    ApplyTrapEffects(Target);
    MulticastOnTrapTriggered(Target);
    
    Destroy();
}

// ✅ 개선된 멀티캐스트 함수 - 시각적 업데이트
void ACYTrapBase::MulticastUpdateTrapVisuals_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("🎨 MulticastUpdateTrapVisuals called on %s"), 
           HasAuthority() ? TEXT("Server") : TEXT("Client"));
    
    // ✅ 클라이언트에서 강제로 시각적 재설정
    InitializeTrapVisuals();
}

void ACYTrapBase::MulticastOnTrapTriggered_Implementation(ACYPlayerCharacter* Target)
{
    if (!HasAuthority()) // 클라이언트에서만 실행
    {
        OnTrapTriggered(Target);
    }
}

void ACYTrapBase::OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void ACYTrapBase::OnPickupSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

void ACYTrapBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnTrapDestroyed();
    Super::EndPlay(EndPlayReason);
}

void ACYTrapBase::OnTrapSpawned_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("🔧 Base trap spawned"));
}

void ACYTrapBase::OnTrapArmed_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("⚡ Base trap armed"));
    PlayTrapSound();
}

void ACYTrapBase::OnTrapTriggered_Implementation(ACYPlayerCharacter* Target)
{
    if (Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("💥 Base trap triggered on %s"), *Target->GetName());
    }
}

void ACYTrapBase::OnTrapDestroyed_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("🗑️ Base trap destroyed"));
}

// ✅ 기본 구현 - 하위 클래스에서 오버라이드해야 함
void ACYTrapBase::SetupTrapVisuals_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("🎨 SetupTrapVisuals (BASE) called for %s"), *GetClass()->GetName());

    if (ItemMesh)
    {
        // ✅ 기본 메시 설정 (이미 생성자에서 설정되었거나 하위 클래스에서 설정됨)
        if (!ItemMesh->GetStaticMesh())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ BASE: No mesh set in constructor! This should not happen."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎨 BASE: Using existing mesh: %s"), 
                   ItemMesh->GetStaticMesh() ? *ItemMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
        }
        
        // ✅ 가시성 강제 보장
        ItemMesh->SetVisibility(true);
        ItemMesh->SetHiddenInGame(false);
        ItemMesh->SetCastShadow(true);
        ItemMesh->MarkRenderStateDirty();
        
        UE_LOG(LogTemp, Warning, TEXT("🎨 BASE trap visuals setup complete: %s"), 
               ItemMesh->GetStaticMesh() ? *ItemMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ BASE: ItemMesh is NULL"));
    }
}

void ACYTrapBase::PlayTrapSound_Implementation()
{
    if (TrapData.TriggerSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), TrapData.TriggerSound, GetActorLocation());
    }
}

void ACYTrapBase::ApplyCustomEffects_Implementation(ACYPlayerCharacter* Target)
{
    UE_LOG(LogTemp, Log, TEXT("🎯 Applying base custom effects"));
}

void ACYTrapBase::ApplyTrapEffects(ACYPlayerCharacter* Target)
{
	if (!Target) 
	{
		UE_LOG(LogTemp, Error, TEXT("❌ ApplyTrapEffects: Target is null"));
		return;
	}

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!TargetASC) 
	{
		UE_LOG(LogTemp, Error, TEXT("❌ ApplyTrapEffects: Target has no AbilitySystemComponent"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("🎯 Applying trap effects to %s"), *Target->GetName());

	// ✅ TrapData의 GameplayEffects 적용
	int32 EffectCount = 0;
	for (TSubclassOf<UGameplayEffect> EffectClass : TrapData.GameplayEffects)
	{
		if (EffectClass)
		{
			FActiveGameplayEffectHandle Handle = ApplySingleEffect(TargetASC, EffectClass);
			if (Handle.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("✅ Applied TrapData effect: %s"), *EffectClass->GetName());
				EffectCount++;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ Failed to apply TrapData effect: %s"), *EffectClass->GetName());
			}
		}
	}

	// ✅ 백업: AttributeSet 방식이 실패한 경우 직접 제어
	if (EffectCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ No GameplayEffects applied, using direct movement control"));
		ApplyDirectMovementControl(Target);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ Applied %d GameplayEffects successfully"), EffectCount);
	}

	// 커스텀 효과 적용
	ApplyCustomEffects(Target);
}

FActiveGameplayEffectHandle ACYTrapBase::ApplySingleEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!TargetASC || !EffectClass) 
	{
		return FActiveGameplayEffectHandle();
	}

	// ✅ 단순화된 효과 적용
	FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
    
	FGameplayEffectSpecHandle EffectSpec = TargetASC->MakeOutgoingSpec(EffectClass, 1, EffectContext);
	if (EffectSpec.IsValid())
	{
		FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
        
		if (Handle.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("✅ Successfully applied effect: %s"), *EffectClass->GetName());
		}
        
		return Handle;
	}

	return FActiveGameplayEffectHandle();
}

bool ACYTrapBase::EnsureTargetHasCombatAttributeSet(ACYPlayerCharacter* Target)
{
    if (!Target) return false;

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!TargetASC) return false;

    // ✅ CombatAttributeSet 존재 확인
    const UCYCombatAttributeSet* CombatAttrSet = TargetASC->GetSet<UCYCombatAttributeSet>();
    if (!CombatAttrSet)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Target %s does not have UCYCombatAttributeSet"), 
               *Target->GetName());
        
        // ✅ AttributeSet이 없으면 동적으로 추가 시도
        UCYCombatAttributeSet* NewAttrSet = NewObject<UCYCombatAttributeSet>(TargetASC->GetOwner());
        if (NewAttrSet)
        {
            TargetASC->AddAttributeSetSubobject(NewAttrSet);
            UE_LOG(LogTemp, Warning, TEXT("✅ Added UCYCombatAttributeSet to %s"), *Target->GetName());
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to create UCYCombatAttributeSet for %s"), 
                   *Target->GetName());
            return false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ Target %s has UCYCombatAttributeSet"), *Target->GetName());
    return true;
}

void ACYTrapBase::LogCurrentMoveSpeed(ACYPlayerCharacter* Target)
{
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!TargetASC) return;

    const UCYCombatAttributeSet* CombatAttrSet = TargetASC->GetSet<UCYCombatAttributeSet>();
    if (CombatAttrSet)
    {
        float CurrentMoveSpeed = CombatAttrSet->GetMoveSpeed();
        UE_LOG(LogTemp, Warning, TEXT("🏃 Current AttributeSet MoveSpeed: %f"), CurrentMoveSpeed);
    }

    // MovementComponent 확인
    if (ACharacter* Character = Cast<ACharacter>(Target))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            float CurrentMaxWalkSpeed = MovementComp->MaxWalkSpeed;
            UE_LOG(LogTemp, Warning, TEXT("🏃 Current MovementComponent MaxWalkSpeed: %f"), 
                   CurrentMaxWalkSpeed);
        }
    }
}

void ACYTrapBase::ApplyDirectMovementControl(ACYPlayerCharacter* Target)
{
	if (!Target) return;

	ACharacter* Character = Cast<ACharacter>(Target);
	if (!Character) return;

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp) return;

	float TargetSpeed = 0.0f;
	float Duration = 3.0f;
	FString EffectName = TEXT("Unknown");

	// ✅ 트랩 타입별 속도 설정
	switch (TrapType)
	{
	case ETrapType::Freeze:
		TargetSpeed = 0.0f;
		Duration = 3.0f;
		EffectName = TEXT("Freeze");
		break;
            
	case ETrapType::Slow:
		TargetSpeed = 100.0f;
		Duration = 5.0f;
		EffectName = TEXT("Slow");
		break;
            
	default:
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("🎯 BACKUP: Applying direct %s effect"), *EffectName);

	// ✅ 즉시 적용
	MovementComp->MaxWalkSpeed = TargetSpeed;
    
	if (TargetSpeed <= 0.0f)
	{
		MovementComp->StopMovementImmediately();
		MovementComp->MaxAcceleration = 0.0f;
		MovementComp->JumpZVelocity = 0.0f;
	}
	else
	{
		MovementComp->MaxAcceleration = 200.0f;
		MovementComp->JumpZVelocity = 0.0f;
	}

	// ✅ 타이머로 복원
	FTimerHandle RestoreTimer;
	GetWorld()->GetTimerManager().SetTimer(RestoreTimer, [MovementComp]()
	{
		if (MovementComp)
		{
			MovementComp->MaxWalkSpeed = 600.0f;
			MovementComp->MaxAcceleration = 2048.0f;
			MovementComp->JumpZVelocity = 600.0f;
			UE_LOG(LogTemp, Warning, TEXT("🏃 BACKUP: Restored normal movement"));
		}
	}, Duration, false);

	UE_LOG(LogTemp, Warning, TEXT("✅ BACKUP: Applied %s effect"), *EffectName);
}