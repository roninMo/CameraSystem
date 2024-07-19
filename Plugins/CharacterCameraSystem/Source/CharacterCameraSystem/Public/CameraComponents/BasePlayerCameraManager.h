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
	 * The default uses a value to determine what camera modes you're using which isn't initialized anywhere, so it always runs the actor's CalcCamera function, or the blueprint Update Camera function. \n\n
	 * I'd rather it branch out from enum types for different camera modes, and let everything else false into place. \n\name
	 *
	 * This handles the camera logic, and checks if the blueprint has any logic that should take precedence before handling this logic except
	 * instead of doing through the viewTarget_Internal, it's all handled here and branches out to these functions below
	 *
	 * @returns the camera's view target reference information for updating the camera
	 */
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	
	/**
	 * The blueprint function for handling updating the player's camera. This is where you add different camera styles and determine what behavior the camera should take
	 * You're also able to add your own additional logic for handling the camera. \n\n
	 * 
	 * The order of operations for the UpdateViewTarget function is -> BlueprintUpdateCamera -> UpdateViewTarget -> UpdateViewTarget(Blueprint) \n
	 * If BlueprintUpdateCamera returns true, it skips everything else. If BP_UpdateViewTarget returns true, it skips UpdateViewTargetInternal and let's the player handle the camera logic instead
	 *
	 * @remarks I've found that the majority of the logic needed is smooth transitions and camera locations, which is handled when the camera style is set.
	 * The rest of the additional logic is in the @see TargetLockSpringArm component
	 *
	 * @param	OutVT				ViewTarget to update.
	 * @param	DeltaTime			Delta Time since last camera update (in seconds).
	 * @param	bApplyModifiers		whether UpdateViewTarget should apply camera modifiers
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera", DisplayName = "Update View Target (Blueprint)") void BP_UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime, bool& bApplyModifiers); 
	void BP_UpdateViewTarget_Implementation(FTViewTarget& OutVT, float DeltaTime, bool& bApplyModifiers);
	
	
	/**
	 * The camera behavior while the camera style is first person
	 * @remarks Overriding this in blueprint removes the original logic
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives") void FirstPersonCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void FirstPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	/**
	 * The camera behavior while the camera style is third person
	 * @remarks Overriding this in blueprint removes the original logic
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives") void ThirdPersonCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void ThirdPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);

	/**
	 * The camera behavior while the camera style is aiming (in third person)
	 * @remarks Overriding this in blueprint removes the original logic
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives") void ThirdPersonAimingCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void ThirdPersonAimingCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	/**
	 * The camera behavior while the camera style is target locking
	 * @remarks Overriding this in blueprint removes the original logic
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives") void TargetLockCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void TargetLockCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);

	/**
	 * The camera behavior while the camera style is spectator
	 * @remarks Overriding this in blueprint removes the original logic
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives") void SpectatorCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void SpectatorCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	
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
