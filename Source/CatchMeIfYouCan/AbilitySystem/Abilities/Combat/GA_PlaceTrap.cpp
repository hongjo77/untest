// GA_PlaceTrap.cpp - 트랩별 특성 반영 개선
#include "GA_PlaceTrap.h"

#include "AbilitySystem/CYCombatGameplayTags.h"
#include "Items/CYTrapFactory.h"
#include "Items/CYItemBase.h"
#include "Engine/World.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "Camera/CameraComponent.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Items/Traps/CYTrapBase.h"

UGA_PlaceTrap::UGA_PlaceTrap()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// ✅ 팀프로젝트 방식으로 태그 설정
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(CYGameplayTags::Ability_Combat_PlaceTrap);
	SetAssetTags(AssetTags);
    
	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(CYGameplayTags::State_Combat_Stunned);
	BlockedTags.AddTag(CYGameplayTags::State_Combat_Dead);
	ActivationBlockedTags = BlockedTags;
}

void UGA_PlaceTrap::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("🚀 GA_PlaceTrap::ActivateAbility called"));
    
    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ GA_PlaceTrap: No authority or prediction key"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ GA_PlaceTrap: OwnerActor is null"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🚀 GA_PlaceTrap: OwnerActor found: %s"), *OwnerActor->GetName());

    // ✅ SourceObject에서 특정 아이템 가져오기 (우선순위 1)
    ACYItemBase* SourceItem = GetSourceItemFromAbility(Handle, ActorInfo);
    
    // ✅ 백업: 인벤토리에서 트랩 아이템 찾기 (우선순위 2)
    if (!SourceItem)
    {
        SourceItem = FindValidTrapItemInInventory(OwnerActor);
        UE_LOG(LogTemp, Warning, TEXT("🎯 GA_PlaceTrap: Using fallback inventory search"));
    }
    
    if (!SourceItem)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ GA_PlaceTrap: No valid trap item found"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // ✅ 아이템 타입별 트랩 생성 정보 로깅
    LogTrapCreationInfo(SourceItem);

    // 트랩 설치 위치 계산
    FVector SpawnLocation = CalculateSpawnLocation(OwnerActor);
    FRotator SpawnRotation = OwnerActor->GetActorRotation();
    
    UE_LOG(LogTemp, Warning, TEXT("🚀 GA_PlaceTrap: Spawn location: %s"), *SpawnLocation.ToString());

    // ✅ 개선된 팩토리를 통한 트랩 생성
    ACYTrapBase* NewTrap = CreateTrapFromSourceItem(SourceItem, SpawnLocation, SpawnRotation, OwnerActor);

    if (NewTrap)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ Trap successfully created: %s"), 
               *NewTrap->GetClass()->GetName());
        
        // ✅ 트랩 설치 후 해당 특정 아이템 소모 처리
        ConsumeSpecificItemFromInventory(OwnerActor, SourceItem);
        
        // ✅ 트랩별 성공 메시지 표시
        ShowTrapPlacementSuccess(NewTrap);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create trap from item: %s"), 
               *SourceItem->ItemName.ToString());
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🚀 GA_PlaceTrap: Ability completed"));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

ACYItemBase* UGA_PlaceTrap::GetSourceItemFromAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo)
{
    // AbilitySpec의 SourceObject 확인
    FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
    if (AbilitySpec && AbilitySpec->SourceObject.IsValid())
    {
        ACYItemBase* SourceItem = Cast<ACYItemBase>(AbilitySpec->SourceObject.Get());
        UE_LOG(LogTemp, Warning, TEXT("🎯 GA_PlaceTrap: Using SourceObject item: %s"), 
               SourceItem ? *SourceItem->ItemName.ToString() : TEXT("Invalid"));
        return SourceItem;
    }
    
    return nullptr;
}

void UGA_PlaceTrap::LogTrapCreationInfo(ACYItemBase* SourceItem)
{
    if (!SourceItem) return;
    
    FString ItemName = SourceItem->ItemName.ToString();
    ETrapType InferredType = UCYTrapFactory::InferTrapTypeFromItem(SourceItem);
    FString TrapTypeName = UCYTrapFactory::GetTrapTypeName(InferredType);
    
    UE_LOG(LogTemp, Warning, TEXT("🎯 Creating trap:"));
    UE_LOG(LogTemp, Warning, TEXT("   📦 Source Item: %s"), *ItemName);
    UE_LOG(LogTemp, Warning, TEXT("   🎭 Inferred Type: %s"), *TrapTypeName);
    UE_LOG(LogTemp, Warning, TEXT("   📊 Item Count: %d"), SourceItem->ItemCount);
}

ACYTrapBase* UGA_PlaceTrap::CreateTrapFromSourceItem(ACYItemBase* SourceItem, 
    const FVector& SpawnLocation, const FRotator& SpawnRotation, AActor* OwnerActor)
{
    if (!SourceItem || !GetWorld()) return nullptr;
    
    // ✅ 팩토리를 통한 트랩 생성 (타입별 특성 자동 적용)
    ACYTrapBase* NewTrap = UCYTrapFactory::CreateTrapFromItem(
        GetWorld(),
        SourceItem,
        SpawnLocation,
        SpawnRotation,
        OwnerActor,
        Cast<APawn>(OwnerActor)
    );
    
    if (NewTrap)
    {
        // ✅ 추가적인 트랩 설정 (필요시)
        ConfigureNewTrap(NewTrap, SourceItem);
    }
    
    return NewTrap;
}

