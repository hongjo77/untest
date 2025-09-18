// CYTrapBase.cpp - testun 방식으로 단순화

#include "Items/Traps/CYTrapBase.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "Character/CYPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
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

    bReplicates = true;
    SetReplicateMovement(true);
    bAlwaysRelevant = true;

    // 기본 설정
    TriggerRadius = 100.0f;
    ArmingDelay = 2.0f;
    TrapLifetime = 60.0f;

    // 기본 트랩 데이터 초기화
    TrapData.TrapType = TrapType;
    TrapData.TrapName = ItemName;
    TrapData.TrapDescription = ItemDescription;
    TrapData.TriggerRadius = TriggerRadius;
    TrapData.ArmingDelay = ArmingDelay;
    TrapData.TrapLifetime = TrapLifetime;

    // 기본 메시 설정 (하위 클래스에서 오버라이드)
    if (ItemMesh)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseTrapMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        if (BaseTrapMesh.Succeeded())
        {
            ItemMesh->SetStaticMesh(BaseTrapMesh.Object);
            ItemMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.1f));
        }
    }
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

    InitializeTrapVisuals();
    SetupTrapForCurrentState();
    
    if (HasAuthority())
    {
        OnTrapSpawned();
    }
}

void ACYTrapBase::InitializeTrapVisuals()
{
    SetupTrapVisuals();
}

void ACYTrapBase::SetupTrapForCurrentState()
{
    if (!InteractionSphere) return;

    if (TrapState == ETrapState::MapPlaced)
    {
        // 픽업 가능 상태
        InteractionSphere->SetSphereRadius(150.0f);
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
        
        if (HasAuthority())
        {
            InteractionSphere->OnComponentBeginOverlap.Clear();
            InteractionSphere->OnComponentEndOverlap.Clear();
            InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACYTrapBase::OnPickupSphereOverlap);
            InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ACYTrapBase::OnPickupSphereEndOverlap);
        }
    }
    else if (TrapState == ETrapState::PlayerPlaced)
    {
        // 트리거 모드
        if (HasAuthority())
        {
            SetupTrapTimers();
        }
    }
}

void ACYTrapBase::ConvertToPlayerPlacedTrap(AActor* PlacingPlayer)
{
    if (!HasAuthority()) return;

    TrapState = ETrapState::PlayerPlaced;
    SetOwner(PlacingPlayer);
    bIsPickedUp = true;
    
    SetupTrapForCurrentState();
    MulticastUpdateTrapVisuals();
    ForceNetUpdate();
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
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACYTrapBase::OnTriggerSphereOverlap);
    }

    ForceNetUpdate();
    OnTrapArmed();
}

void ACYTrapBase::OnTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult)
{
    if (TrapState != ETrapState::PlayerPlaced || !bIsArmed || !HasAuthority()) return;

    if (OtherActor == GetOwner()) return;

    ACYPlayerCharacter* Target = Cast<ACYPlayerCharacter>(OtherActor);
    if (!Target) return;

    UE_LOG(LogTemp, Warning, TEXT("TRAP TRIGGERED! %s stepped on %s's trap"), 
           *Target->GetName(), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));

    OnTrapTriggered(Target);
    ApplyTrapEffects(Target);
    MulticastOnTrapTriggered(Target);
    
    Destroy();
}

void ACYTrapBase::ApplyTrapEffects(ACYPlayerCharacter* Target)
{
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!TargetASC) return;

    UE_LOG(LogTemp, Warning, TEXT("🎯 Applying trap effects to %s"), *Target->GetName());

    // ✅ testun 방식: 단순한 GameplayEffect 적용
    for (TSubclassOf<UGameplayEffect> EffectClass : TrapData.GameplayEffects)
    {
        if (EffectClass)
        {
            ApplySingleEffect(TargetASC, EffectClass);
        }
    }

    for (TSubclassOf<UGameplayEffect> EffectClass : ItemEffects)
    {
        if (EffectClass)
        {
            ApplySingleEffect(TargetASC, EffectClass);
        }
    }

    ApplyCustomEffects(Target);
}

FActiveGameplayEffectHandle ACYTrapBase::ApplySingleEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
    if (!TargetASC || !EffectClass) 
    {
        return FActiveGameplayEffectHandle();
    }

    FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);
    
    FGameplayEffectSpecHandle EffectSpec = TargetASC->MakeOutgoingSpec(EffectClass, 1, EffectContext);
    if (EffectSpec.IsValid())
    {
        FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
        if (Handle.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ Applied effect: %s"), *EffectClass->GetName());
        }
        return Handle;
    }

    return FActiveGameplayEffectHandle();
}

void ACYTrapBase::MulticastUpdateTrapVisuals_Implementation()
{
    InitializeTrapVisuals();
}

void ACYTrapBase::MulticastOnTrapTriggered_Implementation(ACYPlayerCharacter* Target)
{
    if (!HasAuthority())
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

// 기본 구현들 (하위 클래스에서 오버라이드)
void ACYTrapBase::OnTrapSpawned_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("Base trap spawned"));
}

void ACYTrapBase::OnTrapArmed_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Trap armed: %s"), *ItemName.ToString());
    PlayTrapSound();
}

void ACYTrapBase::OnTrapTriggered_Implementation(ACYPlayerCharacter* Target)
{
    if (Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("Base trap triggered on %s"), *Target->GetName());
    }
}

void ACYTrapBase::OnTrapDestroyed_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("Base trap destroyed"));
}

void ACYTrapBase::SetupTrapVisuals_Implementation()
{
    if (ItemMesh)
    {
        ItemMesh->SetVisibility(true);
        ItemMesh->SetHiddenInGame(false);
        ItemMesh->SetCastShadow(true);
        ItemMesh->MarkRenderStateDirty();
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
    // 하위 클래스에서 구현
}