#include "CYPlayerCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "AbilitySystem/CYCombatGameplayTags.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYItemInteractionComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Input/CYInputComponent.h"
#include "Input/CYInputGameplayTags.h"
#include "Player/CYPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/CYItemBase.h"
#include "Items/CYWeaponBase.h"

ACYPlayerCharacter::ACYPlayerCharacter(const FObjectInitializer& ObjectInitializer) 
	:	Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(FName("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(FName("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

void ACYPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	APlayerController* OwningPlayerController = GetController<APlayerController>();
	if (OwningPlayerController && OwningPlayerController->GetLocalPlayer())
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(DefaultMappingContext);
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ACYPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	ACYPlayerState* PS = GetPlayerState<ACYPlayerState>();
	if (PS)
	{
		// 서버측에서 실행되는 ASC 캐싱 및 ASC 설정으로써 클라이언트는 OnRep_PlayerState()에서 해당 로직 진행
		CYAbilitySystemComponent = Cast<UCYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

		// 서버에서만 어빌리티 세트를 초기화
		InitializeAbilitySets();
	}
}

void ACYPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACYPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UCYInputComponent* CYInputComponent = Cast<UCYInputComponent>(PlayerInputComponent);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ACYPlayerCharacter::Input_Move, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ACYPlayerCharacter::Input_Look, false);

	TArray<uint32> BindHandles;
	CYInputComponent->BindAbilityActions(DefaultInputConfig, this, &ThisClass::Input_AbilityInputTagStarted, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

	// 아이템 상호작용 입력
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_Interact, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_Interact, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_Attack, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_Attack, false);
    
	// 인벤토리 슬롯 입력 (1~9번 키)
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot1, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot1, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot2, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot2, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot3, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot3, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot4, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot4, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot5, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot5, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot6, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot6, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot7, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot7, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot8, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot8, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot9, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot9, false);
	
}

void ACYPlayerCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	if (AController* LocalController = GetController())
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, LocalController->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void ACYPlayerCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		AddControllerPitchInput(Value.Y);
	}
}

void ACYPlayerCharacter::Input_AbilityInputTagStarted(FGameplayTag InputTag)
{
	if (!CYAbilitySystemComponent.IsValid())
	{
		return;
	}
	
	CYAbilitySystemComponent->AbilityInputTagStarted(InputTag);
}

void ACYPlayerCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (!CYAbilitySystemComponent.IsValid())
	{
		return;
	}

	CYAbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void ACYPlayerCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (!CYAbilitySystemComponent.IsValid())
	{
		return;
	}

	CYAbilitySystemComponent->AbilityInputTagReleased(InputTag);
}

void ACYPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	ACYPlayerState* PS = GetPlayerState<ACYPlayerState>();
	if (PS)
	{
		CYAbilitySystemComponent = Cast<UCYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}

// 아이템 상호작용 입력
void ACYPlayerCharacter::Input_Interact(const FInputActionValue& InputActionValue)
{
    if (ItemInteractionComponent)
    {
        ItemInteractionComponent->InteractWithNearbyItem();
    }
}

void ACYPlayerCharacter::Input_Attack(const FInputActionValue& InputActionValue)
{
	// 🔥 한 번의 좌클릭으로 하나의 동작만 실행
	
	// 1. 현재 들고 있는 아이템이 있으면 아이템 사용 (우선순위 높음)
	if (InventoryComponent && InventoryComponent->CurrentHeldItem)
	{
		bool bUsedItem = InventoryComponent->UseHeldItem();
		
		if (bUsedItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("🎯 Used held item: %s"), 
				   *InventoryComponent->CurrentHeldItem->ItemName.ToString());
		}
		return; // 🔥 아이템 사용했으면 여기서 종료
	}
	
	// 2. 무기가 장착되어 있으면 무기 공격
	if (WeaponComponent && WeaponComponent->CurrentWeapon)
	{
		bool bUsedWeapon = WeaponComponent->PerformAttack();
		
		if (bUsedWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚔️ Attacked with weapon: %s"), 
				   *WeaponComponent->CurrentWeapon->ItemName.ToString());
		}
		return; // 🔥 무기 공격했으면 여기서 종료
	}
	
	// 3. 둘 다 없으면 인벤토리 디버그 표시
	ShowInventoryDebug();
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			TEXT("No weapon equipped or item held - Press number keys to select"));
	}
}

void ACYPlayerCharacter::ShowInventoryDebug()
{
    if (InventoryComponent)
    {
        InventoryComponent->ShowInventoryDebug();
    }
}

// 인벤토리 슬롯 입력 (1~9번 키)
void ACYPlayerCharacter::Input_UseSlot1(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(1); // 무기 슬롯 1
}

void ACYPlayerCharacter::Input_UseSlot2(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(2); // 무기 슬롯 2
}

void ACYPlayerCharacter::Input_UseSlot3(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(3); // 무기 슬롯 3
}

void ACYPlayerCharacter::Input_UseSlot4(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(4); // 아이템 슬롯 1
}

void ACYPlayerCharacter::Input_UseSlot5(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(5); // 아이템 슬롯 2
}

void ACYPlayerCharacter::Input_UseSlot6(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(6); // 아이템 슬롯 3
}

void ACYPlayerCharacter::Input_UseSlot7(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(7); // 아이템 슬롯 4
}

void ACYPlayerCharacter::Input_UseSlot8(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(8); // 아이템 슬롯 5
}

void ACYPlayerCharacter::Input_UseSlot9(const FInputActionValue& InputActionValue)
{
	if (InventoryComponent) InventoryComponent->HoldItem(9); // 아이템 슬롯 6
}