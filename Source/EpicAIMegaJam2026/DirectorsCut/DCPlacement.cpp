#include "DCPlacement.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool DCPlacement::TraceGround(UWorld* World, FHitResult& Hit, const FVector& Start, const FVector& End, FCollisionQueryParams Params)
{
	if (!World) return false;
	while (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		const UPrimitiveComponent* Component = Hit.GetComponent();
		const AActor* Actor = Hit.GetActor();
		const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
		const FString MeshName = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : FString();
		const bool bVegetation = (Actor && Actor->ActorHasTag(TEXT("DCNoCameraGround")))
			|| (Component && Component->ComponentHasTag(TEXT("DCNoCameraGround")))
			|| MeshName.Contains(TEXT("Tree")) || MeshName.Contains(TEXT("Leaf"))
			|| MeshName.Contains(TEXT("Leaves")) || MeshName.Contains(TEXT("Foliage"));
		if (!bVegetation) return true;
		if (!Component) return false;
		Params.AddIgnoredComponent(Component);
	}
	return false;
}
