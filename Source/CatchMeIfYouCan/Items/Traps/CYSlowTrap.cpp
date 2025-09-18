#include "Items/Traps/CYSlowTrap.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"

ACYSlowTrap::ACYSlowTrap()
{
	ItemName = FText::FromString("Slow Trap");
	TrapType = ETrapType::Slow;
	TriggerRadius = 120.0f;
    
	// 슬로우 효과 추가
	TrapEffects.Add(UGE_SlowTrap::StaticClass());
    
	// 파란색으로 설정
	if (ItemMesh)
	{
		UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));
		if (CylinderMesh)
		{
			ItemMesh->SetStaticMesh(CylinderMesh);
			ItemMesh->SetWorldScale3D(FVector(0.8f, 0.8f, 0.1f)); // 넓고 얇게
            
			// 파란색 머티리얼 설정 (런타임에서)
			UMaterialInterface* Material = ItemMesh->GetMaterial(0);
			if (Material)
			{
				UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Blue);
					ItemMesh->SetMaterial(0, DynamicMaterial);
				}
			}
		}
	}
}