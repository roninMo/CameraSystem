// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerCameraTypes.h"
#include "UObject/Interface.h"
#include "CameraPlayerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UCameraPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Camera logic that the camera component uses for capturing information to perform camera functionality. Add this to characters that use the camera class
 */
class CHARACTERCAMERASYSTEM_API ICameraPlayerInterface
{
	GENERATED_BODY()

public:
	/** Returns the camera style */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Utility")
	FName GetCameraStyle() const;
	virtual FName GetCameraStyle_Implementation() const;

	/** Returns the camera orientation */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Utility") 
	ECameraOrientation GetCameraOrientation() const;
	virtual ECameraOrientation GetCameraOrientation_Implementation() const;

	/** Sets the player's camera style, and handles camera transitions and other logic specific to each style. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Utility")
	void SetCameraStyle(FName Style);
	virtual void SetCameraStyle_Implementation(FName Style);

	/** Sets the player's camera orientation */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Utility")
	void SetCameraOrientation(ECameraOrientation Orientation);
	virtual void SetCameraOrientation_Implementation(ECameraOrientation Orientation);
	
	
};
