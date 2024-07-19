// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraComponents/CameraPlayerInterface.h"

// Add default functionality here for any ICameraPlayerInterface functions that are not pure virtual.
FName ICameraPlayerInterface::GetCameraStyle_Implementation() const
{
	return FName();
}

ECameraOrientation ICameraPlayerInterface::GetCameraOrientation_Implementation() const
{
	return ECameraOrientation::Center;
}

void ICameraPlayerInterface::SetCameraStyle_Implementation(FName Style)
{
}

void ICameraPlayerInterface::SetCameraOrientation_Implementation(ECameraOrientation Orientation)
{
}
