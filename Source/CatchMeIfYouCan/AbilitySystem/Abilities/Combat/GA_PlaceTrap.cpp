#include "AbilitySystem/Abilities/Combat/GA_PlaceTrap.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"
#include "Character/CYPlayerCharacter.h"
#include "Items/Traps/CYTrapBase.h"
#include "Items/Traps/CYSlowTrap.h"
#include "Items/Traps/CYFreezeTrap.h"
#include "Items/Traps/CYDamageTrap.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UGA_PlaceTrap::UGA_PlaceTrap()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

    // 태그 설정
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(CYGameplayTags::Ability_Combat_PlaceTrap);
    SetAssetTags(AssetTags);
    
    // 블로킹 태그 설정
    FGameplayTagContainer BlockedTags;
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Stunned);
    BlockedTags.AddTag(CYGameplayTags::State_Combat_Dead);
    ActivationBlockedTags = BlockedTags;
    
    // 쿨다운 GE 클래스 설정
    CooldownGameplayEffectClass = UGE_TrapPlaceCooldown::StaticClass();
    
    UE_LOG(LogTemp, Warning, TEXT("🛠️ PlaceTrap GA created"));
}

void UGA_PlaceTrap::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 🔥 쿨다운 먼저 체크 및 적용
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("⏰ Trap placement on cooldown"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 🔥 소스 오브젝트에서 트랩 아이템 가져오기
    ACYTrapBase* TrapItem = GetTrapItemFromSource();
    if (!TrapItem)
    {
        // 폴백: 인벤토리에서 찾기
        TrapItem = Cast<ACYTrapBase>(FindTrapItemInInventory());
        if (!TrapItem)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ No trap item found"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
    }

    // 트랩 설치 위치 계산
    FVector SpawnLocation = CalculateSpawnLocation();
    
    // 트랩 생성 및 설치
    ACYTrapBase* NewTrap = CreateTrapFromItem(TrapItem, SpawnLocation);
    
    if (NewTrap)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ Trap placed: %s at %s"), 
               *NewTrap->ItemName.ToString(), *SpawnLocation.ToString());
        
        // 아이템 소모
        ConsumeItemFromInventory(TrapItem);
        
        // 성공 메시지
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
                FString::Printf(TEXT("🎯 %s Placed!"), *NewTrap->ItemName.ToString()));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create trap"));
    }
    
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

ACYTrapBase* UGA_PlaceTrap::GetTrapItemFromSource() const
{
    // 어빌리티 스펙의 소스 오브젝트에서 트랩 아이템 가져오기
    const FGameplayAbilitySpec* AbilitySpec = GetCurrentAbilitySpec();
    if (AbilitySpec && AbilitySpec->SourceObject.IsValid())
    {
        ACYTrapBase* TrapItem = Cast<ACYTrapBase>(AbilitySpec->SourceObject.Get());
        if (TrapItem)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎯 Using trap from source: %s"), *TrapItem->ItemName.ToString());
            return TrapItem;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎯 No trap source found, searching inventory..."));
    return nullptr;
}

ACYItemBase* UGA_PlaceTrap::FindTrapItemInInventory()
{
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor) return nullptr;

    UCYInventoryComponent* InventoryComp = OwnerActor->FindComponentByClass<UCYInventoryComponent>();
    if (!InventoryComp) return nullptr;

    // 아이템 슬롯에서 트랩 찾기
    for (ACYItemBase* Item : InventoryComp->ItemSlots)
    {
        if (Item && Item->ItemType == EItemType::Trap && Item->ItemCount > 0)
        {
            return Item;
        }
    }

    return nullptr;
}

