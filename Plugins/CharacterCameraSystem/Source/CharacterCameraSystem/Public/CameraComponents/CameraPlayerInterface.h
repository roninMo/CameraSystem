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
	/** Returns the camera style. The default styles are "Fixed", "Spectator", "FirstPerson", "ThirdPerson", "TargetLocking", and "Aiming". You can also add your own in the BasePlayerCameraManager class */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Style")
	FName GetCameraStyle() const;
	virtual FName GetCameraStyle_Implementation() const;

	/** Returns the orientation of the camera. This is only valid for third person, however you can adjust this to add your own logic */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Orientation") 
	ECameraOrientation GetCameraOrientation() const;
	virtual ECameraOrientation GetCameraOrientation_Implementation() const;

	/**
	 * Sets the camera style, and calls the OnCameraStyleSet function for handling camera transitions and other logic specific to each style.
	 * The default styles are "Fixed", "Spectator", "FirstPerson", "ThirdPerson", "TargetLocking", and "Aiming"
	 * 
	 * @remark Overriding this functions removes the default logic for transitioning between styles
	 * @remark OnCameraStyleSet should be called if TryActivateCameraTransition returns true
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Style")
	void SetCameraStyle(FName Style);
	virtual void SetCameraStyle_Implementation(FName Style);

	/**
	 * Sets the player's camera orientation.
	 * 
	 * @remark Overriding this functions removes the default logic for transitioning between orientations
	 * @remark OnCameraOrientation should be called if TryActivateCameraOrientation returns true
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Orientation")
	void SetCameraOrientation(ECameraOrientation Orientation);
	virtual void SetCameraOrientation_Implementation(ECameraOrientation Orientation);
	
	
};
