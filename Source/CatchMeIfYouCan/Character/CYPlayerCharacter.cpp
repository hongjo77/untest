#include "CYPlayerCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystem/CYAbilitySystemComponent.h"
#include "Components/Items/CYInventoryComponent.h"
#include "Components/Items/CYWeaponComponent.h"
#include "Input/CYInputComponent.h"
#include "Input/CYInputGameplayTags.h"
#include "Player/CYPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/CYItemInputTags.h"
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

	// 아이템 입력 추가
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_Interact, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_Interact, false);
	
	// 인벤토리 슬롯
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot1, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot1, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot2, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot2, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot3, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot3, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot4, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot4, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot5, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot5, false);
	CYInputComponent->BindNativeAction(DefaultInputConfig, CYGameplayTags::InputTag_UseSlot6, ETriggerEvent::Started, this, &ACYPlayerCharacter::Input_UseSlot6, false);

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

	// WeaponAttack 어빌리티인 경우 인벤토리 표시
	if (InputTag.MatchesTagExact(CYGameplayTags::InputTag_Ability_WeaponAttack))
	{
		UE_LOG(LogTemp, Warning, TEXT("🎮 WeaponAttack ability triggered"));
        
		// 로컬 컨트롤러에서만 인벤토리 표시
		if (IsLocallyControlled())
		{
			DisplayInventoryStatus();
		}
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

void ACYPlayerCharacter::Input_Interact(const FInputActionValue& InputActionValue)
{
	InteractPressed();
}

void ACYPlayerCharacter::Input_UseSlot1(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(1000); // 무기 슬롯 1
}

void ACYPlayerCharacter::Input_UseSlot2(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(1001); // 무기 슬롯 2
}

void ACYPlayerCharacter::Input_UseSlot3(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(1002); // 무기 슬롯 3
}

void ACYPlayerCharacter::Input_UseSlot4(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(0); // 아이템 슬롯 1
}

void ACYPlayerCharacter::Input_UseSlot5(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(1); // 아이템 슬롯 2
}

void ACYPlayerCharacter::Input_UseSlot6(const FInputActionValue& InputActionValue)
{
	UseInventorySlot(2); // 아이템 슬롯 3
}

void ACYPlayerCharacter::DisplayInventoryStatus()
{
    if (!GEngine) return;

    // ✅ 인벤토리 컴포넌트 확인
    if (!InventoryComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No InventoryComponent found"));
        return;
    }

    // 기존 메시지 제거
    GEngine->ClearOnScreenDebugMessages();

    // 인벤토리 상태 표시
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("=== 📦 INVENTORY STATUS ==="));
    
    // 무기 슬롯 (1~3번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🗡️ WEAPONS (Keys 1-3):"));
    for (int32 i = 0; i < InventoryComponent->WeaponSlots.Num(); ++i)
    {
        FString WeaponInfo;
        if (InventoryComponent->WeaponSlots[i])
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 1, 
                *InventoryComponent->WeaponSlots[i]->ItemName.ToString(), 
                InventoryComponent->WeaponSlots[i]->ItemCount
            );
            
            // ✅ testun 방식: 안전한 비교
            if (WeaponComponent && WeaponComponent->CurrentWeapon && 
                WeaponComponent->CurrentWeapon == InventoryComponent->WeaponSlots[i])
            {
                WeaponInfo += TEXT(" ⭐ EQUIPPED");
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, WeaponInfo);
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, WeaponInfo);
            }
        }
        else
        {
            WeaponInfo = FString::Printf(TEXT("  [%d] Empty"), i + 1);
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Silver, WeaponInfo);
        }
    }
    
    // 아이템 슬롯 (4~9번 키)
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("🎒 ITEMS (Keys 4-9):"));
    int32 MaxDisplayItems = FMath::Min(6, InventoryComponent->ItemSlots.Num());
    for (int32 i = 0; i < MaxDisplayItems; ++i)
    {
        FString ItemInfo;
        if (InventoryComponent->ItemSlots[i])
        {
            ItemInfo = FString::Printf(TEXT("  [%d] %s x%d"), 
                i + 4, 
                *InventoryComponent->ItemSlots[i]->ItemName.ToString(), 
                InventoryComponent->ItemSlots[i]->ItemCount
            );
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, ItemInfo);
        }
        else
        {
            ItemInfo = FString::Printf(TEXT("  [%d] Empty"), i + 4);
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Silver, ItemInfo);
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("=================="));
}
