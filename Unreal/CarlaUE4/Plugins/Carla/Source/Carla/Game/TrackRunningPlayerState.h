// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/NavigationPlayerState.h"
#include "TrackRunningPlayerState.generated.h"

/**
 * For storing information about track running with variable traffic
 */
UCLASS()
class CARLA_API ATrackRunningPlayerState : public ANavigationPlayerState
{
	GENERATED_BODY()

public:
	ATrackRunningPlayerState();

	virtual void Reset() override;

	virtual void ResetExperimentState() override;

	virtual void CopyProperties(APlayerState *playerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	int GetNumberOfVehicles() const
	{
		return numberOfVehicles;
	}

	UFUNCTION(BlueprintCallable)
	bool IsPlayerRunningOnTrack() const
	{
		return bIsRunningOnTrack;
	}

	UFUNCTION(BlueprintCallable)
	int GetTrackRunningDirection() const
	{
		return trackRunningDirection;
	}

private:

	friend class ATrackRunningController;

	// traffic needs to be logged since it is now variable
	UPROPERTY(VisibleAnywhere, Replicated)
	int numberOfVehicles = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool bIsRunningOnTrack = false;			// has the subject been instructed to run down the track?

	UPROPERTY(VisibleAnywhere, Replicated)
	int trackRunningDirection = 0;			// What direction is the subject going? 0 for not running, 1 up, 2 down
};
