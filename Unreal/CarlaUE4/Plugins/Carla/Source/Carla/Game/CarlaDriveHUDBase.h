// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaUMGBase.h"
#include "Vehicle/MRIPlayerController.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Vehicle/TrackedLearningController.h"
#include "Vehicle/ForagingController.h"
#include "Vehicle/LearningPracticeController.h"
#include "Vehicle/LearningPracticeListController.h"
#include "CarlaDriveHUDBase.generated.h"

/**
 * Base class for destination navigation HUD blueprint
 */
UCLASS()
class CARLA_API UCarlaDriveHUDBase : public UCarlaUMGBase
{
	GENERATED_BODY()
	
public:
	UCarlaDriveHUDBase(const FObjectInitializer &objectInitializer);

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	FString GetDisplayText() const
	{
		return displayText;
	};

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	EDisplayTextColor GetDisplayTextColor() const
	{
		return displayTextColor;
	};

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	FString GetPointsText() const
	{
		return pointsText;
	};

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	FString GetTimerText() const
	{
		return timerText;
	};

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	void SetDisplayText(FString text);

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	void SetDisplayTextColor(EDisplayTextColor color);

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	void UpdateDisplayPoints(int currentPoints);

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	void SetTimerText(int seconds);

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	void GetDestinationParameters(FVector2D &vector, float &distance, float& direction);

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	FString GetDestinationName() const ;

	UFUNCTION(BlueprintPure, Category = "Navigation HUD")
	FString ProximalTo();

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	virtual void FindController() override;

	UFUNCTION(BlueprintCallable, Category = "Navigation HUD")
	void BindDelegates();

	UFUNCTION(BlueprintPure, Category = "Learning HUD")
	int GetNumberOfDestinationsVisited() const ;

	UFUNCTION(BlueprintPure, Category = "Learning HUD")
	int GetTotalNumberOfDestinations() const ;

	UFUNCTION(BlueprintImplementableEvent, Category = "Navigation HUD")
	void SetShowNavigationHelp(bool showHelp);

	UFUNCTION()
	void OnDestinationsListUpdated();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateDestinationList();

protected:
	AMRIPlayerController* playerController;

	ANavigationVehicleController* navigationController;

	ATrackedLearningController* trackedLearningController;

	AForagingController* foragingController;

	ALearningPracticeController* learningPracticeController;

	ALearningPracticeListController* learningPracticeListController;

	FString displayText = FString("");

	EDisplayTextColor displayTextColor = EDisplayTextColor::White;

	FString pointsText = FString("Score: 0");

	FString timerText = FString("0:00");
};
