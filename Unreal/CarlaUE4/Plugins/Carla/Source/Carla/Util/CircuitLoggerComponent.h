// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Util/NavigationLoggerComponent.h"
#include "Game/CircuitPlayerState.h"
#include "CircuitLoggerComponent.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API UCircuitLoggerComponent : public UNavigationLoggerComponent
{
	GENERATED_BODY()
	
	UCircuitLoggerComponent();

public:
	virtual void LogFrame() override;

protected:
	ACircuitPlayerState *circuitPlayerState;
	
	
};
