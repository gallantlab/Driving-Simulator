// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningPracticeListController.h"
#include "Util/LearningPracticeListLoggerComponent.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"


ALearningPracticeListController::ALearningPracticeListController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UUserWidget> HUDObject(TEXT("/Game/Blueprints/Game/DestinationsListHUD.DestinationsListHUD_C"));

	if (HUDObject.Succeeded())
	{
		DestinationsHUDClass = HUDObject.Class;
	}
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("CRITICAL ERROR: Could not find WBP_DestinationsListHUD! Check path in C++."));
		DestinationsHUDClass = nullptr;
	}
}

void ALearningPracticeListController::BeginPlay()
{
	Super::BeginPlay();

	if (DestinationsHUDClass)
	{
		DestinationsHUDInstance = CreateWidget<UUserWidget>(this, DestinationsHUDClass);
		if (DestinationsHUDInstance)
		{
			DestinationsHUDInstance->AddToViewport(10);
			DestinationsHUDInstance->RemoveFromParent();
		}
	}

	if (destinations)
	{
		UE_LOG(LogFMRI, Log, TEXT("Pre-loading destination images into cache..."));

		// Loop through every single destination in the database to preload
		for (int i = 0; i < destinations->Num(); i++)
		{
			FString DestName = destinations->GetDestinationName(i);
			FindDestinationImage(DestName);
		}

		UE_LOG(LogFMRI, Log, TEXT("Finished pre-loading images."));
	}

}

void ALearningPracticeListController::FindCurrentDestination() {
	// find first unvisited target
	int firstUnvisited = -1;
	for (int i = 0; i < learningPracticeListPlayerState->isTargetVisited.Num(); i++)
	{
		if (!learningPracticeListPlayerState->isTargetVisited[i])
		{
			firstUnvisited = i;
			break;
		}
	}

	// Only set destination if we found an unvisited target
	if (firstUnvisited != -1) {
		SetCurrentDestination(learningPracticeListPlayerState->currentTargets[firstUnvisited], false);
	}
	else {
		UE_LOG(LogFMRI, Warning, TEXT("No unvisited targets found after configuration!"));
		// This shouldn't happen with the fix above, but handle gracefully
		SetCurrentDestination(-1, false);
	}
}

void ALearningPracticeListController::ConfigureNextDestination()
{
	UE_LOG(LogFMRI, Log, TEXT("Generating foraging targets"));

	// DEBUG 1: Verify the file and load status
    UE_LOG(LogFMRI, Error, TEXT("[DEBUG] Filename: %s"), *visitationSaveFileName);
    UE_LOG(LogFMRI, Error, TEXT("[DEBUG] LoadedVisited Bool: %s"), loadedVisited ? TEXT("TRUE") : TEXT("FALSE"));

	learningPracticeListPlayerState->currentTargets.Empty();
	learningPracticeListPlayerState->isTargetVisited.Empty();

	learningPracticeListPlayerState->NumTargetsToVisit = numTargets;
	UE_LOG(LogFMRI, Log, TEXT("Picking %d targets"), numTargets);
	int newTarget;

	if (loadedVisited)
	{
		int numSavedVisited = destinations->GetNumberOfVisitedDestinations();
		UE_LOG(LogFMRI, Log, TEXT("%d targets have been previously visited"), numSavedVisited);

		// if (numSavedVisited >= numTargets)
		// {
		// 	UE_LOG(LogFMRI, Warning, TEXT("All targets have been previously visited. Resetting visited state."));
		// 	destinations->ClearVisitedDestinations();
		// 	destinations->SaveHasBeenVisited(visitationSaveFileName);
		// 	numSavedVisited = 0;
		// }

		for (int i = 0; i < numTargets; i++)
		{
			newTarget = GetRandomActiveDestination();
			// ensure no duplicates in current targets
			while (learningPracticeListPlayerState->currentTargets.Contains(newTarget)) {
				newTarget = GetRandomActiveDestination();
			}
			bool bWasVisited = destinations->GetHasBeenVisited(newTarget);
			if (bWasVisited)
			{
				UE_LOG(LogFMRI, Log, TEXT("Target %s has been previously visited"), *(destinations->GetDestinationName(newTarget)));
				learningPracticeListPlayerState->currentTargets.Add(newTarget);
				learningPracticeListPlayerState->isTargetVisited.Add(true);
				destinations->At(newTarget).SetTriggerBoxVisibility(false);
			}
			else
            {
                learningPracticeListPlayerState->currentTargets.Add(newTarget);
                learningPracticeListPlayerState->isTargetVisited.Add(false);
                destinations->At(newTarget).SetTriggerBoxVisibility(false);
            }
			// if (destinations->GetHasBeenVisited(newTarget)) {
			// 	learningPracticeListPlayerState->currentTargets.Add(newTarget);
			// 	learningPracticeListPlayerState->isTargetVisited.Add(true);
			// }
			// else {
			// 	learningPracticeListPlayerState->currentTargets.Add(newTarget);
			// 	learningPracticeListPlayerState->isTargetVisited.Add(false);
			// }
		}
	}

	else {
		for (int i = 0; i < numTargets; i++) {
			newTarget = GetRandomActiveDestination();
			while (learningPracticeListPlayerState->currentTargets.Contains(newTarget)) {
				newTarget = GetRandomActiveDestination();
			}
			learningPracticeListPlayerState->currentTargets.Add(newTarget);
			learningPracticeListPlayerState->isTargetVisited.Add(false);
		}
	}
	FindCurrentDestination();
	SetDisplayText(FString::Printf(TEXT("Destination list updated.")), EDisplayedPromptType::TargetListUpdated);
	OnDestinationsUpdated.ExecuteIfBound();
}

