// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningPracticeController.h"
#include "Settings/CarlaSettings.h"

ALearningPracticeController::ALearningPracticeController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	randomStream = FRandomStream();
}

void ALearningPracticeController::BeginPlay()
{
	Super::BeginPlay();

	const auto& CarlaSettings = gameInstance->GetCarlaSettings();
	numTargets = CarlaSettings.NumberTargets;
	visitationSaveFileName = FString::Printf(TEXT("%s %s visited.dat"), *(CarlaSettings.Subject),
												 *UExperimentType::ToString(CarlaSettings.GetExperimentType()));
	loadedVisited = destinations->LoadHasBeenVisited(visitationSaveFileName);

	// if there are fewer active destinations than number from settings, reduce to that number
	if (numTargets > destinations->GetNumberOfActiveDestinations())
		numTargets = destinations->GetNumberOfActiveDestinations();

	destinationPoints = CarlaSettings.DestinationPoints;
	// change all destination values to None
	if (!destinationPoints) {
		for (int i = 0; i < destinations->Num(); i++) {
			destinations->At(i).SetTargetValue(ETargetPointValue::None);
		}
	}
}

int ALearningPracticeController::GetRandomActiveDestination()
{
	int activeDestination;
	do
	{
		activeDestination = randomStream.RandHelper(destinations->Num());
	} while (!destinations->IsDestinationActive(activeDestination));
	return activeDestination;
}

void ALearningPracticeController::FindCurrentDestination() {
	// find first unvisited target
	int firstUnvisited = -1;
	for (int i = 0; i < learningPracticePlayerState->isTargetVisited.Num(); i++)
	{
		if (!learningPracticePlayerState->isTargetVisited[i])
		{
			firstUnvisited = i;
			break;
		}
	}

	// Only set destination if we found an unvisited target
	if (firstUnvisited != -1) {
		SetCurrentDestination(learningPracticePlayerState->currentTargets[firstUnvisited], false);
	}
	else {
		UE_LOG(LogFMRI, Warning, TEXT("No unvisited targets found after configuration!"));
		// This shouldn't happen with the fix above, but handle gracefully
		SetCurrentDestination(-1, false);
	}
}

void ALearningPracticeController::ConfigureNextDestination()
{
	learningPracticePlayerState->currentTargets.Empty();
	learningPracticePlayerState->isTargetVisited.Empty();
	learningPracticePlayerState->NumTargetsToCollect = 0;

	learningPracticePlayerState->NumTargetsToCollect = numTargets;
	UE_LOG(LogFMRI, Log, TEXT("Picking %d targets"), numTargets);
	int newTarget;

	if (loadedVisited)
	{
		int numSavedVisited = destinations->GetNumberOfVisitedDestinations();
		UE_LOG(LogFMRI, Log, TEXT("%d targets have been previously visited"), numSavedVisited);

		if (numSavedVisited >= numTargets)
		{
			UE_LOG(LogFMRI, Warning, TEXT("All targets have been previously visited. Resetting visited state."));
			destinations->ClearVisitedDestinations();
			destinations->SaveHasBeenVisited(visitationSaveFileName);
			numSavedVisited = 0;
		}

		for (int i = 0; i < numTargets; i++)
		{
			newTarget = GetRandomActiveDestination();
			// ensure no duplicates in current targets
			while (learningPracticePlayerState->currentTargets.Contains(newTarget)) {
				newTarget = GetRandomActiveDestination();
			}
			if (destinations->GetHasBeenVisited(newTarget)) {
				learningPracticePlayerState->currentTargets.Add(newTarget);
				learningPracticePlayerState->isTargetVisited.Add(true);
			}
			else {
				learningPracticePlayerState->currentTargets.Add(newTarget);
				learningPracticePlayerState->isTargetVisited.Add(false);
				destinations->At(newTarget).SetTriggerBoxVisibility(true);
			}
		}
	}
	else {
		for (int i = 0; i < numTargets; i++) {
			newTarget = GetRandomActiveDestination();
			learningPracticePlayerState->currentTargets.Add(newTarget);
			learningPracticePlayerState->isTargetVisited.Add(false);
			destinations->At(newTarget).SetTriggerBoxVisibility(true);
		}
	}
	FindCurrentDestination();
}
int ALearningPracticeController::CheckArrival()
{
	for (int i = 0 ; i < learningPracticePlayerState->currentTargets.Num(); i++)
	{
		int thisTarget = learningPracticePlayerState->currentTargets[i];
		if (destinations->At(thisTarget).IsProximal(GetPawn()) &&
			abs(learningPracticePlayerState->GetForwardSpeed()) < collectionSpeed &&
			!learningPracticePlayerState->isTargetVisited[i])
		{
			learningPracticePlayerState->isTargetVisited[i] = true;
			destinations->At(thisTarget).SetTriggerBoxVisibility(false);
			destinations->SetHasBeenVisited(thisTarget, true);
			destinations->SaveHasBeenVisited(visitationSaveFileName);
			UE_LOG(LogFMRI, Log, TEXT("Arrived at target %s, index %d"), *(destinations->GetDestinationName(thisTarget)), thisTarget);
			int nextTarget = -1;
			for (int j = 0; j < learningPracticePlayerState->currentTargets.Num(); j++)
				if (!learningPracticePlayerState->isTargetVisited[j])
				{
					nextTarget = learningPracticePlayerState->currentTargets[j];
					break;
				}
			SetCurrentDestination(nextTarget, false);

			PlayPickup();
			UpdatePoints(destinationPoints ? destinations->GetTargetPointsValue(thisTarget) : 1);
			bIsNumVisitedStale = true;
			return thisTarget;
		}
	}
	return -1;
}

