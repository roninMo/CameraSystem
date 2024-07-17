// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerCameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "BasePlayerCameraManager.generated.h"

// Camera logic
// ->  Get the character's control rotation, and add that logic with interpolations to the camera rotation
// ->  Using the previous frames target location interpolate that with the current character location
// ->  Finish the target location and rotation calculations of the camera
// ->  Add camera adjustments if something is blocking the view
// ->  Adjusting the pivot offset and the pov based on character controls? Give the character access to these perhaps?


class ACharacterCameraLogic;
/**
 * 
 */
UCLASS()
class CHARACTERCAMERASYSTEM_API ABasePlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
protected:
	/** Camera smoothing and transition values */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager|Offsets") FVector PivotLagSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager|Offsets") FVector CurrentPivotLagSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager") float CrouchBlendDuration;
	UPROPERTY(BlueprintReadWrite) float CrouchBlendTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager") float RotationLagSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=  "Player Camera Manager") float OutOfBoundsLagSpeed;

	/** Camera values derived from the player on possess */
	UPROPERTY(BlueprintReadWrite) FName CameraSocketFirstPerson;
	UPROPERTY(BlueprintReadWrite) FName CameraSocketThirdPerson;
	UPROPERTY(BlueprintReadWrite) ECameraOrientation CameraOrientation;
	UPROPERTY(BlueprintReadWrite) ECameraStyle CamStyle;
	UPROPERTY(BlueprintReadWrite) TObjectPtr<ACharacterCameraLogic> Character;
	
	/** Camera capture values */
	UPROPERTY(BlueprintReadWrite) FMinimalViewInfo PreviousView;
	UPROPERTY(BlueprintReadWrite) FVector CharacterLocation;
	UPROPERTY(BlueprintReadWrite) FRotator CharacterRotation;
	UPROPERTY(BlueprintReadWrite) FVector TargetLocation;
	UPROPERTY(BlueprintReadWrite) FRotator TargetRotation;
	UPROPERTY(BlueprintReadWrite) FVector SmoothTargetLocation;
	UPROPERTY(BlueprintReadWrite) FRotator SmoothTargetRotation;
	UPROPERTY(BlueprintReadWrite) FVector CalculatedLocation;
	UPROPERTY(BlueprintReadWrite) FRotator CalculatedRotation;

	
public:
	ABasePlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	
//----------------------------------------------------------------------------------------------------------------------//
// Camera update functions                UpdateCamera -> UpdateViewTarget -> UpdateViewTarget_Internal -> CalcCamera	//
//----------------------------------------------------------------------------------------------------------------------//
	/**
	 * This function updates the view target and is extrapolated into multiple functions for blueprint customization and functionality, but I've broken into down into a single function for camera logic
	 * The default uses a value to determine what camera modes you're using which isn't initialized anywhere, so it always runs the actor's CalcCamera function, or the blueprint Update Camera function.
	 * I'd rather it branch out from enum types for different camera modes, and let everything else false into place. This handles the camera logic, and checks if the
	 * blueprint has any logic that should take precedence before handling this logic except instead of doing through the viewTarget_Internal, it's all handled here and branches out to these functions below
	 */
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	// TODO: I want a more advanced camera where there is technically no pivot point and the view target is the player with a box that the camera doesn't allow the character out of view (with smooth transitions for all scenarios)
	// This way the camera turns with the player, and functions much like the current camera while moving forward
	// However if they come towards the camera or go to the side, the camera turns with the player instead of the character just running sideways. This gives a different feel and atmosphere because
	// The focus isn't on the world, and it's easier to take in your surroundings instead of the camera always being focused on you

	// I'd create a mode for aiming where you center the camera so when the player is aiming it's not offset, and cut out all the smoothing everywhere
	
	/** The logic for the third person camera */
	UFUNCTION(BlueprintCallable) virtual void ThirdPersonCameraLogic(float DeltaTime, FTViewTarget& OutVT);

	/** The logic for third person aiming that's based on a center point */
	UFUNCTION(BlueprintCallable) virtual void ThirdPersonAimLogic(float DeltaTime, FTViewTarget& OutVT);
	
	/** The logic for the first person camera */
	UFUNCTION(BlueprintCallable) virtual void FirstPersonCameraLogic(float DeltaTime, FTViewTarget& OutVT);
	
	/** The logic for the free camera */
	UFUNCTION(BlueprintCallable) virtual void SpectatorCamLogic(float DeltaTime, FTViewTarget& OutVT);
	
	
//--------------------------------------------------------------------------------------------------//
// Camera calculation functions																		//
//--------------------------------------------------------------------------------------------------//
	/** Calculates a smooth interpolation between the camera's position and the target location */
	UFUNCTION(BlueprintCallable) virtual FVector CalculateCameraDrag(FVector Current, FVector Target, FRotator CameraRotation, float DeltaTime);

	/** Returns the camera socket specific to the current camera style */
	UFUNCTION(BlueprintCallable) virtual FName GetCameraSocketName();

	/** Handle smooth transitions of crouch logic while the player is crouching in air */
	UFUNCTION(BlueprintCallable) virtual void InAirCrouchLogic(FTViewTarget& OutVT, float DeltaTime);
	
	/** 
	 * Sets a new ViewTarget.
	 * @param NewViewTarget - New viewtarget actor.
	 * @param TransitionParams - Optional parameters to define the interpolation from the old viewtarget to the new. Transition will be instant by default.
	 */
	virtual void SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams) override;

	
};
