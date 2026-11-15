#pragma once

#include "CoreMinimal.h"
#include "Engine/Scene.h"

class UWorld;

namespace DCView
{
	/**
	 * Scene captures ignore post-process volumes - they render with their own settings only, and switch Lumen off unless
	 * told otherwise - while the main view blends every volume it is in. Build the settings the main view would have at
	 * Location (project lighting defaults, then each volume by priority, then the camera's own), so a capture looks like
	 * the main screen: same grade, exposure, post-process materials and lighting.
	 */
	FPostProcessSettings MainViewSettings(UWorld* World, const FVector& Location,
		const FPostProcessSettings& CameraSettings = FPostProcessSettings());
}
