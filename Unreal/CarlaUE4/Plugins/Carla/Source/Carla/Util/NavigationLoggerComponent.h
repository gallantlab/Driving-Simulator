// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/NavigationPlayerState.h"
#include "ExperimentLoggerComponent.h"
#include "NavigationLoggerComponent.generated.h"


/**
 * Use for navigation experiments
 */
UCLASS()
class CARLA_API UNavigationLoggerComponent : public UExperimentLoggerComponent
{
	GENERATED_BODY()

public:

	virtual void LogFrame() override;

protected:
	ANavigationPlayerState *navigationPlayerState;
	
};
