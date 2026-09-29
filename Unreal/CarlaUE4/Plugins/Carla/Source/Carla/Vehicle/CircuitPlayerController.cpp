// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitPlayerController.h"

int CircuitStateToInt(EPlayerCircuitState state)
{
	switch (state)
	{
		case EPlayerCircuitState::WrongDirection :
			return -2;
		case EPlayerCircuitState::OffCourse :
			return -1;
		case EPlayerCircuitState::NoCourse :
			return 0;
		case EPlayerCircuitState::OnCourse :
			return 1;
		default:
			return -3;
	}
}

ACircuitPlayerController::ACircuitPlayerController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	UE_LOG(LogFMRI, Log, TEXT("Circuit controller spawned"))
}

void ACircuitPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CircuitPlayerState = Cast<ACircuitPlayerState>(CarlaPlayerState);
	if (!CircuitPlayerState)
		UE_LOG(LogFMRI, Error, TEXT("Player state is not a circuit player state!"));

	// Query for possible circuits
	for (TActorIterator<ACircuit> circuitIterator(GetWorld()); circuitIterator; ++circuitIterator)
		circuits.Add(*circuitIterator);
	currentCircuit = circuits[0];
	UE_LOG(LogFMRI, Log, TEXT("Found %d circuits"), circuits.Num());

	// check if circuit is directional
	bIsCircuitDirectional = gameInstance->GetCarlaSettings().bDirectionalCircuit;
}


void ACircuitPlayerController::ExperimentTick(float dTime)
{
	CheckIsOnCourse();
	Super::ExperimentTick(dTime);
}


void ACircuitPlayerController::Tick(float dTime)
{
	Super::Tick(dTime);

	if (showInstructions > 0) showInstructions -= dTime;
	if (showArrival > 0) showArrival -= dTime;
	if (showProgress > 0) showProgress -= dTime;
}


void ACircuitPlayerController::CheckIsOnCourse()
{
	textColor = EDisplayTextColor::White;
	if (currentCircuit)
	{
		isOnCourse = currentCircuit->Overlaps(GetPossessedVehicle());

		switch (isOnCourse)
		{
			case EPlayerCircuitState::WrongDirection :
				if (bIsCircuitDirectional)	// only needed if the circuit is directional
					textColor = EDisplayTextColor::Red;
				break;
			case EPlayerCircuitState::OffCourse :
				textColor = EDisplayTextColor::Red;
				break;
			default:
				break;
		}
	}
	else isOnCourse = EPlayerCircuitState::NoCourse;
	CircuitPlayerState->isOnCourse = CircuitStateToInt(isOnCourse);
}


void ACircuitPlayerController::SetIsDrivingOnCircuit(bool state)
{
	if (state)
	{
		bIsDrivingOnCircuit = true;
	}
	else
	{
		CircuitPlayerState->currentCircuit = -1;
	}
}

void ACircuitPlayerController::OnFixedRouteFinished()
{
	Super::OnFixedRouteFinished();
	if (gameInstance->GetDemoState() < 2)		// only set this if demo is running since we don't want to mess with reset stuff
	{
		secondsUntilNextInstruction = randomStream.FRandRange(2.0f, 8.0f);
		UE_LOG(LogFMRI, Log, TEXT("%f seconds until next trial"), secondsUntilNextInstruction);
	}
}


void ACircuitPlayerController::ConfigureNextDestination()
{
	Super::ConfigureNextDestination();
	currentCircuit = circuits[0];
	bIsTrialConfigured = true;
}


void ACircuitPlayerController::ResetExperimentState()
{
	Super::ResetExperimentState();
	bIsTrialConfigured = false;
	bIsDrivingOnCircuit = false;
	showArrival = 0;
	showInstructions = 0;
	secondsUntilNextInstruction = 0;
	nLapsDesired = 0;
	nLapsCompleted = 0;
	showProgress = 0.0;
	currentCircuit = nullptr;
}


FString ACircuitPlayerController::GetDisplayText() const
{
	FString superResult = Super::GetDisplayText();
	if (superResult != EMPTY_STRING)
		return superResult;
	switch (isOnCourse)
	{
		case EPlayerCircuitState::OffCourse:
			return OFF_COURSE;
		case EPlayerCircuitState::WrongDirection:
			if (bIsCircuitDirectional)
				return WRONG_DIRECTION;
		default:
			return superResult;
	}
}


void ACircuitPlayerController::PickNextDestination(int minIndex, int maxIndex)
{
	UE_LOG(LogFMRI, Log, TEXT("Generating next destination on circuit. Current destination is %d"), currentDestination)
	UE_LOG(LogFMRI, Log, TEXT(" Min index %d max index %d "), destinations->GetMinIndex(), destinations->GetMaxIndex())
	// pick a destination that is 1/4 - 3/4 the way around the track
	int nextDestination = randomStream.RandRange(currentDestination + destinations->Num() * gameInstance->GetCarlaSettings().CircuitMinDestinationFraction,
												 currentDestination + destinations->Num() * gameInstance->GetCarlaSettings().CircuitMaxDestinationFraction);
	SetCurrentDestination(nextDestination % destinations->Num());
}


void ACircuitPlayerController::SetCurrentDestination(int newDestination, bool markerOff)
{
	UE_LOG(LogFMRI, Log, TEXT("Circuit Set destination value as %d"), newDestination);
	Super::SetCurrentDestination(newDestination, markerOff);
	if (newDestination > -1)
	{
		currentDestination = newDestination;
	}
}