#include "CYCharacterBase.h"
#include "CYLogChannels.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
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
	
	// 기본적으로 부여할 AbilitySet 설정
	for (UCYAbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (IsValid(AbilitySet))
		{
			// 해당 시점에서 AbilitySet을 부여하고 부여된 Ability들의 Handle을 GrantedAbilitySetHandles에 저장
			FCYAbilitySet_GrantedHandles& GrantedHandles = GrantedAbilitySetHandles.AddDefaulted_GetRef();
			AbilitySet->GiveToAbilitySystem(CYASC, &GrantedHandles);
		}
	}

	// Item 트랩에서 Move 관련 확인
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			if (IsValid(this) && CYAbilitySystemComponent.IsValid())
			{
				UCYAbilitySystemComponent* CYASC = CYAbilitySystemComponent.Get();
				const UAttributeSet* ConstAttrSet = CYASC->GetAttributeSet(UCYCombatAttributeSet::StaticClass());
				if (UCYCombatAttributeSet* CombatAttrSet = const_cast<UCYCombatAttributeSet*>(Cast<UCYCombatAttributeSet>(ConstAttrSet)))
				{
					float CurrentMoveSpeed = CombatAttrSet->GetMoveSpeed();
					UE_LOG(LogTemp, Warning, TEXT("🏃 %s initial MoveSpeed: %f"), 
						   *GetName(), CurrentMoveSpeed);
                    
					// CharacterMovementComponent와 동기화 확인
					if (ACharacter* Character = Cast<ACharacter>(this))
					{
						if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
						{
							UE_LOG(LogTemp, Warning, TEXT("🏃 %s CharacterMovement MaxWalkSpeed: %f"), 
								   *GetName(), MovementComp->MaxWalkSpeed);
                            
							// 동기화가 안 되어 있다면 강제 동기화
							if (FMath::Abs(CurrentMoveSpeed - MovementComp->MaxWalkSpeed) > 10.0f)
							{
								UE_LOG(LogTemp, Warning, TEXT("⚠️ Initial MoveSpeed desync, forcing sync..."));
								CombatAttrSet->HandleMoveSpeedChange();
							}
						}
					}
				}
			}
		});
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
	if (WeaponComponent)
	{
		WeaponComponent->PerformAttack();
	}
}

void ACYCharacterBase::UseInventorySlot(int32 SlotIndex)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseItem(SlotIndex);
	}
}
