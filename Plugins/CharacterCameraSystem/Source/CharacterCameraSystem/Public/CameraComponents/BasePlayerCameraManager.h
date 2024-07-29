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
 * The Character Camera System's Camera Manager. Camera character's player controllers should use this camera manager class
 *
 * A PlayerCameraManager is responsible for managing the camera for a particular
 * player. It defines the final view properties used by other systems (e.g. the renderer),
 * meaning you can think of it as your virtual eyeball in the world. It can compute the 
 * final camera properties directly, or it can arbitrate/blend between other objects or 
 * actors that influence the camera (e.g. blending from one CameraActor to another).
 * 
 * The PlayerCameraManagers primary external responsibility is to reliably respond to
 * various Get*() functions, such as GetCameraViewPoint. Most everything else is
 * implementation detail and overrideable by user projects.
 * 
 * By default, a PlayerCameraManager maintains a "view target", which is the primary actor
 * the camera is associated with. It can also apply various "post" effects to the final 
 * view state, such as camera animations, shakes, post-process effects or special 
 * effects such as dirt on the lens.
 *
 * @see https://docs.unrealengine.com/latest/INT/Gameplay/Framework/Camera/
 */
UCLASS()
class CHARACTERCAMERASYSTEM_API ABasePlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
protected:
	/**** Camera smoothing and transition values ****/
	/** The pivot lag speed used for handling camera drag smoothing and transition speeds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager|Offsets") FVector PivotLagSpeed;

	/** The blend duration during crouch transitions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager") float CrouchBlendDuration;
	/** The blend time of the current crouch transition */
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager") float CrouchBlendTime;

	/** The rotation lag speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Camera Manager") float RotationLagSpeed;

	/** The out of bounds lag speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=  "Player Camera Manager") float OutOfBoundsLagSpeed;

	/**** Camera values derived from the player on possess ****/
	// UPROPERTY(BlueprintReadWrite) ECameraStyle CamStyle;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager") ECameraOrientation CameraOrientation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager") TObjectPtr<ACharacterCameraLogic> Character;
	
	/** Camera view target values */
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FMinimalViewInfo PreviousView;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FVector CharacterLocation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FRotator CharacterRotation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FVector TargetLocation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FRotator TargetRotation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FVector SmoothTargetLocation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FRotator SmoothTargetRotation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FVector CalculatedLocation;
	UPROPERTY(BlueprintReadWrite, Category = "Player Camera Manager|Update View Target") FRotator CalculatedRotation;

	
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera", DisplayName = "Blueprint Update View Target") void BP_UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime, bool& bApplyModifiers); 
	void BP_UpdateViewTarget_Implementation(FTViewTarget& OutVT, float DeltaTime, bool& bApplyModifiers);
	
	
	/**
	 * The camera behavior while the camera style is first person
	 * @remarks Overriding this function replaces the default camera behavior
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives", DisplayName = "Camera Behavior (FirstPerson)") 
	void FirstPersonCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void FirstPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	/**
	 * The camera behavior while the camera style is third person
	 * @remarks Overriding this function replaces the default camera behavior
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives", DisplayName = "Camera Behavior (ThirdPerson)") 
	void ThirdPersonCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void ThirdPersonCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);

	/**
	 * The camera behavior while the camera style is aiming (in third person)
	 * @remarks Overriding this function replaces the default camera behavior
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives", DisplayName = "Camera Behavior (ThirdPerson - Aiming)") 
	void ThirdPersonAimingCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void ThirdPersonAimingCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	/**
	 * The camera behavior while the camera style is target locking
	 * @remarks Overriding this function replaces the default camera behavior
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives", DisplayName = "Camera Behavior (ThirdPerson - TargetLock)") 
	void TargetLockCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void TargetLockCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);

	/**
	 * The camera behavior while the camera style is spectator
	 * @remarks Overriding this function replaces the default camera behavior
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera|Perspectives", DisplayName = "Camera Behavior (Spectator)") 
	void SpectatorCameraBehavior(float DeltaTime, FTViewTarget& OutVT);
	virtual void SpectatorCameraBehavior_Implementation(float DeltaTime, FTViewTarget& OutVT);
	
	
//--------------------------------------------------------------------------------------------------//
// Camera calculation functions																		//
//--------------------------------------------------------------------------------------------------//
	/** Calculates a smooth interpolation between the camera's position and the target location */
	UFUNCTION(BlueprintCallable, Category = "Camera|Perspectives") virtual FVector CalculateCameraDrag(FVector Current, FVector Target, FRotator CameraRotation, float DeltaTime);

	/** Handle smooth transitions of crouch logic while the player is crouching in air */
	UFUNCTION(BlueprintCallable, Category = "Camera|Utilities") virtual void InAirCrouchLogic(FTViewTarget& OutVT, float DeltaTime);
	
	/** 
	 * Sets a new ViewTarget.
	 * @param NewViewTarget - New viewtarget actor.
	 * @param TransitionParams - Optional parameters to define the interpolation from the old viewtarget to the new. Transition will be instant by default.
	 */
	virtual void SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams) override;

	
};
