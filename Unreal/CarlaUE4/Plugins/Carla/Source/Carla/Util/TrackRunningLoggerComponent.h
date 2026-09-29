// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/TrackRunningPlayerState.h"
#include "NavigationLoggerComponent.h"
#include "TrackRunningLoggerComponent.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API UTrackRunningLoggerComponent : public UNavigationLoggerComponent
{
	GENERATED_BODY()

	UTrackRunningLoggerComponent();
	
public:
	virtual void LogFrame() override;

protected:
	ATrackRunningPlayerState *trackRunningPlayerState;
	
	
};