int ALearningPracticeListController::CheckArrival()
{
	for (int i = 0 ; i < learningPracticeListPlayerState->currentTargets.Num(); i++)
	{
		int thisTarget = learningPracticeListPlayerState->currentTargets[i];
		if (destinations->At(thisTarget).IsProximal(GetPawn()) &&
			abs(learningPracticeListPlayerState->GetForwardSpeed()) < collectionSpeed &&
			!learningPracticeListPlayerState->isTargetVisited[i])
		{
			learningPracticeListPlayerState->isTargetVisited[i] = true;
			destinations->At(thisTarget).SetTriggerBoxVisibility(false);
			destinations->SetHasBeenVisited(thisTarget, true);
			destinations->SaveHasBeenVisited(visitationSaveFileName);
			UE_LOG(LogFMRI, Log, TEXT("Arrived at target %s, index %d"), *(destinations->GetDestinationName(thisTarget)), thisTarget);
			int nextTarget = -1;
			for (int j = 0; j < learningPracticeListPlayerState->currentTargets.Num(); j++)
				if (!learningPracticeListPlayerState->isTargetVisited[j])
				{
					nextTarget = learningPracticeListPlayerState->currentTargets[j];
					break;
				}
			SetCurrentDestination(nextTarget, false);

			UpdatePoints(destinationPoints ? destinations->GetTargetPointsValue(thisTarget) : 1);
			bIsNumVisitedStale = true;
			OnDestinationsUpdated.ExecuteIfBound();
			return thisTarget;
		}
	}
	return -1;
}

void ALearningPracticeListController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		learningPracticeListPlayerState = Cast<ALearningPracticeListPlayerState>(PlayerState);
		check(learningPracticeListPlayerState != nullptr);
	}
}

void ALearningPracticeListController::OnSegmentEnd(float maxWait, bool isLost) {
	if (GetNumberOfVisitedActiveDestinations() != GetNumberOfActiveDestinations()) {
		UE_LOG(LogFMRI, Log, TEXT("Segment ended but not all targets were visited (%d out of %d)"),
			GetNumberOfVisitedActiveDestinations(), GetNumberOfActiveDestinations());
		learningPracticeListPlayerState->NumTargetsToVisit = 0;
	}
	if (!isLost)
		// visited all targets
		SetDisplayText(FString::Printf(TEXT("Trial ended. All %d items visited"), GetNumberOfTargets()), EDisplayedPromptType::ForagingEnd);
	else
	{
		SetDisplayText(FString::Printf(TEXT("Trial ended")), EDisplayedPromptType::ForagingEnd);
		SetCurrentDestination(-2);
	}
	bIsNumVisitedStale = true;
	bIsNumActiveStale = true;
}


