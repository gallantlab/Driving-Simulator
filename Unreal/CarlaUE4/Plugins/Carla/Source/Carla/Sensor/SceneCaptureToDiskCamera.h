// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Sensor/SceneCaptureCamera.h"

#include "SceneCaptureToDiskCamera.generated.h"

UCLASS(Blueprintable,BlueprintType)
class CARLA_API ASceneCaptureToDiskCamera : public ASceneCaptureCamera
{
  GENERATED_BODY()

public:

  ASceneCaptureToDiskCamera(const FObjectInitializer& ObjectInitializer);

  virtual void BeginPlay() override;

  virtual void Tick(float DeltaTime) override;

  UFUNCTION(BlueprintCallable)
  bool SaveCaptureToDisk(const FString &FilePath) const;

  bool SaveNextFrame();	// auto-increments things

  void SetSaveFolder(const FString& folder);

public:

  UPROPERTY(Category = "Scene Capture", EditAnywhere, BlueprintReadWrite)
  bool bCaptureScene = false;

  UPROPERTY(Category = "Scene Capture", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = bCaptureScene))
  FString SaveToFolder;

  UPROPERTY(Category = "Scene Capture", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = bCaptureScene))
  FString FileName;

  UPROPERTY(Category = "Scene Capture", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = bCaptureScene))
  float CapturesPerSecond = 10.0f;

private:

  UPROPERTY()
  uint32 CaptureFileNameCount;
};