void ALearningPracticeController::OnSegmentEnd(float maxWait, bool isLost)
{
	if (!isLost)
		// visited all targets
		SetDisplayText(FString::Printf(TEXT("Trial ended. All %d items collected"), GetNumberOfTargets()), EDisplayedPromptType::ForagingEnd);
	else
	{
		SetDisplayText(FString::Printf(TEXT("Trial ended")), EDisplayedPromptType::ForagingEnd);
		SetCurrentDestination(-2);
	}
	bIsNumVisitedStale = true;
	bIsNumActiveStale = true;
}

void ALearningPracticeController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		learningPracticePlayerState = Cast<ALearningPracticePlayerState>(PlayerState);
		check(learningPracticePlayerState != nullptr);
	}
}

void ALearningPracticeController::ExperimentTick(float dTime)
{
	if (segmentEndDelay > 0 && !learningPracticePlayerState->IsPaused())
	{
		segmentEndDelay -= dTime;
		if (segmentEndDelay <= 0)
		{
			segmentEndDelay = -1.0f;
			OnSegmentEnd(12.0);
			return;
		}
	}
	if (GetNumberOfTargets() <= 0)		// no foraging targets, waiting on one to be generated
	{
		if (secondsUntilNextDestination <= 0)					// is generating time
		{
			ConfigureNextDestination();
			SetDisplayText(FString::Printf(TEXT("Visit all %d destinations"), GetNumberOfTargets()), EDisplayedPromptType::ForagingStart);
		}
		else if (!learningPracticePlayerState->IsPaused())				// allows pausing of things between trials
			secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else	// is currently foraging
	{
		int arrivedTarget = CheckArrival();
		if (arrivedTarget != -1)
		{
			const FString arrivedTargetName = destinations->GetDestinationName(arrivedTarget);
			SetDisplayText(ARRIVEDAT+arrivedTargetName, EDisplayedPromptType::TargetVisited);
			FindCurrentDestination();
			if (GetNumberOfVisitedTargets() == GetNumberOfTargets()) {
				segmentEndDelay = 2.0f;
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

int ALearningPracticeController::GetNumberOfTargets() const
{
	return learningPracticePlayerState->NumTargetsToCollect;
}

int ALearningPracticeController::GetNumberOfVisitedTargets() const
{
	return destinations->GetNumberOfVisitedDestinations();
}

void ALearningPracticeController::TTLdown()
{
	Super::TTLdown();
	UE_LOG(LogFMRI, Log, TEXT("Learning Practice controller TTL down"));
}


void ALearningPracticeController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		for (int32 i = InputComponent->GetNumActionBindings() - 1; i >= 0; --i)
		{
			const FInputActionBinding& Binding = InputComponent->GetActionBinding(i);
			if (Binding.ActionName == "Button4")
			{
				InputComponent->RemoveActionBinding(i);
			}
		}
		// TTL Pausing
		InputComponent->BindAction("Pause", IE_Pressed, this, &ALearningPracticeController::TogglePause);
	}
}

void ALearningPracticeController::TogglePause()
{
	// only allow pausing between trials
	if (secondsUntilNextDestination > 0)
	{
		learningPracticePlayerState->paused = !learningPracticePlayerState->paused;
		if (learningPracticePlayerState->IsPaused())
		{
			SetDisplayText(TEXT("trials paused"), -1, EDisplayedPromptType::Paused);
		}
		else
		{
			ClearDisplayText();
		}
	}
}

void ALearningPracticeController::ReloadActiveDestinations()
{
	Super::ReloadActiveDestinations();
	bIsNumVisitedStale = true;
}

void ALearningPracticeController::ResetVisitedDestinations()
{
	bIsNumVisitedStale = true;
	UE_LOG(LogFMRI, Log, TEXT("Resetting visited destinations"));
	destinations->ClearVisitedDestinations();
	destinations->SaveHasBeenVisited(visitationSaveFileName);
	ConfigureNextDestination();
}
