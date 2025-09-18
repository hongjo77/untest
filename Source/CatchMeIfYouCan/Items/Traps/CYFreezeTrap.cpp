#include "Items/Traps/CYFreezeTrap.h"
#include "AbilitySystem/Effects/CYCombatGameplayEffects.h"

ACYFreezeTrap::ACYFreezeTrap()
{
	ItemName = FText::FromString("Freeze Trap");
	TrapType = ETrapType::Freeze;
	TriggerRadius = 100.0f;
    
	// 프리즈 효과 추가
	TrapEffects.Add(UGE_ImmobilizeTrap::StaticClass());
    
	// 청록색 큐브로 설정
	if (ItemMesh)
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
		if (CubeMesh)
		{
			ItemMesh->SetStaticMesh(CubeMesh);
			ItemMesh->SetWorldScale3D(FVector(0.8f, 0.8f, 0.3f)); // 큐브 모양
            
			// 청록색 머티리얼 설정
			UMaterialInterface* Material = ItemMesh->GetMaterial(0);
			if (Material)
			{
				UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Green);
					ItemMesh->SetMaterial(0, DynamicMaterial);
				}
			}
		}
	}
}