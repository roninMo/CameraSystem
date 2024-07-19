#pragma once


#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerCameraTypes.generated.h"


/** The different camera styles */
#define CameraStyle_None FName("None")
#define CameraStyle_Fixed FName("Fixed")
#define CameraStyle_Spectator FName("Spectator")
#define CameraStyle_FirstPerson FName("FirstPerson")
#define CameraStyle_ThirdPerson FName("ThirdPerson")
#define CameraStyle_TargetLocking FName("TargetLocking")
#define CameraStyle_Aiming FName("Aiming")




/**
*	The third person orientation of the camera
*	This is used to determine what pivot location on the skeletal mesh is used for the camera position
*/
UENUM(BlueprintType)
enum class ECameraOrientation : uint8
{
	None						UMETA(DisplayName = "None"),
	Center						UMETA(DisplayName = "Center"),
	LeftShoulder				UMETA(DisplayName = "Left Shoulder"),
	RightShoulder				UMETA(DisplayName = "Right Shoulder")
};


/**
*	What direction is the transition from the previous target to the current target? Is it from the left side? 
*/
UENUM(BlueprintType)
enum class EPreviousTargetLockOrientation : uint8
{
	None						UMETA(DisplayName = "None"),
	Right						UMETA(DisplayName = "Right"),
	Left						UMETA(DisplayName = "Left"),
	Center						UMETA(DisplayName = "Center"),
	Back						UMETA(DisplayName = "Back"),
	Front						UMETA(DisplayName = "Front"),
};




/**
 * 
 */
UCLASS()
class CHARACTERCAMERASYSTEM_API UCameraPostProcessingSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPostProcessSettings HideCamera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FPostProcessSettings DefaultCameraSettings;
	
	
};




/*
* Target lock information for quickly finding how close a target is to the player, and how far to one side they are
*/
USTRUCT(BlueprintType)
struct FTargetLockInformation
{
	GENERATED_USTRUCT_BODY()
		FTargetLockInformation(
			AActor* Target = nullptr,
			const float DistanceToTarget = 0.0,
			const float AngleFromForwardVector = 0.0
		) :
    
		Target(Target),
		DistanceToTarget(DistanceToTarget),
		AngleFromForwardVector(AngleFromForwardVector)
	{}

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)                     AActor* Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)                     float DistanceToTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)                     float AngleFromForwardVector;
    
};
