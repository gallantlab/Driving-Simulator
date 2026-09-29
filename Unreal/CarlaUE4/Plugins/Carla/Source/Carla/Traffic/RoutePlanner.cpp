// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "RoutePlanner.h"

#include "Util/RandomEngine.h"
#include "Vehicle/CarlaWheeledVehicle.h"
#include "Vehicle/WheeledVehicleAIController.h"

#include "Engine/CollisionProfile.h"

static bool IsSplineValid(const USplineComponent* SplineComponent)
{
	return (SplineComponent != nullptr) && (SplineComponent->GetNumberOfSplinePoints() > 1);
}

static AWheeledVehicleAIController* GetVehicleAIController(AActor* Actor)
{
	auto* Vehicle = (Actor->IsPendingKill() ? nullptr : Cast<ACarlaWheeledVehicle>(Actor));
	return (Vehicle != nullptr ? Cast<AWheeledVehicleAIController>(Vehicle->GetController()) : nullptr);
}

static const USplineComponent* PickARoute(URandomEngine& RandomEngine, const TArray<USplineComponent*>& Routes, const TArray<float>& Probabilities)
{
	check(Routes.Num() > 0);

	if (Routes.Num() == 1)
	{
		return Routes[0];
	}

	auto Index = RandomEngine.GetIntWithWeight(Probabilities);
	check((Index >= 0) && (Index < Routes.Num()));
	return Routes[Index];
}

ARoutePlanner::ARoutePlanner(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetHiddenInGame(true);
	TriggerVolume->SetMobility(EComponentMobility::Static);
	TriggerVolume->SetCollisionProfileName(FName("OverlapAll"));
	TriggerVolume->bGenerateOverlapEvents = true;
}

#if WITH_EDITOR
void ARoutePlanner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);
  const auto Size = Routes.Num();
  if (PropertyChangedEvent.Property && (Size != Probabilities.Num())) {
	Probabilities.Reset(Size);
	for (auto i = 0; i < Size; ++i) {
	  Probabilities.Add(1.0f / static_cast<float>(Size));
	  if (Routes[i] == nullptr) {
		Routes[i] = NewObject<USplineComponent>(this);
		Routes[i]->SetupAttachment(RootComponent);
		Routes[i]->SetHiddenInGame(true);
		Routes[i]->SetMobility(EComponentMobility::Static);
		Routes[i]->RegisterComponent();
	  }
	}
  }
}
#endif // WITH_EDITOR

void ARoutePlanner::BeginPlay()
{
	Super::BeginPlay();

	if (Routes.Num() < 1)
	{
		UE_LOG(LogCarla, Warning, TEXT("ARoutePlanner has no route assigned."));
		return;
	}

	for (auto&& Route : Routes)
	{
		if (!IsSplineValid(Route))
		{
			UE_LOG(LogCarla, Error, TEXT("ARoutePlanner has a route with zero way-points."));
			return;
		}
	}

	// Register delegate on begin overlap.
	if (!TriggerVolume->OnComponentBeginOverlap.IsAlreadyBound(this, &ARoutePlanner::OnTriggerBeginOverlap))
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ARoutePlanner::OnTriggerBeginOverlap);
	}

	// use settings from config
	const UCarlaSettings& settings = Cast<UCarlaGameInstance>(GetGameInstance())->GetCarlaSettings();
	maxBiasRadius = settings.maxBiasRadius;
	maxSubjectBias = settings.maxSubjectBias;
	repulsionBias = settings.repulsionBias;
	inversionRadius = settings.inversionRadius;
	minBiasRadius = settings.minBiasRadius;
}

