// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ForagingController.h"

AForagingController::AForagingController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void AForagingController::BeginPlay()
{
	Super::BeginPlay();

	const auto& CarlaSettings = gameInstance->GetCarlaSettings();
	minNumTargets = CarlaSettings.MinNumberTargets;
	maxNumTargets = CarlaSettings.MaxNumberTargets;
	targetAvailabilityMultiplier = CarlaSettings.AvailableTargetMultiplier;
	valueResetProbability = CarlaSettings.ValueResetProbability;
	minStableTrials = CarlaSettings.MinStableTrials;

	numStableTrials = 0;

	baseTrialTime = CarlaSettings.BaseTrialTime;
	additionalItemTime = CarlaSettings.AdditionalItemTime;

	maxTrialTime = CarlaSettings.MaxTrialTime;

	parameterAdjust = AdaptiveTrialParameterGenerator();

	collectionSpeed = CarlaSettings.CollectionSpeed * 44.7; // MPH in settings, convert to cm/s

	if (CarlaSettings.GetExperimentType() == EExperimentType::TimedForaging)
		isTimed = true;

	// need to initialize random target values
	destinations->RandomizeTargetValues();
}


void AForagingController::ConfigureNextDestination()
{
	UE_LOG(LogFMRI, Log, TEXT("Generating foraging targets"));

	foragingPlayerState->currentForageTargets.Empty();
	foragingPlayerState->isTargetForaged.Empty();

	// roll the dice to reset target values
	if ((numStableTrials >= minStableTrials) && (randomStream.GetFraction() < valueResetProbability))
	{
		UE_LOG(LogFMRI, Log, TEXT("Values reset"));
		destinations->RandomizeTargetValues();
		numStableTrials = 0;
	}

	int numTargets = randomStream.RandRange(minNumTargets, maxNumTargets);
	secondsRemaining = baseTrialTime + additionalItemTime * (numTargets - 3);

	parameterAdjust.ValidateTrialInfo(numTargets, secondsRemaining, targetAvailabilityMultiplier, 
		baseTrialTime, maxTrialTime, minNumTargets, maxNumTargets);

	foragingPlayerState->NumTargetsToCollect = numTargets;
	UE_LOG(LogFMRI, Log, TEXT("Picking %d foraging targets"), foragingPlayerState->NumTargetsToCollect);

	int numberToActivate = FGenericPlatformMath::CeilToInt(foragingPlayerState->NumTargetsToCollect * targetAvailabilityMultiplier);
	if (numberToActivate > destinations->GetNumberOfActiveDestinations())	// upper bound because otherwise the while loop will be infinite
		numberToActivate = destinations->GetNumberOfActiveDestinations();

	thisTrialDuration = secondsRemaining;
	foragingPlayerState->trialDuration = thisTrialDuration;
	for (int i = 0; i < numberToActivate; i ++)	
	{
		int newTarget = destinations->GetNextDestination(GetPawn());

		// make sure this is not a duplicate of one already picked
		while (foragingPlayerState->currentForageTargets.Contains(newTarget))
			newTarget = destinations->GetNextDestination(GetPawn());

		// add to list and also turn on the triggerbox
		foragingPlayerState->currentForageTargets.Add(newTarget);
		foragingPlayerState->isTargetForaged.Add(false);
		foragingPlayerState->targetValues.Add(destinations->GetTargetPointsValue(newTarget));
		destinations->At(newTarget).SetTriggerBoxVisibility(true);

		destinationName = &(destinations->At(newTarget).GetName());
		UE_LOG(LogFMRI, Log, TEXT("    Foraging target %s, index %d"), **destinationName, newTarget);
	}

	// set this so if the subject needs help, the NavigationPlayerController can display help
	SetCurrentDestination(foragingPlayerState->currentForageTargets[0], false);

	// if configured, start the countdown timer
	// the timer is looped to tick once a second
	// will be cleared when things fall to 0
	if (isTimed)
	{
		SetTimeRemainingDelegate.ExecuteIfBound(secondsRemaining);
		GetWorld()->GetTimerManager().SetTimer(secondTickTimer, this, &AForagingController::SecondTick, 1.0, true);
	}
}

void AForagingController::SecondTick()
{
	secondsRemaining--;
	// time is up
	SetTimeRemainingDelegate.ExecuteIfBound(secondsRemaining);
	if (secondsRemaining == 0)
	{
		// force end the trial
		// we're not changing the signature because we can record the elapsed time
		// and determine how the trial ended
		OnSegmentEnd(12, true);
	}
}


bool AForagingController::CheckForArrival()
{
	for (int i = 0 ; i < foragingPlayerState->currentForageTargets.Num(); i++)
	{
		int thisTarget = foragingPlayerState->currentForageTargets[i];
		if (destinations->At(thisTarget).IsProximal(GetPawn()) &&	// are we at the triggerbox
			abs(foragingPlayerState->GetForwardSpeed()) < collectionSpeed &&		// are we stopped
			!foragingPlayerState->isTargetForaged[i])					// have we already collected this
		{
			// change stored state about foraging
			foragingPlayerState->isTargetForaged[i] = true;
			visitedCount++;
			destinations->At(thisTarget).SetTriggerBoxVisibility(false);

			// update information for the super NavigationController state
			// basically select the next available target as the destination
			int nextTarget = -1;
			for (int j = 0; j < foragingPlayerState->currentForageTargets.Num(); j++)
				if (!foragingPlayerState->isTargetForaged[j])
				{
					nextTarget = foragingPlayerState->currentForageTargets[j];
					break;
				}
			// the init as -1 guarantees that at the end of the trial it gets set to -1
			SetCurrentDestination(nextTarget, false);

			PlayPickup();
			UpdatePoints(destinations->GetTargetPointsValue(thisTarget));
			timeElapsedToRecentPickup = thisTrialDuration - secondsRemaining;
			return true;
		}
	}
	return false;
}

