// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FaceTracker.generated.h"

UCLASS()
class EASYVIRTUALWINDOW_API AFaceTracker : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	FString DeviceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	FVector EyePosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	bool bTracked = false;

	AFaceTracker();

	void SetGravity(const FVector& InGravity);

	void SetEyePosition(const FVector& InPosition);

	void SetTrackState(bool bInTracked);

	FVector GetWorldEyePosition() const;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UBillboardComponent> SpriteComponent;
#endif
};
