// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieSceneEnumTrack.generated.h"

class UEnum;

/**
 * Handles manipulation of byte properties in a movie scene
 */
UCLASS()
class MOVIESCENETRACKS_API UMovieSceneEnumTrack : public UMovieScenePropertyTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual void PostLoad() override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

	void SetEnum(UEnum* Enum);

	class UEnum* GetEnum() const;

protected:
	UPROPERTY()
	UEnum* Enum;
};
