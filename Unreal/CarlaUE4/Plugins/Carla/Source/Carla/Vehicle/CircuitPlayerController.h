// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Util/Circuit.h"
#include "Game/CircuitPlayerState.h"
#include "CircuitPlayerController.generated.h"

const FString OFF_COURSE = FString("off course");
const FString WRONG_DIRECTION = FString("wrong direction");
const FString LONG_CIRCUIT = FString("long circuit");
const FString MEDIUM_CIRCUIT = FString("medium circuit");
const FString SHORT_CIRCUIT = FString("short circuit");
const FString ONE_LAP = FString(" 1 lap");
const FString TWO_LAPS = FString(" 2 laps");
const FString THREE_LAPS = FString(" 3 laps");
const FString TWO = FString("2 of");
const FString THREE = FString("3 of");
const FString FINISHED = FString("Trial finished");
const FString BAD = FString("Bad");

/**
 * Used for circuit/closed-loop experiments
 */
UCLASS()
class CARLA_API ACircuitPlayerController : public ANavigationVehicleController
{
	GENERATED_BODY()

public:
	ACircuitPlayerController(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;

	virtual void ExperimentTick(float dTime) override;

	virtual void Tick(float dTime) override;

	virtual void OnFixedRouteFinished() override;

	void SetIsDrivingOnCircuit(bool state);

	bool ShowInstructions() const {return showInstructions > 0;};

	bool ShowProgress() const {return showProgress > 0;}

	bool ShowFinish() const {return showArrival > 0;};

	bool IsOnCourse() const {return isOnCourse > EPlayerCircuitState::OffCourse; };

	ECircuitLength GetCircuitLength () const ;

	int GetLapsDesired() const {return nLapsDesired;};

	int GetLapsCompleted() const {return nLapsCompleted;};

	virtual FString GetDisplayText() const override;

protected:
	// sets traffic conditions, etc, for next trial
	void ConfigureNextDestination() override;

	virtual void PickNextDestination(int minIndex = 0, int maxIndex = -1) override;

	virtual void ResetExperimentState() override;

	void CheckIsOnCourse();

	int IncrementLapsCompleted();

	void SetLapsCompleted(int);

	void SetLapsDesired(int);

	void SetCurrentDestination(int newDestination, bool markerOff = true) override;

private:
	bool bIsDrivingOnCircuit = false;
	EPlayerCircuitState isOnCourse = EPlayerCircuitState::NoCourse;
	int nLapsCompleted = 0;
	int nLapsDesired = 0;
	float secondsUntilNextInstruction = 0.0f;

	ACircuitPlayerState *CircuitPlayerState;

	float showInstructions = 0.0;
	float showArrival = 0.0;
	float showProgress = 0.0;
	bool bIsTrialConfigured = false;

	TArray<ACircuit*> circuits;

	ACircuit* currentCircuit = nullptr;

	/**
	 * Persists until the beginning of the next trial, unlike the one in the NavigationPlayerState,
	 * which persists only until the end of the current trial and is -1 between trials.
	 * So this is only -1 at the beginning of a run
	 */
	int currentDestination = -1;

	/**
	 * If false, will not display "wrong way" text
	 */
	bool bIsCircuitDirectional = true;
};
