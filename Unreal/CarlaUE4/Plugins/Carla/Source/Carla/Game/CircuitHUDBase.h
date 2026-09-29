// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaUMGBase.h"
#include "Vehicle/CircuitPlayerController.h"
#include "CircuitHUDBase.generated.h"

/**
 * For displaying stuff for driving down a circuit
 */
UCLASS()
class CARLA_API UCircuitHUDBase : public UCarlaUMGBase
{
	GENERATED_BODY()

	UCircuitHUDBase(const FObjectInitializer &objectInitializer);

	UFUNCTION(BlueprintPure, Category = "Circuit HUD")
	FString GetDisplayText() const ;

	UFUNCTION(BlueprintCallable, Category = "Circuit HUD")
	virtual void FindController() override;

	protected:
	ACircuitPlayerController* circuitPlayerController;
	
	
};
