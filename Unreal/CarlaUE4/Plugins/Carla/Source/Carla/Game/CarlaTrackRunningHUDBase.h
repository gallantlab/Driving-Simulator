// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaUMGBase.h"
#include "Vehicle/TrackRunningController.h"

#include "CarlaTrackRunningHUDBase.generated.h"

/**
 * Base class for track running HUD blueprint
 */
UCLASS()
class CARLA_API UCarlaTrackRunningHUDBase : public UCarlaUMGBase
{
	GENERATED_BODY()
	
public:
	UCarlaTrackRunningHUDBase(const FObjectInitializer &objectInitializer);

	UFUNCTION(BlueprintPure, Category = "Track Running HUD")
	FString GetDisplayText() const ;

	UFUNCTION(BlueprintCallable, Category = "Track Running HUD")
	virtual void FindController() override;

protected:
	ATrackRunningController* trackRunningController;
	
};
