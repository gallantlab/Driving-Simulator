// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Vehicle/LearningPracticeController.h"
#include "Game/LearningPracticeListPlayerState.h"
#include "LearningPracticeListController.generated.h"

DECLARE_DELEGATE(FOnDestinationsUpdated);

USTRUCT(BlueprintType)
struct FDestinationUIInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	bool bIsVisited;

	UPROPERTY(BlueprintReadOnly)
	int32 ID;
};
/**
 * 
 */
UCLASS()
class CARLA_API ALearningPracticeListController : public ALearningPracticeController
{
	GENERATED_BODY()

public:
	ALearningPracticeListController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void Possess(APawn *aPawn) override;

	virtual void ExperimentTick(float) override;

	FOnDestinationsUpdated OnDestinationsUpdated;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UTexture2D* FindDestinationImage(FString DestinationName);

	UFUNCTION(BlueprintCallable, Category = "UI")
	TArray<FDestinationUIInfo> GetCurrentDestinationsForUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleListRequested();

	void ToggleListInput();

protected:
	virtual void FindCurrentDestination();

	virtual void ConfigureNextDestination() override;

	virtual int CheckArrival() override;

	virtual void OnSegmentEnd(float maxWait, bool isLost = false) override;

	virtual void TTLdown() override;

	virtual void TogglePause();

	virtual void SetupInputComponent() override;

	virtual int GetNumberOfTargets() const override;

	UPROPERTY()
	TSubclassOf<class UUserWidget> DestinationsHUDClass;

	UPROPERTY()
	class UUserWidget* DestinationsHUDInstance;

	UPROPERTY()
	TMap<FString, UTexture2D*> TextureCache;

	virtual bool ShouldCheckOutOfBounds() const override;

private:
	ALearningPracticeListPlayerState *learningPracticeListPlayerState;

	int segmentVisitedCount = 0;
};
