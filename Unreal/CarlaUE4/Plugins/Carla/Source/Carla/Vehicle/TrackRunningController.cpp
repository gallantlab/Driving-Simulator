// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "TrackRunningController.h"

ATrackRunningController::ATrackRunningController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	UE_LOG(LogFMRI, Log, TEXT("Track running controller spawned"))
}

void ATrackRunningController::BeginPlay()
{
	Super::BeginPlay();
	trackRunningPlayerState = Cast<ATrackRunningPlayerState>(CarlaPlayerState);
	if (!trackRunningPlayerState)
		UE_LOG(LogFMRI, Error, TEXT("Player state is not a track running player state!"));

	// Needed because HumanStartZone set the direction on begin overlap, and when the player vehicle
	// spawns, it does not yet have a controller on which set the direction, so the controller
	// needs to check when it assumes control
	TArray<AActor*> humanStartZones = TArray<AActor*>();
	GetPossessedVehicle()->GetOverlappingActors(humanStartZones, AHumanStartZone::StaticClass());
	if (humanStartZones.Num() != 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Vehicle not overlapping a human start zone!"))
	}
	else
	{
		SetTrackRunningDirection(Cast<AHumanStartZone>(humanStartZones[0])->runningDirection);
	}
	// override settings config
	randomDestinations = true;

	if (!destinations->IsIndexed())
	{
		UE_LOG(LogFMRI, Error, TEXT("Destinations are not indexed! Behaviour will be undefined."));
	}

	ConfigureNextTrial();
}

