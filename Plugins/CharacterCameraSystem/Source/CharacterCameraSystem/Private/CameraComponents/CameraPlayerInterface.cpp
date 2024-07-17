// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraComponents/CameraPlayerInterface.h"

// Add default functionality here for any ICameraPlayerInterface functions that are not pure virtual.
ECameraStyle ICameraPlayerInterface::GetCameraStyle_Implementation() const
{
	return ECameraStyle::None;
}

ECameraOrientation ICameraPlayerInterface::GetCameraOrientation_Implementation() const
{
	return ECameraOrientation::Center;
}

FName ICameraPlayerInterface::GetCameraSocket_Implementation(ECameraStyle Style) const
{
	return FName();
}

void ICameraPlayerInterface::SetCameraStyle_Implementation(ECameraStyle Style)
{
}

void ICameraPlayerInterface::SetCameraOrientation_Implementation(ECameraOrientation Orientation)
{
}
