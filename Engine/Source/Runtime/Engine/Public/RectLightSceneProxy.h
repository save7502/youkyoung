// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RectLightSceneProxy.h: FRectLightSceneProxy definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Components/RectLightComponent.h"
#include "LocalLightSceneProxy.h"
#include "SceneManagement.h"

class FRectLightSceneProxy : public FLocalLightSceneProxy
{
public:
	float		SourceWidth;
	float		SourceHeight;
	UTexture*	SourceTexture;
#if RHI_RAYTRACING
	FRWBuffer   RectLightMipTree;
	FIntVector  RectLightMipTreeDimensions;
#endif

	FRectLightSceneProxy(const URectLightComponent* Component);
	virtual ~FRectLightSceneProxy();

	virtual bool IsRectLight() const override;

	virtual bool HasSourceTexture() const override;

	/** Accesses parameters needed for rendering the light. */
	virtual void GetLightShaderParameters(FLightShaderParameters& LightParameters) const override;

	/**
	* Sets up a projected shadow initializer for shadows from the entire scene.
	* @return True if the whole-scene projected shadow should be used.
	*/
	virtual bool GetWholeSceneProjectedShadowInitializer(const FSceneViewFamily& ViewFamily, TArray<FWholeSceneProjectedShadowInitializer, TInlineAllocator<6> >& OutInitializers) const;

#if RHI_RAYTRACING
	void BuildRectLightMipTree(FRHICommandListImmediate& RHICmdList);
#endif
};