void ATrackRunningController::ExperimentTick(float dTime)
{
	if (!bIsRunningDownTrack)		// not moving to a destination
	{
		if (secondsUntilNextDestination <= 0)					// is generating time
		{
			ConfigureNextDestination();
			SetDisplayText(GOTO + GetCurrentDestinationName(), EDisplayedPromptType::GoTo);	// display destination for two seconds
		}
		else secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else
	{
		if (CheckForArrival())		// close enough to destination and is stopped
		{
			UE_LOG(LogFMRI, Log, TEXT("Arrived at %s"), **destinationName);
			SetDisplayText(ARRIVEDAT + GetCurrentDestinationName(), EDisplayedPromptType::Arrive);// display arrival message for two seconds
			SetCurrentDestination(-1);
			if (!bRelinquishControlOnStop)					// destination to be configured after car finishes turning around
				secondsUntilNextDestination = randomStream.FRandRange(4.0f, 12.0f);
			UE_LOG(LogFMRI, Log, TEXT("%f seconds until next destination"), secondsUntilNextDestination);
			if (!randomDestinations)
			{
				gameInstance->IncrementIndexInNavigationSequence();
				gameInstance->SaveSubjectStateFile();
			}
		}
	}
}

void ATrackRunningController::ConfigureNextTrial()
{
//	int nextTraffic;
//	do	// don't want same traffic condition to repeat back-to-back
//	{
//		nextTraffic = randomStream.RandHelper(3);
//	} while (nextTraffic == currentTraffic);
//	currentTraffic = nextTraffic;
//	UE_LOG(LogFMRI, Log, TEXT("Next traffic density is %d"), currentTraffic);
	trackRunningPlayerState->numberOfVehicles = GetGameInstance()->SetTrafficDensity(static_cast<ETrafficDensity>(currentTraffic));
	bIsTrialConfigured = true;
}


void ATrackRunningController::SetIsRunningDownTrack(bool state)
{
	bIsRunningDownTrack = state;
	trackRunningPlayerState->bIsRunningOnTrack = state;
}


void ATrackRunningController::SetFixedRoute(const TArray<FVector> &Locations, bool bOverwriteCurrent,
											int newDirection)
{
	if (newDirection == currentDirection)
	{
		UE_LOG(LogFMRI, Log, TEXT("New direction is same as current direction"))
		return;
	}

	if (newDirection != 0)
	{
		Super::SetFixedRoute(Locations, bOverwriteCurrent, newDirection);
		if (!bIsRunningDownTrack) // if the subject has already arrived, immediately take over player control
			RelinquishPlayerControl();
		bIsTrialConfigured = false;
		if (gameInstance->GetDemoState() < 2)		// only set this if demo is running since we don't want to mess with reset stuff
		{
			secondsUntilNextDestination = 100;		// because otherwise this is still <= 0
			UE_LOG(LogFMRI, Log, TEXT("Is end of track and turning around"))
		}
	}
}


void ATrackRunningController::PickNextDestination(int minIndex, int maxIndex)
{
	UE_LOG(LogFMRI, Log, TEXT("Generating next destination on track. Current destination is %d"), currentDestination)
	UE_LOG(LogFMRI, Log, TEXT(" Min index %d max index %d "), destinations->GetMinIndex(), destinations->GetMaxIndex())
	if (currentDestination > destinations->GetMinIndex() && currentDestination < destinations->GetMaxIndex() /*in the middle of the track*/)
	{	// don't allow turn-arounds

		UE_LOG(LogFMRI, Log, TEXT("Is in middle of track"))
		if (currentDirection == 1)	// moving towards higher indices
		{
			if (currentDestination >= destinations->GetMaxIndex() - 3) // too close to ends so just go there.
			{
				UE_LOG(LogFMRI, Log, TEXT("Sending to max index %d"), destinations->GetMaxIndex())
				SetCurrentDestination(destinations->GetMaxIndex());
			}
			else
			{
				UE_LOG(LogFMRI, Log, TEXT("Sending towards higher indices"))
				Super::PickNextDestination(currentDestination, maxIndex);
			}
		}
		else // (currentDirection == -1) // moving towards lower indices. there should only be +/-1 for this
		{
			if (currentDestination <= destinations->GetMinIndex() + 3)
			{
				UE_LOG(LogFMRI, Log, TEXT("Sending to min index %d"), destinations->GetMinIndex())
				SetCurrentDestination(destinations->GetMinIndex());
			}
			else
			{
				UE_LOG(LogFMRI, Log, TEXT("Sending towards lower indices"))
				Super::PickNextDestination(minIndex, currentDestination);
			}
		}
	}
	else
	{
		// use whatever randomness the nav controller does
		Super::PickNextDestination(minIndex, maxIndex);
	}
}


void ATrackRunningController::SetCurrentDestination(int newDestination, bool markerOff)
{
	Super::SetCurrentDestination(newDestination, markerOff);
	if (newDestination > -1)
	{
		currentDestination = newDestination;
		SetIsRunningDownTrack(true);
	}
	else SetIsRunningDownTrack(false);
}


void ATrackRunningController::OnFixedRouteFinished()
{
	Super::OnFixedRouteFinished();
	if (gameInstance->GetDemoState() < 2)	// only set this if demo is running since we don't want to mess with reset stuff
	{
		secondsUntilNextDestination = randomStream.FRandRange(0.0f, 4.0f);
		UE_LOG(LogFMRI, Log, TEXT("%f seconds until next trial"), secondsUntilNextDestination);
	}
}


void ATrackRunningController::SetTrackRunningDirection(int direction)
{
	currentDirection = direction;
	trackRunningPlayerState->trackRunningDirection = direction;
	UE_LOG(LogFMRI, Log, TEXT("Running direction set to %d"), direction);
}


ETrafficDensity ATrackRunningController::GetTrafficDensity() const
{
	return gameInstance->GetTrafficDensity();
}


void ATrackRunningController::ResetExperimentState()
{
	Super::ResetExperimentState();
	bIsTrialConfigured = false;
	bIsRunningDownTrack = false;
	currentDestination = -1;
	secondsUntilNextInstruction = 0;
}


void ATrackRunningController::Reset()
{
	Super::Reset();
	ResetExperimentState();
}