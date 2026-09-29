// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "TrackedLearningController.h"

#include "Game/MRIPlayerState.h"
#include "Settings/CarlaSettings.h"


ATrackedLearningController::ATrackedLearningController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void ATrackedLearningController::BeginPlay()
{
	Super::BeginPlay();

	UCarlaSettings &settings = gameInstance->GetCarlaSettings();
	visitationSaveFileName = FString::Printf(TEXT("%s %s visited.dat"), *(settings.Subject),
											 *UExperimentType::ToString(settings.GetExperimentType()));
	destinations->LoadHasBeenVisited(visitationSaveFileName);
}

void ATrackedLearningController::ExperimentTick(float dTime)
{
	if (segmentEndDelay > 0)
	{
		segmentEndDelay -= dTime;
		if (segmentEndDelay <= 0)
		{
			segmentEndDelay = -1.0f;
			OnSegmentEnd(12.0);
			return;
		}
	}
	if (navigationPlayerState->currentDestination < 0)		// no destination, waiting on one to be generated
	{
		if (GetNumberOfVisitedActiveDestinations() == GetNumberOfActiveDestinations()) {
			return; // skips out of loop
		}
		if (secondsUntilNextDestination <= 0)					// is generating time
		{
			ConfigureNextDestination();
			SetDisplayText(GOTO + GetCurrentDestinationName(), EDisplayedPromptType::GoTo);	// display destination for two seconds
		}
		else secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else	// is currently navigating to a destination
	{
		if (CheckForArrival())		// close enough to destination and is stopped
		{
			OnSegmentEnd(12.0);
			if (GetNumberOfVisitedActiveDestinations() ==  GetNumberOfActiveDestinations()) {
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

void ATrackedLearningController::PickNextDestination(int minIndex, int maxIndex)
{
	// force permuted destinations
	SetCurrentDestination(destinations->PickPurePermutedDestination(GetPawn()));
	if (GetCurrentDestination() >= 0) {
		for (int i = 0; i < destinations->Num(); i++) {
			if (i == GetCurrentDestination())
				destinations->At(i).SetTriggerBoxVisibility(true);
			else
				destinations->At(i).SetTriggerBoxVisibility(false);
		}
	}
}

void ATrackedLearningController::OnSegmentEnd(float maxWait, bool isLost)
{
	if (GetNumberOfVisitedActiveDestinations() == GetNumberOfActiveDestinations()) {
		SetDisplayText(SESSIONEND, -1, EDisplayedPromptType::SessionEnd);
	}
	else {
		destinations->SetHasBeenVisited(GetCurrentDestination());
		destinations->SaveHasBeenVisited(visitationSaveFileName);
		bIsNumVisitedStale = true;
		Super::OnSegmentEnd(maxWait, isLost);
	}
}
void ATrackedLearningController::ReloadActiveDestinations()
{
	Super::ReloadActiveDestinations();
	bIsNumVisitedStale = true;
	bIsActiveCountStale = true;
}

void ATrackedLearningController::ResetVisitedDestinations()
{
	bIsNumVisitedStale = true;
	destinations->ClearVisitedDestinations();
	destinations->SaveHasBeenVisited(visitationSaveFileName);
}