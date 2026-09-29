// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CarlaDriveHUDBase.h"


UCarlaDriveHUDBase::UCarlaDriveHUDBase(const FObjectInitializer& initializer)
	: Super(initializer)
{
	navigationController = nullptr;
}

void UCarlaDriveHUDBase::FindController()
{
	Super::FindController();

	navigationController = Cast<ANavigationVehicleController>(controller);
	trackedLearningController = Cast<ATrackedLearningController>(controller);
	learningPracticeController = Cast<ALearningPracticeController>(controller);
	playerController = Cast<AMRIPlayerController>(controller);
	learningPracticeListController = Cast<ALearningPracticeListController>(controller);
}

void UCarlaDriveHUDBase::BindDelegates()
{
	if (playerController)
	{
		if (playerController->SetDisplayTextDelegate.IsBound())
			playerController->SetDisplayTextDelegate.Unbind();
		playerController->SetDisplayTextDelegate.BindUObject(this, &UCarlaDriveHUDBase::SetDisplayText);

		if (playerController->SetDisplayTextColorDelegate.IsBound())
			playerController->SetDisplayTextColorDelegate.Unbind();
		playerController->SetDisplayTextColorDelegate.BindUObject(this, &UCarlaDriveHUDBase::SetDisplayTextColor);

		if (playerController->SetDisplayPointsDelegate.IsBound())
			playerController->SetDisplayPointsDelegate.Unbind();
		playerController->SetDisplayPointsDelegate.BindUObject(this, &UCarlaDriveHUDBase::UpdateDisplayPoints);

		if (playerController->SetTimeRemainingDelegate.IsBound())
			playerController->SetTimeRemainingDelegate.Unbind();
		playerController->SetTimeRemainingDelegate.BindUObject(this, &UCarlaDriveHUDBase::SetTimerText);
	}
	if (navigationController)
	{
		if (navigationController->ShowHelpInfoDelegate.IsBound())
			navigationController->ShowHelpInfoDelegate.Unbind();
		navigationController->ShowHelpInfoDelegate.BindUObject(this, &UCarlaDriveHUDBase::SetShowNavigationHelp);
		UE_LOG(LogFMRI, Log, TEXT("HUD binding delegate to controller %ssuccessful"), navigationController->ShowHelpInfoDelegate.IsBound() ? TEXT("") : TEXT("not "))
	}
	if (learningPracticeListController)
	{
		if (learningPracticeListController->OnDestinationsUpdated.IsBound())
		{
			learningPracticeListController->OnDestinationsUpdated.Unbind();
		}
		learningPracticeListController->OnDestinationsUpdated.BindUObject(this, &UCarlaDriveHUDBase::OnDestinationsListUpdated);
		UE_LOG(LogFMRI, Log, TEXT("HUD binding delegate to learning practice list controller %ssuccessful"), learningPracticeListController->OnDestinationsUpdated.IsBound() ? TEXT("") : TEXT("not "))
	}
}

void UCarlaDriveHUDBase::GetDestinationParameters(FVector2D &vector, float &distance, float& direction)
{
	if (navigationController)
		navigationController->GetDestinationParameters(vector, distance, direction);
}

FString UCarlaDriveHUDBase::GetDestinationName() const
{
	if (navigationController)
		if (navigationController->GetDistanceToDestination() > 0)
			return navigationController->GetCurrentDestinationName();
	return FString("No destination");
}

FString UCarlaDriveHUDBase::ProximalTo()
{
	if (!navigationController) FindController();
#ifdef NAV
	return navigationController->GetProximalTarget();
#else
	return FString("None");
#endif
}

int UCarlaDriveHUDBase::GetNumberOfDestinationsVisited() const {
	if (trackedLearningController)
	{
		return trackedLearningController->GetNumberOfVisitedActiveDestinations();
	}
	if (learningPracticeController)
	{
		return learningPracticeController->GetNumberOfVisitedActiveDestinations();
	}
	if (learningPracticeListController)
	{
			return learningPracticeListController->GetNumberOfVisitedActiveDestinations();
	}
	return 0;
}

int UCarlaDriveHUDBase::GetTotalNumberOfDestinations() const {
	if (trackedLearningController)
	{
		return trackedLearningController->GetNumberOfActiveDestinations();
	}
	if (learningPracticeController)
	{
		return learningPracticeController->GetNumberOfActiveDestinations();
	}
	if (learningPracticeListController)
	{
			return learningPracticeListController->GetNumberOfActiveDestinations();
	}
	return 0;
}

void UCarlaDriveHUDBase::SetDisplayText(FString text)
{
	displayText = text;
}

void UCarlaDriveHUDBase::SetDisplayTextColor(EDisplayTextColor color)
{
	displayTextColor = color;
}

void UCarlaDriveHUDBase::UpdateDisplayPoints(int currentPoints)
{
	pointsText = FString::Printf(TEXT("Score: %d"), currentPoints);
}

void UCarlaDriveHUDBase::SetTimerText(int seconds)
{
	timerText = FString::Printf(TEXT("%d:%02d"), seconds / 60, seconds % 60);
}
void UCarlaDriveHUDBase::OnDestinationsListUpdated()
{
	UpdateDestinationList();
}