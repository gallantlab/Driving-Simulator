// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"

#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Vehicle/WheeledVehicleAIController.h"


#include "RoutePlanner.generated.h"

/// Assign a random route to every ACarlaWheeledVehicle entering the trigger
/// volume. Routes must be added in editor after placing this actor into the
/// world. Spline tangents are ignored, only locations are taken into account
/// for making the route.
UCLASS()
class CARLA_API ARoutePlanner : public AActor
{
GENERATED_BODY()

public:

	ARoutePlanner(const FObjectInitializer& ObjectInitializer);

protected:

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif // WITH_EDITOR

	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	UPROPERTY(EditAnywhere)
	UBoxComponent* TriggerVolume;

	UPROPERTY(BlueprintReadWrite, Category = "Traffic Routes", EditAnywhere)
	TArray<USplineComponent*> Routes;

	UPROPERTY(BlueprintReadWrite, Category = "Traffic Routes", EditAnywhere, EditFixedSize)
	TArray<float> Probabilities;

	/// Indicates that upon intersection, the player controller should disengage player control
	/// and follow the spline, and return to player control until after spline is traversed.
	/// if 0, will not control player vehicle
	/// other numbers indicate that new track direction a subject will end up at
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int NewPlayerDirection = 0;

	/// If ControlPlayerVehicle is true, does this planner also control AI vehicles?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bControlAIAlongWithPlayer = false;

private:
	double maxSubjectBias = 1.0f;	// max probability that vehicles should be sent in direct towards the subject
	double repulsionBias = 0.65;	// amount to direct away from subject in inversion
	double inversionRadius = 10000;	// distance from subject at which cars begin to be directed directed _away_ from subject
	double maxBiasRadius = 40000;	// distance from subject at which max attraction begins
	double minBiasRadius = 5000;	// distance from subject at which max repulsion is reached

	// Stuff factored out of OnTriggerBeginOverlap
	const USplineComponent* ChoosePathForVehicle(AActor *actor, AWheeledVehicleAIController* vehicleController);

	void MakeRoute(const USplineComponent *routeToUse, TArray<FVector> &WayPoints);

};