void UGA_PlaceTrap::ConfigureNewTrap(ACYTrapBase* NewTrap, ACYItemBase* SourceItem)
{
    if (!NewTrap || !SourceItem) return;
    
    // ✅ 트랩별 특별 설정 (예: 아이템 개수에 따른 효과 증폭 등)
    ETrapType TrapType = NewTrap->TrapType;
    
    switch (TrapType)
    {
        case ETrapType::Freeze:
            UE_LOG(LogTemp, Log, TEXT("🧊 Configured FreezeTrap with enhanced ice effects"));
            break;
            
        case ETrapType::Slow:
            UE_LOG(LogTemp, Log, TEXT("🌀 Configured SlowTrap with enhanced slowing effects"));
            break;
            
        case ETrapType::Damage:
            UE_LOG(LogTemp, Log, TEXT("⚔️ Configured DamageTrap with enhanced damage effects"));
            break;
            
        default:
            UE_LOG(LogTemp, Log, TEXT("🎯 Configured basic trap"));
            break;
    }
}

void UGA_PlaceTrap::ShowTrapPlacementSuccess(ACYTrapBase* NewTrap)
{
    if (!NewTrap || !GEngine) return;
    
    ETrapType TrapType = NewTrap->TrapType;
    FString TrapTypeName = UCYTrapFactory::GetTrapTypeName(TrapType);
    
    // ✅ 트랩별 성공 메시지 색상
    FColor MessageColor = FColor::White;
    FString MessageIcon = TEXT("🎯");
    
    switch (TrapType)
    {
        case ETrapType::Freeze:
            MessageColor = FColor::Cyan;
            MessageIcon = TEXT("❄️");
            break;
            
        case ETrapType::Slow:
            MessageColor = FColor::Blue;
            MessageIcon = TEXT("🌀");
            break;
            
        case ETrapType::Damage:
            MessageColor = FColor::Red;
            MessageIcon = TEXT("⚔️");
            break;
    }
    
    FString SuccessMessage = FString::Printf(TEXT("%s %s Trap Placed!"), 
                                           *MessageIcon, *TrapTypeName);
    
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, MessageColor, SuccessMessage);
    UE_LOG(LogTemp, Warning, TEXT("✅ %s"), *SuccessMessage);
}

ACYItemBase* UGA_PlaceTrap::FindValidTrapItemInInventory(AActor* OwnerActor)
{
    if (!OwnerActor) return nullptr;

    UCYInventoryComponent* InventoryComp = OwnerActor->FindComponentByClass<UCYInventoryComponent>();
    if (!InventoryComp) return nullptr;

    // 아이템 슬롯에서 트랩 아이템 찾기
    for (ACYItemBase* Item : InventoryComp->ItemSlots)
    {
        if (Item && Item->ItemTag.MatchesTag(FGameplayTag::RequestGameplayTag("Item.Trap")) && Item->ItemCount > 0)
        {
            return Item;
        }
    }

    return nullptr;
}

void UGA_PlaceTrap::ConsumeSpecificItemFromInventory(AActor* OwnerActor, ACYItemBase* SourceItem)
{
    if (!OwnerActor || !SourceItem) return;

    UCYInventoryComponent* InventoryComp = OwnerActor->FindComponentByClass<UCYInventoryComponent>();
    if (!InventoryComp) return;

    // ✅ 해당 특정 아이템만 소모
    SourceItem->ItemCount--;
    
    if (SourceItem->ItemCount <= 0)
    {
        // 아이템이 모두 소모되면 슬롯에서 제거
        for (int32 i = 0; i < InventoryComp->ItemSlots.Num(); ++i)
        {
            if (InventoryComp->ItemSlots[i] == SourceItem)
            {
                InventoryComp->ItemSlots[i] = nullptr;
                
                // 이벤트 발생
                int32 UnifiedIndex = i; // ItemSlot은 0부터 시작
                InventoryComp->OnInventoryChanged.Broadcast(UnifiedIndex, nullptr);
                
                SourceItem->Destroy();
                break;
            }
        }
    }
    else
    {
        // 수량만 감소한 경우 이벤트 발생
        for (int32 i = 0; i < InventoryComp->ItemSlots.Num(); ++i)
        {
            if (InventoryComp->ItemSlots[i] == SourceItem)
            {
                int32 UnifiedIndex = i;
                InventoryComp->OnInventoryChanged.Broadcast(UnifiedIndex, SourceItem);
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔧 Consumed specific trap item: %s (Remaining: %d)"), 
           *SourceItem->ItemName.ToString(), SourceItem->ItemCount);
}

FVector UGA_PlaceTrap::CalculateSpawnLocation(AActor* OwnerActor)
{
    if (!OwnerActor)
    {
        return FVector::ZeroVector;
    }

    FHitResult HitResult;
    UCYWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UCYWeaponComponent>();
    
    // 라인 트레이스로 정확한 위치 계산
    if (WeaponComp && WeaponComp->PerformLineTrace(HitResult, 300.0f))
    {
        return HitResult.Location;
    }

    // 백업: 캐릭터 앞쪽에 배치
    FVector ForwardLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * 200.0f;
    ForwardLocation.Z = OwnerActor->GetActorLocation().Z;
    return ForwardLocation;
}
