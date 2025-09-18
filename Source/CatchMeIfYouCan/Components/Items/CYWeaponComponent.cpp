﻿// CYWeaponComponent.cpp - testun 방식으로 단순화

#include "Components/Items/CYWeaponComponent.h"
#include "Items/CYWeaponBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "CYInventoryComponent.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "Net/UnrealNetwork.h"

UCYWeaponComponent::UCYWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UCYWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCYWeaponComponent, CurrentWeapon);
}

bool UCYWeaponComponent::EquipWeapon(ACYWeaponBase* Weapon)
{
    if (!Weapon || !GetOwner()->HasAuthority()) return false;

    if (CurrentWeapon)
    {
        UnequipWeapon();
    }

    CurrentWeapon = Weapon;
    AttachWeaponToOwner(Weapon);
    DisableWeaponInteraction(Weapon);
    
    OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    return true;
}

bool UCYWeaponComponent::UnequipWeapon()
{
    if (!CurrentWeapon || !GetOwner()->HasAuthority()) return false;

    ACYWeaponBase* OldWeapon = CurrentWeapon;
    CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    CurrentWeapon = nullptr;

    OnWeaponChanged.Broadcast(OldWeapon, nullptr);
    return true;
}

bool UCYWeaponComponent::PerformAttack()
{
    if (!GetOwner()->HasAuthority()) 
    {
        return false;
    }

    if (CurrentWeapon) 
    {
        return ExecuteWeaponAttack();
    }
    
    return false;
}

bool UCYWeaponComponent::ExecuteWeaponAttack()
{
    if (!CurrentWeapon) 
    {
        return false;
    }

    UCYAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
    if (!ASC) 
    {
        return false;
    }

    // ✅ testun 방식: 단순한 어빌리티 활성화
    return ASC->TryActivateAbilityByTag(CYGameplayTags::Ability_Combat_WeaponAttack);
}

void UCYWeaponComponent::DisplayInventoryStatus()
{
    if (!GEngine) return;

    UCYInventoryComponent* InventoryComp = GetOwner()->FindComponentByClass<UCYInventoryComponent>();
    if (!InventoryComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("❌ No InventoryComponent found"));
        return;
    }

    GEngine->ClearOnScreenDebugMessages();
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("=== 📦 INVENTORY STATUS ==="));
    
    // 무기 슬롯 (1~3번 키)
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("🗡️ WEAPONS (Keys 1-3):"));
    for (int32 i = 0; i < InventoryComp->WeaponSlots.Num(); ++i)
    {
        FString WeaponInfo;
        if (InventoryComp->WeaponSlots[i])
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 1, 
                *InventoryComp->WeaponSlots[i]->ItemName.ToString(), 
                InventoryComp->WeaponSlots[i]->ItemCount
            );
            
            ACYWeaponBase* SlotWeapon = Cast<ACYWeaponBase>(InventoryComp->WeaponSlots[i]);
            if (CurrentWeapon && SlotWeapon && CurrentWeapon == SlotWeapon)
            {
                WeaponInfo += TEXT(" ⭐ EQUIPPED");
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, WeaponInfo);
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, WeaponInfo);
            }
        }
        else
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] Empty"), i + 1);
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Silver, WeaponInfo);
        }
    }
    
    // 아이템 슬롯 (4~9번 키)
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("🎒 ITEMS (Keys 4-9):"));
    int32 MaxDisplayItems = FMath::Min(6, InventoryComp->ItemSlots.Num());
    for (int32 i = 0; i < MaxDisplayItems; ++i)
    {
        FString ItemInfo;
        if (InventoryComp->ItemSlots[i])
        {
            ItemInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 4, 
                *InventoryComp->ItemSlots[i]->ItemName.ToString(), 
                InventoryComp->ItemSlots[i]->ItemCount
            );
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, ItemInfo);
        }
        else
        {
            ItemInfo = FString::Printf(TEXT("  [%d] Empty"), i + 4);
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Silver, ItemInfo);
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("=================="));
}

void UCYWeaponComponent::ClientDisplayInventoryStatus_Implementation()
{
    DisplayInventoryStatus();
}

bool UCYWeaponComponent::PerformLineTrace(FHitResult& OutHit, float Range)
{
    UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
    if (!Camera) return false;

    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + (Camera->GetForwardVector() * Range);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    return GetWorld()->LineTraceSingleByChannel(
        OutHit, Start, End, ECC_Visibility, Params
    );
}

void UCYWeaponComponent::OnRep_CurrentWeapon()
{
    OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    
    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
    }
}

UCYAbilitySystemComponent* UCYWeaponComponent::GetOwnerAbilitySystemComponent() const
{
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
    {
        return Cast<UCYAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
    }
    return nullptr;
}

USkeletalMeshComponent* UCYWeaponComponent::GetOwnerMesh() const
{
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        return Character->GetMesh();
    }
    return nullptr;
}

void UCYWeaponComponent::AttachWeaponToOwner(ACYWeaponBase* Weapon)
{
    if (!Weapon) return;

    USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
    if (OwnerMesh)
    {
        Weapon->AttachToComponent(
            OwnerMesh,
            FAttachmentTransformRules::SnapToTargetIncludingScale,
            WeaponSocketName
        );
    }
}

void UCYWeaponComponent::DisableWeaponInteraction(ACYWeaponBase* Weapon)
{
    if (!Weapon) return;

    if (Weapon->ItemMesh)
    {
        Weapon->ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (Weapon->InteractionSphere)
    {
        Weapon->InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}