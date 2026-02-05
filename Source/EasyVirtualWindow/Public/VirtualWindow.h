// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VirtualWindow.generated.h"

UCLASS()
class EASYVIRTUALWINDOW_API AVirtualWindow : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	bool bEnable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	float WindowWidth = 68.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	float WindowHeight = 38.58;

	// TODO
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Window")
	float RealVirtualRate = 1.0;

	AVirtualWindow();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	TSharedPtr<class FVirtualWindowSceneViewExtension> VirtualWindowExtension;

	FCriticalSection Mutex;

	UPROPERTY()
	TArray<TObjectPtr<class AFaceTracker>> FaceTrackers;

	UPROPERTY()
	TObjectPtr<class UOSCServer> OSCServer;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> PlaneComponent;
#endif

	UFUNCTION()
	void CalcEyePosition(FVector& OutEyePosition, bool& bOutTracked);

	UFUNCTION()
	void OnOSCMessageReceived(const struct FOSCMessage& Message, const FString& IPAddress, int32 Port);
};
