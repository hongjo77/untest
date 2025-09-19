#include "CYCharacterBase.h"
#include "CYLogChannels.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "AbilitySystem/Attributes/CYCombatAttributeSet.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYItemInteractionComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ACYCharacterBase::ACYCharacterBase(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Item 컴포넌트들 생성
	InventoryComponent = CreateDefaultSubobject<UCYInventoryComponent>(TEXT("InventoryComponent"));
	ItemInteractionComponent = CreateDefaultSubobject<UCYItemInteractionComponent>(TEXT("ItemInteractionComponent"));
	WeaponComponent = CreateDefaultSubobject<UCYWeaponComponent>(TEXT("WeaponComponent"));

}

UAbilitySystemComponent* ACYCharacterBase::GetAbilitySystemComponent() const
{
	return CYAbilitySystemComponent.Get();
}

void ACYCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ACYCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAbilitySets();
	Super::EndPlay(EndPlayReason);
}

void ACYCharacterBase::InitializeAbilitySets()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!CYAbilitySystemComponent.IsValid())
	{
		UE_LOG(LogCY, Warning, TEXT("%s Side: InitializeAbilitySets: AbilitySystemComponent is invalid on %s"), *GetClientServerContextString(this), *GetNameSafe(this));
		return;
	}

	// 기존 Ability 제거
	RemoveAbilitySets();

	UCYAbilitySystemComponent* CYASC = CYAbilitySystemComponent.Get();
	
	// 기본 AbilitySet 부여
	for (UCYAbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (IsValid(AbilitySet))
		{
			FCYAbilitySet_GrantedHandles& GrantedHandles = GrantedAbilitySetHandles.AddDefaulted_GetRef();
			AbilitySet->GiveToAbilitySystem(CYASC, &GrantedHandles);
		}
	}
	
	// 검증
	const UCYCombatAttributeSet* FinalCheck = CYASC->GetSet<UCYCombatAttributeSet>();
	if (FinalCheck)
	{
		UE_LOG(LogCY, Warning, TEXT("✅ %s has UCYCombatAttributeSet (MoveSpeed: %f)"), 
			   *GetName(), FinalCheck->GetMoveSpeed());
	}
	else
	{
		UE_LOG(LogCY, Error, TEXT("❌ %s still missing UCYCombatAttributeSet!"), *GetName());
	}
}

void ACYCharacterBase::RemoveAbilitySets()
{
	// 서버에서만 제거
	if (!HasAuthority())
	{
		return;
	}
	
	if (!CYAbilitySystemComponent.IsValid())
	{
		return;
	}

	UCYAbilitySystemComponent* CYASC = CYAbilitySystemComponent.Get();

	// 모든 부여된 능력 제거
	for (FCYAbilitySet_GrantedHandles& GrantedHandles : GrantedAbilitySetHandles)
	{
		GrantedHandles.TakeFromAbilitySystem(CYASC);
	}
	GrantedAbilitySetHandles.Empty();
}

void ACYCharacterBase::InteractPressed()
{
	if (ItemInteractionComponent)
	{
		ItemInteractionComponent->InteractWithNearbyItem();
	}
}

void ACYCharacterBase::AttackPressed()
{
	// 🔥 CatchMe 방식: WeaponComponent에서 모든 공격 로직 처리
	if (WeaponComponent)
	{
		WeaponComponent->PerformAttack();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No WeaponComponent found"));
	}
}

void ACYCharacterBase::UseInventorySlot(int32 SlotIndex)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseItem(SlotIndex);
	}
}