// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/CarlaPlayerState.h"
#include "Game/CarlaGameInstance.h"
#include "ExperimentLoggerComponent.generated.h"

class ACarlaSpectatorController;

/**
 * An actor component used by the spectator controller to log experiments
 * Subclass this for particular experiments
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARLA_API UExperimentLoggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UExperimentLoggerComponent();

	// to be called on every tick by the spectator controller to log information
	virtual void LogFrame();

	void SetParentController(ACarlaSpectatorController *parent);

protected:
	ACarlaSpectatorController *spectatorController = nullptr;
	ACarlaPlayerState *playerState = nullptr;
	UCarlaGameInstance *gameInstance = nullptr;
};
