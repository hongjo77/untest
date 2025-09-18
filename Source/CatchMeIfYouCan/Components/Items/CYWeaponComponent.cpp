#include "Components/Items/CYWeaponComponent.h"
#include "Items/CYWeaponBase.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

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

    // 기존 무기 해제
    if (CurrentWeapon)
    {
        UnequipWeapon();
    }

    CurrentWeapon = Weapon;
    AttachWeaponToOwner(Weapon);
    
    // 충돌 비활성화
    if (Weapon->ItemMesh)
    {
        Weapon->ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (Weapon->InteractionSphere)
    {
        Weapon->InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon equipped: %s"), *Weapon->ItemName.ToString());
    return true;
}

bool UCYWeaponComponent::UnequipWeapon()
{
    if (!CurrentWeapon || !GetOwner()->HasAuthority()) return false;

    ACYWeaponBase* OldWeapon = CurrentWeapon;
    CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    CurrentWeapon = nullptr;

    OnWeaponChanged.Broadcast(OldWeapon, nullptr);
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon unequipped: %s"), *OldWeapon->ItemName.ToString());
    return true;
}

bool UCYWeaponComponent::PerformAttack()
{
    if (!GetOwner()->HasAuthority()) return false;
    
    if (!CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ No weapon equipped"));
        return false;
    }

    UCYAbilitySystemComponent* ASC = GetOwnerASC();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No AbilitySystemComponent found"));
        return false;
    }

    // 무기 공격 어빌리티 실행
    bool bSuccess = ASC->TryActivateAbilityByTag(CYGameplayTags::Ability_Combat_WeaponAttack);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon attack activated"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚔️ Weapon attack failed (cooldown or no ability)"));
    }
    
    return bSuccess;
}

bool UCYWeaponComponent::PerformLineTrace(FHitResult& OutHit, float Range)
{
    UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
    if (!Camera) return false;

    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + (Camera->GetForwardVector() * Range);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        OutHit, Start, End, ECC_Visibility, Params
    );
    
    if (bHit)
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 Line trace hit: %s at distance %.1f"), 
               *OutHit.GetActor()->GetName(), 
               FVector::Dist(Start, OutHit.Location));
    }
    
    return bHit;
}

UCYAbilitySystemComponent* UCYWeaponComponent::GetOwnerASC() const
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
        
        UE_LOG(LogTemp, Log, TEXT("🔗 Weapon attached to socket: %s"), *WeaponSocketName.ToString());
    }
}

void UCYWeaponComponent::OnRep_CurrentWeapon()
{
    OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    
    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
        UE_LOG(LogTemp, Log, TEXT("🔄 Client weapon replicated: %s"), *CurrentWeapon->ItemName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("🔄 Client weapon unequipped"));
    }
}