// Director's Cut prototype 5 - game mode for "Rigs & Lenses" (L_DC_RigsLenses).
// Same scene as prototype 4; swaps in the rig/lens camera operator, director, and HUD.

#pragma once

#include "CoreMinimal.h"
#include "DCReplicaGameMode.h"
#include "DCRigLensGameMode.generated.h"

UCLASS()
class ADCRigLensGameMode : public ADCReplicaGameMode
{
	GENERATED_BODY()

public:
	ADCRigLensGameMode();

	virtual void StartPlay() override;
};