void ALearningPracticeListController::ExperimentTick(float dTime) {
	if (segmentEndDelay > 0 && !learningPracticeListPlayerState->IsPaused())
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
			segmentVisitedCount = 0;
			for (bool bVisited : learningPracticeListPlayerState->isTargetVisited)
			{
				if (bVisited) segmentVisitedCount++;
			}
		}
		else if (!learningPracticeListPlayerState->IsPaused())				// allows pausing of things between trials
			secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else
	{
		int arrivedTarget = CheckArrival();
		if (arrivedTarget != -1)
		{
			segmentVisitedCount++;
			const FString arrivedTargetName = destinations->GetDestinationName(arrivedTarget);
			SetDisplayText(ARRIVEDAT+arrivedTargetName, EDisplayedPromptType::TargetVisited);
			FindCurrentDestination();
			if (segmentVisitedCount == GetNumberOfTargets()) {
				segmentEndDelay = 2.0f;
			}
		}
	}
}

bool ALearningPracticeListController::ShouldCheckOutOfBounds() const
{
	return false; // disable out-of-bounds checking
}

int ALearningPracticeListController::GetNumberOfTargets() const
{
	return learningPracticeListPlayerState->NumTargetsToVisit;
}


void ALearningPracticeListController::TTLdown()
{
	Super::TTLdown();
	UE_LOG(LogFMRI, Log, TEXT("LearningPracticeList controller TTL down"));
}


void ALearningPracticeListController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// TTL Pausing
		InputComponent->BindAction("Pause", IE_Pressed, this, &ALearningPracticeListController::TogglePause);
		// Toggle List
		InputComponent->BindAction("ToggleDestinations", IE_Pressed, this, &ALearningPracticeListController::ToggleListInput);
	}
}

void ALearningPracticeListController::TogglePause()
{
	// only allow pausing between trials
	if (secondsUntilNextDestination > 0)
	{
		learningPracticeListPlayerState->paused = !learningPracticeListPlayerState->paused;
		if (learningPracticeListPlayerState->IsPaused())
		{
			SetDisplayText(TEXT("trials paused"), -1, EDisplayedPromptType::Paused);
		}
		else
		{
			ClearDisplayText();
		}
	}
}

void ALearningPracticeListController::ToggleListInput()
{
	if (!DestinationsHUDInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("HUD Instance is NULL. Creation in BeginPlay failed."));
		return;
	}

	if (DestinationsHUDInstance->IsInViewport())
	{
		DestinationsHUDInstance->RemoveFromParent();
		learningPracticeListPlayerState->bIsDestinationsListVisible = false;
		SetInputMode(FInputModeGameOnly());
	}
	else
	{
		DestinationsHUDInstance->AddToViewport(10);
		learningPracticeListPlayerState->bIsDestinationsListVisible = true;
	}
}

TArray<FDestinationUIInfo> ALearningPracticeListController::GetCurrentDestinationsForUI()
{
	TArray<FDestinationUIInfo> OutputList;

	if (!learningPracticeListPlayerState || !destinations)
	{
		return OutputList;
	}

	for (int i = 0; i < learningPracticeListPlayerState->currentTargets.Num(); i++)
	{
		int32 TargetID = learningPracticeListPlayerState->currentTargets[i];

		if (TargetID >= 0 && i < learningPracticeListPlayerState->isTargetVisited.Num())
		{
			FDestinationUIInfo NewInfo;
			NewInfo.Name = destinations->GetDestinationName(TargetID);
			NewInfo.bIsVisited = learningPracticeListPlayerState->isTargetVisited[i];
			NewInfo.ID = TargetID;

			OutputList.Add(NewInfo);
		}
	}

	OutputList.Sort([](const FDestinationUIInfo& A, const FDestinationUIInfo& B) {
		return A.bIsVisited < B.bIsVisited;
	});

	return OutputList;
}

UTexture2D* ALearningPracticeListController::FindDestinationImage(FString DestinationName)
{
    FString SanitizedName = DestinationName.Replace(TEXT(" "), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("&"), TEXT("and"));
	SanitizedName = SanitizedName.ToLower();

	if (TextureCache.Contains(SanitizedName)) {
		return TextureCache[SanitizedName];
	}

    FString FolderPath = "/Game/Blueprints/Game/DestinationImageFiles/";
    FString FullPath = FolderPath + SanitizedName + "." + SanitizedName;

    UTexture2D* LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, *FullPath));

	if (LoadedTexture) {
		TextureCache.Add(SanitizedName, LoadedTexture);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("FAILED to load: %s"), *FullPath);
	}

    return LoadedTexture;
}