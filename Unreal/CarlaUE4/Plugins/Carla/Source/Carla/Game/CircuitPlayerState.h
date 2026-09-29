// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/NavigationPlayerState.h"
#include "CircuitPlayerState.generated.h"


/// Player state relative to the circuit
enum class EPlayerCircuitState
{
	WrongDirection = -2,
	OffCourse = -1,
	NoCourse = 0,
	OnCourse = 1,
};


/**
 * Player state for closed loop circuit experiments
 *
 * 8/6/2020
 * Note that this class was initially written when the circuit was intended as
 * run-n-laps, varying circuit size. It was then changed to be a single circuit
 * but you drive along it to a destination, so it's like the regular taxi driver
 * experiment, but the path is restricted to the circuit. Hence there are properties
 * like nLapsDesired and nLapsCompleted, but they are not used anymore. Indeed,
 * the python logparser does not even make use of these values.
 */
UCLASS()
class CARLA_API ACircuitPlayerState : public ANavigationPlayerState
{
	GENERATED_BODY()

public:
	ACircuitPlayerState();

	virtual void Reset() override;

	virtual void ResetExperimentState() override;

	virtual void CopyProperties(APlayerState *playerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	int GetCurrentCircuit() const
	{
		return currentCircuit;
	}

	UFUNCTION(BlueprintCallable)
	int GetDesiredLaps() const
	{
		return nLapsDesired;
	}

	UFUNCTION(BlueprintCallable)
	int GetCompletedLaps() const
	{
		return nLapsCompleted;
	}

	UFUNCTION(BlueprintCallable)
	int GetIsOnCourse() const
	{
		return isOnCourse;
	}

private:
	friend class ACircuitPlayerController;

	UPROPERTY(VisibleAnywhere, Replicated)
	int currentCircuit = -1;	// -1 indicates not there is not currently a circuit, 0 for short, 1 for medium, 2 for long

	UPROPERTY(VisibleAnywhere, Replicated)
	int nLapsDesired = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	int nLapsCompleted = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	int isOnCourse = 0;			// pun from EPlayerCircuitState
};
