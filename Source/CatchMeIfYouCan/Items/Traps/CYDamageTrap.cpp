#include "Items/Traps/CYDamageTrap.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"

ACYDamageTrap::ACYDamageTrap()
{
	ItemName = FText::FromString("Damage Trap");
	TrapType = ETrapType::Damage;
	TriggerRadius = 90.0f;
    
	// 데미지 효과 추가
	TrapEffects.Add(UGE_WeaponDamage::StaticClass());
    
	// 빨간색 원뿔로 설정
	if (ItemMesh)
	{
		UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone"));
		if (ConeMesh)
		{
			ItemMesh->SetStaticMesh(ConeMesh);
			ItemMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.8f)); // 뾰족하게
            
			// 빨간색 머티리얼 설정
			UMaterialInterface* Material = ItemMesh->GetMaterial(0);
			if (Material)
			{
				UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Red);
					ItemMesh->SetMaterial(0, DynamicMaterial);
				}
			}
		}
	}
}