void ARoutePlanner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Deregister the delegate.
	if (TriggerVolume->OnComponentBeginOverlap.IsAlreadyBound(this, &ARoutePlanner::OnTriggerBeginOverlap))
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &ARoutePlanner::OnTriggerBeginOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void ARoutePlanner::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
//	UE_LOG(LogCarla, Log, TEXT("Route planner overlapping with something"));
	AWheeledVehicleAIController* Controller = GetVehicleAIController(OtherActor);

	if (!Controller)
	{
//		UE_LOG(LogCarla, Log, TEXT("Controller casting resulted in null pointer"));
		return;
	}

	if ((NewPlayerDirection != 0) && !(Controller->IsPossessingThePlayer() || bControlAIAlongWithPlayer))
	{
//		UE_LOG(LogCarla, Log, TEXT("Did not fulfill requirements for player controlling route planner"));
		return;
	}

	const USplineComponent* routeToUse = Controller->IsPossessingThePlayer() ? Routes[0] : ChoosePathForVehicle(OtherActor, Controller);

	TArray <FVector> WayPoints;
	MakeRoute(routeToUse, WayPoints);

	Controller->SetFixedRoute(WayPoints, true, NewPlayerDirection);
}


const USplineComponent* ARoutePlanner::ChoosePathForVehicle(AActor *OtherActor, AWheeledVehicleAIController* vehicleController)
{
	URandomEngine* RandomEngine = (vehicleController != nullptr ? vehicleController->GetRandomEngine() : nullptr);
	if (RandomEngine != nullptr)
	{
		// re-weight route probabilities by distance to subject
		APawn* playerPawn = Cast<UCarlaGameInstance>(GetGameInstance())->GetPlayerPawn();
		FVector vectorToSubject = playerPawn->GetActorLocation() - OtherActor->GetActorLocation();

		// probability of being directed to the subject should increase with distance to the subject
		// and within a certain range it should just be random or a little bit repulsed to keep traffic flowing

		// calculate probabilities as a function of distance to subject
		double subjectProbability = maxSubjectBias;
		double distance = vectorToSubject.Size2D();
		if (distance < maxBiasRadius)        // less than the max bias radius
		{
			if (distance > inversionRadius)    // but more than inversion
				subjectProbability = 0.5 + (maxSubjectBias - 0.5) * (distance - inversionRadius) / (maxBiasRadius - inversionRadius);
			else                            // and less than inversion
			{
				if (distance > minBiasRadius)    // but more than mix radius
					subjectProbability = 0.5 - (repulsionBias - 0.5) * (inversionRadius - distance) / (inversionRadius - minBiasRadius);
				else                            // and less than min repulsion
					subjectProbability = 1.0f - repulsionBias;
			}
		}
		double otherProbability = (1.0f - subjectProbability) / (Routes.Num() - 1);

		// determine which route is the subject route
		int subjectRouteIndex = 0;
		double closestAngleCosine = vectorToSubject.CosineAngle2D(Routes[0]->GetDirectionAtSplinePoint(Routes[0]->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World));    // end spline vector
		double thisCosine = 0;
		for (int i = 1; i < Routes.Num(); i++)
		{
			thisCosine = vectorToSubject.CosineAngle2D(Routes[i]->GetDirectionAtSplinePoint(Routes[i]->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World));
			if (thisCosine > closestAngleCosine)
			{
				closestAngleCosine = thisCosine;
				subjectRouteIndex = i;
			}
		}

		// update probabilities
		for (int i = 0; i < Routes.Num(); i++)
			Probabilities[i] = (i == subjectRouteIndex) ? subjectProbability : otherProbability;

		// pick a route with these updated biases
		return PickARoute(*RandomEngine, Routes, Probabilities);
	}
	UE_LOG(LogCarla, Log, TEXT("Randomengine was null pointer, returning first in route vector"));
	return Routes[0];
}

void ARoutePlanner::MakeRoute(const USplineComponent *routeToUse, TArray<FVector> &WayPoints)
{
	const int Size = routeToUse->GetNumberOfSplinePoints();
	check(Size > 1);
	WayPoints.Reserve(Size);
	for (int i = 1; i < Size; ++i)
	{
		WayPoints.Add(routeToUse->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
	}
}