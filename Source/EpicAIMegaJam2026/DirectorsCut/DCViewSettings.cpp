#include "DCViewSettings.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/Interface_PostProcessVolume.h"

namespace
{
	/** Copies every setting Src overrides onto Dst (and marks it overridden there), like a volume at full weight. */
	void ApplyOverrides(const FPostProcessSettings& Src, FPostProcessSettings& Dst)
	{
		const UScriptStruct* Struct = FPostProcessSettings::StaticStruct();
		for (TFieldIterator<FBoolProperty> It(Struct); It; ++It)
		{
			const FString Flag = It->GetName();
			if (!Flag.StartsWith(TEXT("bOverride_")) || !It->GetPropertyValue_InContainer(&Src)) continue;
			const FString Name = Flag.RightChop(10);
			const FProperty* Value = Struct->FindPropertyByName(FName(*Name));
			Value = Value ? Value : Struct->FindPropertyByName(FName(TEXT("b") + Name));
			if (!Value) continue;
			It->SetPropertyValue_InContainer(&Dst, true);
			Value->CopyCompleteValue_InContainer(&Dst, &Src);
		}
		for (const FWeightedBlendable& Blendable : Src.WeightedBlendables.Array)
		{
			if (Blendable.Object && Blendable.Weight > 0.f) Dst.WeightedBlendables.Array.Add(Blendable);
		}
	}
}

FPostProcessSettings DCView::MainViewSettings(UWorld* World, const FVector& Location, const FPostProcessSettings& CameraSettings)
{
	FPostProcessSettings PP;
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = (EDynamicGlobalIlluminationMethod::Type)
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"))->GetInt();
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = (EReflectionMethod::Type)IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"))->GetInt();
	PP.bOverride_LumenSurfaceCacheResolution = true; // captures default to half resolution
	PP.LumenSurfaceCacheResolution = 1.f;
	if (World)
	{
		for (IInterface_PostProcessVolume& Volume : World->GetPostProcessVolumeIterator()) // ascending priority
		{
			const FPostProcessVolumeProperties Props = Volume.GetProperties();
			if (Props.bIsEnabled && Props.Settings && Props.BlendWeight > 0.f
				&& (Props.bIsUnbound || Volume.EncompassesPoint(Location, 0.f, nullptr)))
			{
				ApplyOverrides(*Props.Settings, PP);
			}
		}
	}
	ApplyOverrides(CameraSettings, PP);
	return PP;
}
