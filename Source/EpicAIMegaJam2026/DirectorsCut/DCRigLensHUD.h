// Director's Cut prototype 5 - HUD: prototype 4's HUD plus a viewfinder, camera/lens readout, and the jib tool.

#pragma once

#include "CoreMinimal.h"
#include "DCReplicaHUD.h"
#include "DCRigLensHUD.generated.h"

class ADCRigCharacter;

UCLASS()
class ADCRigLensHUD : public ADCReplicaHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	virtual void DrawToolStrip(ADCReplicaDirector* D) override;
	virtual void DrawHelp(ADCReplicaDirector* D) override;

private:
	void DrawSkipButton(ADCReplicaDirector* D);
	void DrawViewfinder(ADCReplicaDirector* D);
	void DrawCameraPanel(ADCRigCharacter* P);
	void DrawLensChange(ADCRigCharacter* P);
	void DrawMeter(float X, float Y, float W, float Value01, const FLinearColor& Color, bool bSigned = false);
};
