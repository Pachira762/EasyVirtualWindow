// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class FVirtualWindowSceneViewExtension : public FWorldSceneViewExtension
{
public:
	FVirtualWindowSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld);

	virtual void SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo) override;

	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override;

private:
	bool bModifyView = false;
	FVector WindowPosition;
	float WindowWidth;
	float WindowHeight;
	FTransform ViewTransform{};

public:
	void EnableModifyView(bool bEnable) 
	{ 
		bModifyView = bEnable; 
	}

	void SetWindowPositionAndSize(const FVector& Position, float Width, float Height)
	{
		WindowPosition = Position;
		WindowWidth = Width;
		WindowHeight = Height;
	}

	void SetViewTransform(const FTransform& Transform)
	{
		ViewTransform = Transform;
	}

private:
	inline static TWeakPtr<FVirtualWindowSceneViewExtension> SharedExtension = nullptr;

public:
	static TSharedPtr<FVirtualWindowSceneViewExtension> GetExtension(UWorld* World)
	{
		check(World);

		TSharedPtr<FVirtualWindowSceneViewExtension> Extension = SharedExtension.Pin();
		if (Extension.IsValid())
		{
			return Extension;
		}
		else {
			Extension = FSceneViewExtensions::NewExtension<FVirtualWindowSceneViewExtension>(World);
			SharedExtension = Extension;
			return Extension;
		}
	}
};