void AForagingController::OnSegmentEnd(float maxWait, bool isLost)
{
	if (isTimed)
	{
		GetWorld()->GetTimerManager().ClearTimer(secondTickTimer);
		SetTimeRemainingDelegate.ExecuteIfBound(0);
	}
	
	if (!isLost)
		SetDisplayText(FString::Printf(TEXT("Trial ended. All %d items collected"), GetNumberOfTargets()), EDisplayedPromptType::ForagingEnd);
	else
	{
		SetDisplayText(FString::Printf(TEXT("Trial ended")), EDisplayedPromptType::ForagingEnd);
		SetCurrentDestination(-2);
	}

	parameterAdjust.AddTrialInfo(foragingPlayerState->NumTargetsToCollect, 
							visitedCount,
							timeElapsedToRecentPickup, 
									targetAvailabilityMultiplier);

	// because there may be more targets than needed, turn all of them off one last time
	for (int target: foragingPlayerState->currentForageTargets)
		destinations->At(target).SetTriggerBoxVisibility(false);

	foragingPlayerState->currentForageTargets.Empty();
	foragingPlayerState->isTargetForaged.Empty();
	foragingPlayerState->targetValues.Empty();
	foragingPlayerState->NumTargetsToCollect = 0;
	foragingPlayerState->trialDuration = 0;
	visitedCount = 0;

	numStableTrials++;
	UE_LOG(LogFMRI, Log, TEXT("%d stable trials elapsed"), numStableTrials);

	secondsUntilNextDestination = maxWait > 4 ? randomStream.FRandRange(4.0f, maxWait) : 4;
	UE_LOG(LogFMRI, Log, TEXT("%f seconds until next trial"), secondsUntilNextDestination);
}

void AForagingController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		foragingPlayerState = Cast<AForagingPlayerState>(PlayerState);
		check(foragingPlayerState != nullptr);
	}
}

void AForagingController::Tick(float DeltaSeconds)
{
	// this is the highest priority thing to display
	// we need to get the participant back to the foraging zone
	if (ShouldCheckOutOfBounds()) {
		if (!(this->GetDestinationParserComponent()->IsActorInActiveNeighborhoods(this->GetVehiclePawn())))
		{
			SetDisplayText(TEXT("out of bound\nturn around / follow arrow"), 1, EDisplayedPromptType::OutOfBounds, EDisplayTextColor::Red);
			ShowHelp(1.0);
		}
	}
	Super::Tick(DeltaSeconds);
}

void AForagingController::ExperimentTick(float dTime)
{

	if (GetNumberOfTargets() <= 0)		// no foraging targets, waiting on one to be generated
	{
		if (secondsUntilNextDestination <= 0)					// is generating time
		{
			ConfigureNextDestination();
			SetDisplayText(FString::Printf(TEXT("Find %d items"), GetNumberOfTargets()), EDisplayedPromptType::ForagingStart);
		}
		else if (!foragingPlayerState->IsPaused())				// allows pausing of things between trials
			secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else	// is currently foraging
	{
		if (CheckForArrival())		
		{
			if (GetNumberOfVisitedTargets() == GetNumberOfTargets())
				OnSegmentEnd(12.0);
			else
			{
				SetDisplayText(FString::Printf(TEXT("%d of %d items acquired"), GetNumberOfVisitedTargets(), GetNumberOfTargets()), EDisplayedPromptType::ItemForaged);
			}
		}
		if (lost)
		{
			lostDown -= dTime;
			if (lostDown <= 0)
			{
				lost = false;
				lostPressCount = 0;
			}
			else if (lostPressCount > 2)
			{
				lost = false;
				lostPressCount = 0;
				OnSegmentEnd(12.0, true);
			}
		}
	}
}

int AForagingController::GetNumberOfTargets() const
{
	return foragingPlayerState->NumTargetsToCollect;
}

void AForagingController::PickNextDestination(int minIndex, int maxIndex)
{
	// As far as I can tell this is unnecessary for the Foraging controller.
	// The navigation controller's ConfigureNextDestination calls this, but
	// here, everything is handled within ConfigureNextDestination
}

void AForagingController::TTLdown()
{
	Super::TTLdown();
	UE_LOG(LogFMRI, Log, TEXT("Foraging controller TTL down"));
}


void AForagingController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// TTL Pausing
		InputComponent->BindAction("Pause", IE_Pressed, this, &AForagingController::TogglePause);
	}
}

void AForagingController::TogglePause()
{
	// only allow pausing between trials
	if (secondsUntilNextDestination > 0)
	{
		foragingPlayerState->paused = !foragingPlayerState->paused;
		if (foragingPlayerState->IsPaused())
		{
			SetDisplayText(TEXT("trials paused"), -1, EDisplayedPromptType::Paused);
		}
		else
		{
			ClearDisplayText();
		}
	}
}