ACYTrapBase* UGA_PlaceTrap::CreateTrapFromItem(ACYItemBase* TrapItem, const FVector& Location)
{
    if (!TrapItem || !GetWorld()) return nullptr;
    
    // 🔥 이미 트랩 액터인 경우 복제하여 새로 생성
    ACYTrapBase* SourceTrap = Cast<ACYTrapBase>(TrapItem);
    if (SourceTrap)
    {
        // 같은 클래스의 새 트랩 생성
        TSubclassOf<ACYTrapBase> TrapClass = SourceTrap->GetClass();
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetAvatarActorFromActorInfo();
        SpawnParams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACYTrapBase* NewTrap = GetWorld()->SpawnActor<ACYTrapBase>(TrapClass, Location, FRotator::ZeroRotator, SpawnParams);
        
        if (NewTrap)
        {
            // 플레이어가 설치한 트랩으로 변환
            NewTrap->PlaceTrap(Location, Cast<ACYPlayerCharacter>(GetAvatarActorFromActorInfo()));
            return NewTrap;
        }
    }
    
    // 폴백: 이름 기반 트랩 생성
    TSubclassOf<ACYTrapBase> TrapClass = nullptr;
    FString ItemName = TrapItem->ItemName.ToString().ToLower();
    
    if (ItemName.Contains(TEXT("slow")))
    {
        TrapClass = ACYSlowTrap::StaticClass();
    }
    else if (ItemName.Contains(TEXT("freeze")))
    {
        TrapClass = ACYFreezeTrap::StaticClass();
    }
    else if (ItemName.Contains(TEXT("damage")))
    {
        TrapClass = ACYDamageTrap::StaticClass();
    }
    else
    {
        // 기본값: 슬로우 트랩
        TrapClass = ACYSlowTrap::StaticClass();
    }
    
    if (!TrapClass) return nullptr;
    
    // 트랩 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetAvatarActorFromActorInfo();
    SpawnParams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACYTrapBase* NewTrap = GetWorld()->SpawnActor<ACYTrapBase>(TrapClass, Location, FRotator::ZeroRotator, SpawnParams);
    
    if (NewTrap)
    {
        // 플레이어가 설치한 트랩으로 변환
        NewTrap->PlaceTrap(Location, Cast<ACYPlayerCharacter>(GetAvatarActorFromActorInfo()));
    }
    
    return NewTrap;
}

FVector UGA_PlaceTrap::CalculateSpawnLocation()
{
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    if (!OwnerActor) return FVector::ZeroVector;

    UCYWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UCYWeaponComponent>();
    if (WeaponComp)
    {
        FHitResult HitResult;
        if (WeaponComp->PerformLineTrace(HitResult, 300.0f))
        {
            return HitResult.Location;
        }
    }

    // 백업: 캐릭터 앞쪽에 배치
    FVector ForwardLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * 200.0f;
    ForwardLocation.Z = OwnerActor->GetActorLocation().Z;
    return ForwardLocation;
}

void UGA_PlaceTrap::ConsumeItemFromInventory(ACYItemBase* Item)
{
    if (!Item) return;
    
    AActor* OwnerActor = GetAvatarActorFromActorInfo();
    UCYInventoryComponent* InventoryComp = OwnerActor->FindComponentByClass<UCYInventoryComponent>();
    if (!InventoryComp) return;

    // 아이템 수량 감소
    Item->ItemCount--;
    
    if (Item->ItemCount <= 0)
    {
        // 아이템이 모두 소모되면 슬롯에서 제거
        for (int32 i = 0; i < InventoryComp->ItemSlots.Num(); ++i)
        {
            if (InventoryComp->ItemSlots[i] == Item)
            {
                InventoryComp->ItemSlots[i] = nullptr;
                InventoryComp->OnInventoryChanged.Broadcast(i + 4, nullptr); // 4~9번 키
                Item->Destroy();
                break;
            }
        }
    }
    else
    {
        // 수량만 감소한 경우
        for (int32 i = 0; i < InventoryComp->ItemSlots.Num(); ++i)
        {
            if (InventoryComp->ItemSlots[i] == Item)
            {
                InventoryComp->OnInventoryChanged.Broadcast(i + 4, Item); // 4~9번 키
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔧 Consumed trap item: %s (Remaining: %d)"), 
           *Item->ItemName.ToString(), Item->ItemCount);
}