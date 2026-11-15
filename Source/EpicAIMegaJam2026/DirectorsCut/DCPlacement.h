#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"

class UWorld;

namespace DCPlacement
{
	// Trees remain obstacles, but their trunks/canopies cannot support a camera or hint circle.
	bool TraceGround(UWorld* World, FHitResult& Hit, const FVector& Start, const FVector& End, FCollisionQueryParams Params);
}
