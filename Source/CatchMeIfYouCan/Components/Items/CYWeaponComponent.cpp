// CYWeaponComponent.cpp - CatchMe 방식으로 실제 공격 로직 포함
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
#include "Engine/Engine.h"

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
    
	// 🔥 무기를 보이게 설정 (픽업 상태에서 장착 상태로)
	Weapon->SetActorHiddenInGame(false);
    
	// 충돌 비활성화 (장착된 무기는 월드와 충돌하지 않음)
	if (Weapon->ItemMesh)
	{
		Weapon->ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (Weapon->InteractionSphere)
	{
		Weapon->InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
    
	OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    
	UE_LOG(LogTemp, Warning, TEXT("Weapon equipped: %s"), *Weapon->ItemName.ToString());
	return true;
}

bool UCYWeaponComponent::UnequipWeapon()
{
	if (!CurrentWeapon || !GetOwner()->HasAuthority()) return false;

	ACYWeaponBase* OldWeapon = CurrentWeapon;
    
	// 🔥 무기를 숨기기 (장착 해제 시 인벤토리에 있으므로)
	OldWeapon->SetActorHiddenInGame(true);
    
	// 부착 해제
	OldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon = nullptr;

	OnWeaponChanged.Broadcast(OldWeapon, nullptr);
    
	UE_LOG(LogTemp, Warning, TEXT("Weapon unequipped: %s"), *OldWeapon->ItemName.ToString());
	return true;
}

// 🔥 핵심: CatchMe 방식으로 실제 공격 로직 포함
bool UCYWeaponComponent::PerformAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("PerformAttack called - HasAuthority: %s"), 
		   GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));
    
	// 🔥 무기가 없으면 바로 실패 반환 (공격 로직 실행 안 함)
	if (!CurrentWeapon) 
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformAttack: No weapon equipped"));
		return false;
	}
    
	// 서버에서만 실행
	if (!GetOwner()->HasAuthority()) 
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformAttack: Not authority, returning false"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("PerformAttack: CurrentWeapon found: %s"), 
		   *CurrentWeapon->ItemName.ToString());
	return ExecuteWeaponAttack();
}

bool UCYWeaponComponent::ExecuteWeaponAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("ExecuteWeaponAttack called"));
    
    if (!CurrentWeapon) 
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWeaponAttack: CurrentWeapon is null"));
        return false;
    }

    UCYAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
    if (!ASC) 
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWeaponAttack: AbilitySystemComponent is null"));
        return false;
    }

    // 🔥 안전한 태그 가져오기 (CatchMe 방식)
    FGameplayTag WeaponAttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.Combat.WeaponAttack"));
    
    if (!WeaponAttackTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Weapon attack tag is invalid! Tag: %s"), *WeaponAttackTag.ToString());
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Using tag: %s"), *WeaponAttackTag.ToString());
    
    // 🔥 CatchMe 방식: 중복 실행 방지
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(WeaponAttackTag);
    
    TArray<FGameplayAbilitySpec*> ActivatableAbilities;
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, ActivatableAbilities);
    
    UE_LOG(LogTemp, Warning, TEXT("Found %d activatable abilities"), ActivatableAbilities.Num());
    
    // 첫 번째 어빌리티만 실행
    if (ActivatableAbilities.Num() > 0)
    {
        FGameplayAbilitySpec* FirstAbility = ActivatableAbilities[0];
        if (FirstAbility && FirstAbility->Handle.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Executing FIRST ability only: %s"), 
                   FirstAbility->Ability ? *FirstAbility->Ability->GetName() : TEXT("NULL"));
            
            bool bResult = ASC->TryActivateAbility(FirstAbility->Handle);
            UE_LOG(LogTemp, Warning, TEXT("Weapon attack result: %s"), bResult ? TEXT("Success") : TEXT("Failed"));
            
            return bResult;
        }
    }
    
    // 백업: 일반적인 태그 활성화
    UE_LOG(LogTemp, Warning, TEXT("No valid ability spec found, trying fallback"));
    bool bFallbackResult = ASC->TryActivateAbilityByTag(WeaponAttackTag);
    UE_LOG(LogTemp, Warning, TEXT("Fallback result: %s"), bFallbackResult ? TEXT("Success") : TEXT("Failed"));
    
    return bFallbackResult;
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
        UE_LOG(LogTemp, Log, TEXT("Line trace hit: %s at distance %.1f"), 
               *OutHit.GetActor()->GetName(), 
               FVector::Dist(Start, OutHit.Location));
    }
    
    return bHit;
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
        
        UE_LOG(LogTemp, Log, TEXT("Weapon attached to socket: %s"), *WeaponSocketName.ToString());
    }
}

void UCYWeaponComponent::OnRep_CurrentWeapon()
{
	OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
    
	if (CurrentWeapon)
	{
		AttachWeaponToOwner(CurrentWeapon);
		CurrentWeapon->SetActorHiddenInGame(false);  // 🔥 클라이언트에서도 보이게
		UE_LOG(LogTemp, Log, TEXT("Client weapon replicated: %s"), *CurrentWeapon->ItemName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Client weapon unequipped"));
	}
}