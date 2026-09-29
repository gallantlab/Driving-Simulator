// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/MRIPlayerState.h"
#include "NavigationPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ANavigationPlayerState : public AMRIPlayerState
{
	GENERATED_BODY()

public:
	ANavigationPlayerState();
	virtual void Reset() override;
	virtual void ResetExperimentState() override;
	virtual void CopyProperties(APlayerState *PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual int GetCurrentDestination() const
	{
		return currentDestination;
	}

	float GetSecondsBeforeFirstDestination() const
	{
		return secondsBeforeFirstDestination;
	}

	bool IsShowingHelp() const
	{
		return showingHelp;
	}

	bool IsLost() const
	{
		return isLost;
	}

	/**
	 * Current destination index.
	 * 0 and above are destination indices
	 * -1 is no current destination
	 * -2 is no current destination, and the previous segment ended because subject was lsot
	 */
	UPROPERTY(VisibleAnywhere, Replicated)
 	int currentDestination;


private:
	friend class ANavigationVehicleController;

	UPROPERTY(VisibleAnywhere, Replicated)
	float secondsBeforeFirstDestination = 0.0;	// number of seconds between the beginning of the demo and the first destination

	UPROPERTY(VisibleAnywhere, Replicated)
	bool isLost = false;	// did this trial end because the subject was lost?

	UPROPERTY(VisibleAnywhere, Replicated)
	bool showingHelp = false;	// is the learning HUD being currently shown as help?
};
