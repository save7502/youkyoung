// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "NiagaraRenderer.h"
#include "ParticleResources.h"
#include "NiagaraSpriteVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"
#include "NiagaraEmitterInstanceBatcher.h"
#include "NiagaraSortingGPU.h"
#include "NiagaraCutoutVertexBuffer.h"
#include "RayTracingDefinitions.h"
#include "RayTracingDynamicGeometryCollection.h"
#include "RayTracingInstance.h"

DECLARE_CYCLE_STAT(TEXT("Generate Sprite Vertex Data [GT]"), STAT_NiagaraGenSpriteVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Sprites [RT]"), STAT_NiagaraRenderSprites, STATGROUP_Niagara);

DECLARE_CYCLE_STAT(TEXT("Genereate GPU Buffers"), STAT_NiagaraGenSpriteGpuBuffers, STATGROUP_Niagara);

struct FNiagaraDynamicDataSprites : public FNiagaraDynamicDataBase
{
	//Direct ptr to the dataset. ONLY FOR USE BE GPU EMITTERS.
	//TODO: Even this needs to go soon.
	const FNiagaraDataSet *DataSet;
};



/* Mesh collector classes */
class FNiagaraMeshCollectorResourcesSprite : public FOneFrameResource
{
public:
	FNiagaraSpriteVertexFactory VertexFactory;
	FNiagaraSpriteUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesSprite()
	{
		VertexFactory.ReleaseResource();
	}
};

NiagaraRendererSprites::NiagaraRendererSprites(ERHIFeatureLevel::Type FeatureLevel, UNiagaraRendererProperties *InProps) 
	: NiagaraRenderer()
	, PositionOffset(INDEX_NONE)
	, VelocityOffset(INDEX_NONE)
	, RotationOffset(INDEX_NONE)
	, SizeOffset(INDEX_NONE)
	, ColorOffset(INDEX_NONE)
	, FacingOffset(INDEX_NONE)
	, AlignmentOffset(INDEX_NONE)
	, SubImageOffset(INDEX_NONE)
	, MaterialParamOffset(INDEX_NONE)
	, MaterialParamOffset1(INDEX_NONE)
	, MaterialParamOffset2(INDEX_NONE)
	, MaterialParamOffset3(INDEX_NONE)
	, CameraOffsetOffset(INDEX_NONE)
	, UVScaleOffset(INDEX_NONE)
	, NormalizedAgeOffset(INDEX_NONE)
	, MaterialRandomOffset(INDEX_NONE)
	, CustomSortingOffset(INDEX_NONE)
	, LastSyncId(INDEX_NONE)
{
	//check(InProps);
	VertexFactory = new FNiagaraSpriteVertexFactory(NVFT_Sprite, FeatureLevel);
	Properties = Cast<UNiagaraSpriteRendererProperties>(InProps);
	BaseExtents = FVector(0.5f, 0.5f, 0.5f);

	if (Properties)
	{
		NumCutoutVertexPerSubImage = Properties->GetNumCutoutVertexPerSubimage();
		CutoutVertexBuffer.Data = Properties->GetCutoutData();
	}
}

void NiagaraRendererSprites::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();

	CutoutVertexBuffer.ReleaseResource();
#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.ReleaseResource();
		RayTracingDynamicVertexBuffer.Release();
	}
#endif
}

void NiagaraRendererSprites::CreateRenderThreadResources()
{
	CutoutVertexBuffer.InitResource();
	VertexFactory->InitResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingDynamicVertexBuffer.Initialize(4, 256, PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, TEXT("RayTracingDynamicVertexBuffer"));

		FRayTracingGeometryInitializer Initializer;
		Initializer.PositionVertexBuffer = nullptr;
		Initializer.IndexBuffer = nullptr;
		Initializer.BaseVertexIndex = 0;
		Initializer.VertexBufferStride = 12;
		Initializer.VertexBufferByteOffset = 0;
		Initializer.TotalPrimitiveCount = 0;
		Initializer.VertexBufferElementType = VET_Float3;
		Initializer.PrimitiveType = PT_TriangleList;
		Initializer.bFastBuild = true;
		Initializer.bAllowUpdate = false;
		RayTracingGeometry.SetInitializer(Initializer);
		RayTracingGeometry.InitResource();
	}
#endif
}

void NiagaraRendererSprites::ConditionalInitPrimitiveUniformBuffer(const FNiagaraSceneProxy *SceneProxy) const
{
	// Update the primitive uniform buffer if needed.
	if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
	{
		FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
			FMatrix::Identity,
			FMatrix::Identity,
			SceneProxy->GetActorPosition(),
			SceneProxy->GetBounds(),
			SceneProxy->GetLocalBounds(),
			SceneProxy->ReceivesDecals(),
			false,
			false,
			SceneProxy->UseSingleSampleShadowFromStationaryLights(),
			SceneProxy->GetScene().HasPrecomputedVolumetricLightmap_RenderThread(),
			SceneProxy->UseEditorDepthTest(),
			SceneProxy->GetLightingChannelMask(),
			0,
			INDEX_NONE,
			INDEX_NONE
		);
		WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
		WorldSpacePrimitiveUniformBuffer.InitResource();
	}
}

NiagaraRendererSprites::FCPUSimParticleDataAllocation NiagaraRendererSprites::ConditionalAllocateCPUSimParticleData(FNiagaraDynamicDataSprites* DynamicDataSprites, FGlobalDynamicReadBuffer& DynamicReadBuffer) const
{
	int32 TotalFloatSize = DynamicDataSprites->RTParticleData.GetFloatBuffer().Num() / sizeof(float);

	FCPUSimParticleDataAllocation CPUSimParticleDataAllocation { DynamicReadBuffer };

	if (DynamicDataSprites->DataSet->GetSimTarget() == ENiagaraSimTarget::CPUSim)
	{
		CPUSimParticleDataAllocation.ParticleData = DynamicReadBuffer.AllocateFloat(TotalFloatSize);
		FMemory::Memcpy(CPUSimParticleDataAllocation.ParticleData.Buffer, DynamicDataSprites->RTParticleData.GetFloatBuffer().GetData(), DynamicDataSprites->RTParticleData.GetFloatBuffer().Num());
	}

	return CPUSimParticleDataAllocation;
}

FNiagaraSpriteUniformBufferRef NiagaraRendererSprites::CreatePerViewUniformBuffer(const FSceneView* View, const FSceneViewFamily& ViewFamily, const FNiagaraSceneProxy *SceneProxy) const
{
	FNiagaraSpriteUniformParameters PerViewUniformParameters;
	PerViewUniformParameters.LocalToWorld = bLocalSpace ? SceneProxy->GetLocalToWorld() : FMatrix::Identity;//For now just handle local space like this but maybe in future have a VF variant to avoid the transform entirely?
	PerViewUniformParameters.LocalToWorldInverseTransposed = bLocalSpace ? SceneProxy->GetLocalToWorld().Inverse().GetTransposed() : FMatrix::Identity;
	PerViewUniformParameters.RotationBias = 0.0f;
	PerViewUniformParameters.RotationScale = 1.0f;
	PerViewUniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
	PerViewUniformParameters.DeltaSeconds = ViewFamily.DeltaWorldTime;
	PerViewUniformParameters.NormalsType = 0.0f;
	PerViewUniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
	PerViewUniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
	PerViewUniformParameters.PivotOffset = Properties->PivotInUVSpace * -1.0f; // We do this because we want to slide the coordinates back since 0,0 is the upper left corner.
	PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	PerViewUniformParameters.CameraFacingBlend = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
	PerViewUniformParameters.RemoveHMDRoll = Properties->bRemoveHMDRollInVR;
	PerViewUniformParameters.CustomFacingVectorMask = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageSize.X, Properties->SubImageSize.Y, 1.0f / Properties->SubImageSize.X, 1.0f / Properties->SubImageSize.Y);
	PerViewUniformParameters.PositionDataOffset = PositionOffset;
	PerViewUniformParameters.VelocityDataOffset = VelocityOffset;
	PerViewUniformParameters.RotationDataOffset = RotationOffset;
	PerViewUniformParameters.SizeDataOffset = SizeOffset;
	PerViewUniformParameters.ColorDataOffset = ColorOffset;
	PerViewUniformParameters.MaterialParamDataOffset = MaterialParamOffset;
	PerViewUniformParameters.MaterialParam1DataOffset = MaterialParamOffset1;
	PerViewUniformParameters.MaterialParam2DataOffset = MaterialParamOffset2;
	PerViewUniformParameters.MaterialParam3DataOffset = MaterialParamOffset3;
	PerViewUniformParameters.SubimageDataOffset = SubImageOffset;
	PerViewUniformParameters.FacingDataOffset = FacingOffset;
	PerViewUniformParameters.AlignmentDataOffset = AlignmentOffset;
	PerViewUniformParameters.SubImageBlendMode = Properties->bSubImageBlend;
	PerViewUniformParameters.CameraOffsetDataOffset = CameraOffsetOffset;
	PerViewUniformParameters.UVScaleDataOffset = UVScaleOffset;
	PerViewUniformParameters.NormalizedAgeDataOffset = NormalizedAgeOffset;
	PerViewUniformParameters.MaterialRandomDataOffset = MaterialRandomOffset;
	PerViewUniformParameters.DefaultPos = bLocalSpace ? FVector4(0.0f, 0.0f, 0.0f, 1.0f) : FVector4(SceneProxy->GetLocalToWorld().GetOrigin());

	ENiagaraSpriteFacingMode FacingMode = Properties->FacingMode;
	ENiagaraSpriteAlignment AlignmentMode = Properties->Alignment;

	if (FacingOffset == -1 && FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector)
	{
		FacingMode = ENiagaraSpriteFacingMode::FaceCamera;
	}

	if (AlignmentOffset == -1 && AlignmentMode == ENiagaraSpriteAlignment::CustomAlignment)
	{
		AlignmentMode = ENiagaraSpriteAlignment::Unaligned;
	}

	if (FacingMode == ENiagaraSpriteFacingMode::FaceCameraDistanceBlend)
	{
		float DistanceBlendMinSq = Properties->MinFacingCameraBlendDistance * Properties->MinFacingCameraBlendDistance;
		float DistanceBlendMaxSq = Properties->MaxFacingCameraBlendDistance * Properties->MaxFacingCameraBlendDistance;
		float InvBlendRange = 1.0f / FMath::Max(DistanceBlendMaxSq - DistanceBlendMinSq, 1.0f);
		float BlendScaledMinDistance = DistanceBlendMinSq * InvBlendRange;

		PerViewUniformParameters.CameraFacingBlend.X = 1.0f;
		PerViewUniformParameters.CameraFacingBlend.Y = InvBlendRange;
		PerViewUniformParameters.CameraFacingBlend.Z = BlendScaledMinDistance;
	}

	if (Properties->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
	{
		// velocity aligned
		PerViewUniformParameters.RotationScale = 0.0f;
		PerViewUniformParameters.TangentSelector = FVector4(0.0f, 1.0f, 0.0f, 0.0f);
	}

	if (Properties->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector)
	{
		PerViewUniformParameters.CustomFacingVectorMask = Properties->CustomFacingVectorMask;
	}

	return FNiagaraSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
}

void NiagaraRendererSprites::SetVertexFactoryParticleData(
	FNiagaraSpriteVertexFactory& OutVertexFactory, 
	FNiagaraDynamicDataSprites* DynamicDataSprites, 
	FCPUSimParticleDataAllocation& CPUSimParticleDataAllocation,
	const FSceneView* View,
	const FNiagaraSceneProxy *SceneProxy) const
{
	// Cutout geometry.
	const bool bUseSubImage = Properties->SubImageSize.X != 1 || Properties->SubImageSize.Y != 1;
	const bool bUseCutout = CutoutVertexBuffer.VertexBufferRHI.IsValid();
	if (bUseCutout)
	{	// Is Accessing Properties safe here? Or should values be cached in the constructor?
		if (bUseSubImage)
		{
			OutVertexFactory.SetCutoutParameters(NumCutoutVertexPerSubImage, CutoutVertexBuffer.VertexBufferSRV);
		}
		else // Otherwise simply replace the input stream with the single cutout geometry
		{
			OutVertexFactory.SetVertexBufferOverride(&CutoutVertexBuffer);
		}
	}

	//Sort particles if needed.
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy();
	EBlendMode BlendMode = MaterialRenderProxy->GetMaterial(VertexFactory->GetFeatureLevel())->GetBlendMode();
	OutVertexFactory.SetSortedIndices(nullptr, 0xFFFFFFFF);

	int32 NumInstances = DynamicDataSprites->RTParticleData.GetNumInstances();
	FNiagaraGPUSortInfo SortInfo;
	if (View && Properties->SortMode != ENiagaraSortMode::None && (BlendMode == BLEND_AlphaComposite || BlendMode == BLEND_Translucent || !Properties->bSortOnlyWhenTranslucent))
	{
		SortInfo.ParticleCount = NumInstances;
		SortInfo.SortMode = Properties->SortMode;
		SortInfo.SortAttributeOffset = (SortInfo.SortMode == ENiagaraSortMode::CustomAscending || SortInfo.SortMode == ENiagaraSortMode::CustomDecending) ? CustomSortingOffset : PositionOffset;
		SortInfo.ViewOrigin = View->ViewMatrices.GetViewOrigin();
		SortInfo.ViewDirection = View->GetViewDirection();
		if (bLocalSpace)
		{
			FMatrix InvTransform = SceneProxy->GetLocalToWorld().InverseFast();
			SortInfo.ViewOrigin = InvTransform.TransformPosition(SortInfo.ViewOrigin);
			SortInfo.ViewDirection = InvTransform.TransformVector(SortInfo.ViewDirection);
		}
	};

	if (DynamicDataSprites->DataSet->GetSimTarget() == ENiagaraSimTarget::CPUSim)//TODO: Compute shader for sorting gpu sims and larger cpu sims.
	{
		check(CPUSimParticleDataAllocation.ParticleData.IsValid());
		if (SortInfo.SortMode != ENiagaraSortMode::None && SortInfo.SortAttributeOffset != INDEX_NONE)
		{
			if (GNiagaraGPUSorting &&
				GNiagaraGPUSortingCPUToGPUThreshold != INDEX_NONE &&
				SortInfo.ParticleCount >= GNiagaraGPUSortingCPUToGPUThreshold &&
				SceneProxy->GetBatcher())
			{
				SortInfo.ParticleDataFloatSRV = CPUSimParticleDataAllocation.ParticleData.ReadBuffer->SRV;
				SortInfo.FloatDataOffset = CPUSimParticleDataAllocation.ParticleData.FirstIndex / sizeof(float);
				SortInfo.FloatDataStride = DynamicDataSprites->RTParticleData.GetFloatStride() / sizeof(float);
				const int32 IndexBufferOffset = SceneProxy->GetBatcher()->AddSortedGPUSimulation(SortInfo);
				if (IndexBufferOffset != INDEX_NONE)
				{
					OutVertexFactory.SetSortedIndices(SceneProxy->GetBatcher()->GetGPUSortedBuffer().VertexBufferSRV, IndexBufferOffset);
				}
			}
			else
			{
				FGlobalDynamicReadBuffer::FAllocation SortedIndices;
				SortedIndices = CPUSimParticleDataAllocation.DynamicReadBuffer.AllocateInt32(NumInstances);
				SortIndices(SortInfo.SortMode, SortInfo.SortAttributeOffset, DynamicDataSprites->RTParticleData, SceneProxy->GetLocalToWorld(), View, SortedIndices);
				OutVertexFactory.SetSortedIndices(SortedIndices.ReadBuffer->SRV, SortedIndices.FirstIndex / sizeof(float));
			}
		}
		OutVertexFactory.SetParticleData(CPUSimParticleDataAllocation.ParticleData.ReadBuffer->SRV, CPUSimParticleDataAllocation.ParticleData.FirstIndex / sizeof(float), DynamicDataSprites->RTParticleData.GetFloatStride() / sizeof(float));
	}
	else
	{
		if (SortInfo.SortMode != ENiagaraSortMode::None && SortInfo.SortAttributeOffset != INDEX_NONE && GNiagaraGPUSorting && SceneProxy->GetBatcher())
		{
			SortInfo.ParticleDataFloatSRV = DynamicDataSprites->DataSet->CurrData().GetGPUBufferFloat()->SRV;
			SortInfo.FloatDataOffset = 0;
			SortInfo.FloatDataStride = DynamicDataSprites->RTParticleData.GetFloatStride() / sizeof(float);
			SortInfo.GPUParticleCountSRV = DynamicDataSprites->DataSet->GetCurDataSetIndices().SRV;
			SortInfo.GPUParticleCountOffset = 1;
			const int32 IndexBufferOffset = SceneProxy->GetBatcher()->AddSortedGPUSimulation(SortInfo);
			if (IndexBufferOffset != INDEX_NONE)
			{
				OutVertexFactory.SetSortedIndices(SceneProxy->GetBatcher()->GetGPUSortedBuffer().VertexBufferSRV, IndexBufferOffset);
			}

		}

		OutVertexFactory.SetParticleData(DynamicDataSprites->DataSet->CurrData().GetGPUBufferFloat()->SRV, 0, DynamicDataSprites->DataSet->CurrData().GetFloatStride() / sizeof(float));
	}
}

void NiagaraRendererSprites::CreateMeshBatchForView(
	const FSceneView* View, 
	const FSceneViewFamily& ViewFamily, 
	const FNiagaraSceneProxy *SceneProxy,
	FNiagaraDynamicDataSprites *DynamicDataSprites,
	FMeshBatch& MeshBatch,
	FNiagaraMeshCollectorResourcesSprite& CollectorResources) const
{
	int32 NumInstances = DynamicDataSprites->RTParticleData.GetNumInstances();
	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy();

	ENiagaraSpriteFacingMode FacingMode = Properties->FacingMode;
	ENiagaraSpriteAlignment AlignmentMode = Properties->Alignment;

	CollectorResources.VertexFactory.SetAlignmentMode((uint32)AlignmentMode);
	CollectorResources.VertexFactory.SetFacingMode((uint32)FacingMode);
	CollectorResources.VertexFactory.SetParticleFactoryType(NVFT_Sprite);
	CollectorResources.VertexFactory.InitResource();
	CollectorResources.VertexFactory.SetSpriteUniformBuffer(CollectorResources.UniformBuffer);

	FNiagaraSpriteVFLooseParameters VFLooseParams;
	VFLooseParams.NumCutoutVerticesPerFrame = CollectorResources.VertexFactory.GetNumCutoutVerticesPerFrame();
	VFLooseParams.CutoutGeometry = CollectorResources.VertexFactory.GetCutoutGeometrySRV() ? CollectorResources.VertexFactory.GetCutoutGeometrySRV() : GFNiagaraNullCutoutVertexBuffer.VertexBufferSRV.GetReference();
	VFLooseParams.NiagaraParticleDataFloat = CollectorResources.VertexFactory.GetParticleDataFloatSRV();
	VFLooseParams.NiagaraFloatDataOffset = CollectorResources.VertexFactory.GetFloatDataOffset();
	VFLooseParams.NiagaraFloatDataStride = CollectorResources.VertexFactory.GetFloatDataStride();
	VFLooseParams.ParticleAlignmentMode = CollectorResources.VertexFactory.GetAlignmentMode();
	VFLooseParams.ParticleFacingMode = CollectorResources.VertexFactory.GetFacingMode();
	VFLooseParams.SortedIndices = CollectorResources.VertexFactory.GetSortedIndicesSRV() ? CollectorResources.VertexFactory.GetSortedIndicesSRV().GetReference() : GFNiagaraNullSortedIndicesVertexBuffer.VertexBufferSRV.GetReference();
	VFLooseParams.SortedIndicesOffset = CollectorResources.VertexFactory.GetSortedIndicesOffset();
	if (DynamicDataSprites->DataSet->GetSimTarget() == ENiagaraSimTarget::GPUComputeSim)
	{
		VFLooseParams.IndirectArgsBuffer = DynamicDataSprites->DataSet->GetCurDataSetIndices().SRV;
	}
	else
	{
		VFLooseParams.IndirectArgsBuffer = GFNiagaraNullSortedIndicesVertexBuffer.VertexBufferSRV;
	}

	CollectorResources.VertexFactory.LooseParameterUniformBuffer = FNiagaraSpriteVFLooseParametersRef::CreateUniformBufferImmediate(VFLooseParams, UniformBuffer_SingleFrame);

	MeshBatch.VertexFactory = &CollectorResources.VertexFactory;
	MeshBatch.CastShadow = SceneProxy->CastsDynamicShadow();
	MeshBatch.bUseAsOccluder = false;
	MeshBatch.ReverseCulling = SceneProxy->IsLocalToWorldDeterminantNegative();
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.DepthPriorityGroup = SceneProxy->GetDepthPriorityGroup(View);
	MeshBatch.bCanApplyViewModeOverrides = true;
	MeshBatch.bUseWireframeSelectionColoring = SceneProxy->IsSelected();
	MeshBatch.SegmentIndex = 0;

	if (bIsWireframe)
	{
		MeshBatch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
	}
	else
	{
		MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
	}

	FMeshBatchElement& MeshElement = MeshBatch.Elements[0];
	MeshElement.IndexBuffer = &GParticleIndexBuffer;
	MeshElement.FirstIndex = 0;
	MeshElement.NumPrimitives = 2;
	MeshElement.NumInstances = FMath::Max(0, NumInstances);	//->VertexData.Num();
	MeshElement.MinVertexIndex = 0;
	MeshElement.MaxVertexIndex = 0;// MeshElement.NumInstances * 4 - 1;
	MeshElement.PrimitiveUniformBuffer = WorldSpacePrimitiveUniformBuffer.GetUniformBufferRHI();
	if (DynamicDataSprites->DataSet->GetSimTarget() == ENiagaraSimTarget::GPUComputeSim)
	{
		MeshElement.IndirectArgsBuffer = DynamicDataSprites->DataSet->GetCurDataSetIndices().Buffer;
		MeshElement.NumPrimitives = 0;
	}

	if (NumCutoutVertexPerSubImage == 8)
	{
		MeshElement.NumPrimitives = 6;
		MeshElement.IndexBuffer = &GSixTriangleParticleIndexBuffer;
	}
}

void NiagaraRendererSprites::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderSprites);

	SimpleTimer MeshElementsTimer;

	//check(DynamicDataRender)

	FNiagaraDynamicDataSprites *DynamicDataSprites = static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);

	if (!DynamicDataSprites
		|| DynamicDataSprites->RTParticleData.GetNumInstancesAllocated() == 0
		|| DynamicDataSprites->RTParticleData.GetNumInstances() == 0
		|| nullptr == Properties
		|| !GSupportsResourceView // Current shader requires SRV to draw properly in all cases.
		)
	{
		return;
	}
	
	FCPUSimParticleDataAllocation CPUSimParticleDataAllocation = ConditionalAllocateCPUSimParticleData(DynamicDataSprites, Collector.GetDynamicReadBuffer());

	ConditionalInitPrimitiveUniformBuffer(SceneProxy);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			FNiagaraMeshCollectorResourcesSprite& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesSprite>();
			SetVertexFactoryParticleData(CollectorResources.VertexFactory, DynamicDataSprites, CPUSimParticleDataAllocation, View, SceneProxy);
			CollectorResources.UniformBuffer = CreatePerViewUniformBuffer(View, ViewFamily, SceneProxy);
			FMeshBatch& MeshBatch = Collector.AllocateMesh();
			CreateMeshBatchForView(View, ViewFamily, SceneProxy, DynamicDataSprites, MeshBatch, CollectorResources);

			Collector.AddMesh(ViewIndex, MeshBatch);
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}

#if RHI_RAYTRACING
void NiagaraRendererSprites::GetDynamicRayTracingInstances(FRayTracingMaterialGatheringContext& Context, TArray<FRayTracingInstance>& OutRayTracingInstances, const FNiagaraSceneProxy* SceneProxy)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderSprites);

	SimpleTimer MeshElementsTimer;

	FNiagaraDynamicDataSprites *DynamicDataSprites = static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);

	if (!DynamicDataSprites
		|| DynamicDataSprites->RTParticleData.GetNumInstancesAllocated() == 0
		|| DynamicDataSprites->RTParticleData.GetNumInstances() == 0
		|| nullptr == Properties
		|| !GSupportsResourceView // Current shader requires SRV to draw properly in all cases.
		)
	{
		return;
	}

	FRayTracingInstance RayTracingInstance;
	RayTracingInstance.Geometry = &RayTracingGeometry;
	RayTracingInstance.InstanceTransforms.Add(FMatrix::Identity);

	{
		// Setup material for our ray tracing instance
		FCPUSimParticleDataAllocation CPUSimParticleDataAllocation = ConditionalAllocateCPUSimParticleData(DynamicDataSprites, Context.RayTracingMeshResourceCollector.GetDynamicReadBuffer());
		ConditionalInitPrimitiveUniformBuffer(SceneProxy);
		FNiagaraMeshCollectorResourcesSprite& CollectorResources = Context.RayTracingMeshResourceCollector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesSprite>();
		SetVertexFactoryParticleData(CollectorResources.VertexFactory, DynamicDataSprites, CPUSimParticleDataAllocation, Context.ReferenceView, SceneProxy);
		CollectorResources.UniformBuffer = CreatePerViewUniformBuffer(Context.ReferenceView, Context.ReferenceViewFamily, SceneProxy);
		FMeshBatch MeshBatch;
		CreateMeshBatchForView(Context.ReferenceView, Context.ReferenceViewFamily, SceneProxy, DynamicDataSprites, MeshBatch, CollectorResources);

		ensureMsgf(MeshBatch.Elements[0].IndexBuffer != &GSixTriangleParticleIndexBuffer, TEXT("Cutout geometry is not supported by ray tracing"));

		RayTracingInstance.Materials.Add(MeshBatch);

		// Update dynamic ray tracing geometry
		Context.DynamicRayTracingGeometriesToUpdate.AddDynamicMeshBatchForGeometryUpdate(
			Context.Scene,
			Context.ReferenceView,
			SceneProxy,
			MeshBatch,
			RayTracingGeometry,
			6 * DynamicDataSprites->RTParticleData.GetNumInstances(),
			RayTracingDynamicVertexBuffer);
	}

	RayTracingInstance.BuildInstanceMaskAndFlags();

	OutRayTracingInstances.Add(RayTracingInstance);

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}
#endif

bool NiagaraRendererSprites::SetMaterialUsage()
{
	//Causes deadlock :S Need to look at / rework the setting of materials and render modules.
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraSprites);
}

void NiagaraRendererSprites::TransformChanged()
{
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraRendererSprites::GenerateVertexData(const FNiagaraSceneProxy* Proxy, FNiagaraDataSet &Data, const ENiagaraSimTarget Target)
{
	FNiagaraDynamicDataSprites *DynamicData = nullptr;

	if (bEnabled && Properties)
	{
		SimpleTimer VertexDataTimer;

		SCOPE_CYCLE_COUNTER(STAT_NiagaraGenSpriteVertexData);

		if (PositionOffset == INDEX_NONE || LastSyncId != Properties->SyncId)
		{			
			// optional attributes; we pass -1 as the offset so the VF can branch
			int32 IntDummy;
			Data.GetVariableComponentOffsets(Properties->PositionBinding.DataSetVariable, PositionOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->VelocityBinding.DataSetVariable, VelocityOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->SpriteRotationBinding.DataSetVariable, RotationOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->SpriteSizeBinding.DataSetVariable, SizeOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->ColorBinding.DataSetVariable, ColorOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->SpriteFacingBinding.DataSetVariable, FacingOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->SpriteAlignmentBinding.DataSetVariable, AlignmentOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->SubImageIndexBinding.DataSetVariable, SubImageOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->DynamicMaterialBinding.DataSetVariable, MaterialParamOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->DynamicMaterial1Binding.DataSetVariable, MaterialParamOffset1, IntDummy);
			Data.GetVariableComponentOffsets(Properties->DynamicMaterial2Binding.DataSetVariable, MaterialParamOffset2, IntDummy);
			Data.GetVariableComponentOffsets(Properties->DynamicMaterial3Binding.DataSetVariable, MaterialParamOffset3, IntDummy);
			Data.GetVariableComponentOffsets(Properties->CameraOffsetBinding.DataSetVariable, CameraOffsetOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->UVScaleBinding.DataSetVariable, UVScaleOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->NormalizedAgeBinding.DataSetVariable, NormalizedAgeOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->MaterialRandomBinding.DataSetVariable, MaterialRandomOffset, IntDummy);
			Data.GetVariableComponentOffsets(Properties->CustomSortingBinding.DataSetVariable, CustomSortingOffset, IntDummy);

			if (CustomSortingOffset == INDEX_NONE && (Properties->SortMode == ENiagaraSortMode::CustomAscending || Properties->SortMode == ENiagaraSortMode::CustomDecending))
			{
				UE_LOG(LogNiagara, Warning, TEXT("Niagara Sprite Emitter using custom sorting but does not have a valid custom sorting attribute binding."));
			}

			LastSyncId = Properties->SyncId;
		}
		
		if(Data.CurrData().GetNumInstances() > 0)
		{
			DynamicData = new FNiagaraDynamicDataSprites;

			//TODO: This buffer is far fatter than needed. Just pull out the data needed for rendering.
			Data.CurrData().CopyTo(DynamicData->RTParticleData);

			DynamicData->DataSet = &Data;
		}

		CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();
	}

	return DynamicData;  // for VF that can fetch from particle data directly
}



void NiagaraRendererSprites::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = NewDynamicData;
}

int NiagaraRendererSprites::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataSprites);
	return Size;
}

bool NiagaraRendererSprites::HasDynamicData()
{
//	return DynamicDataRender && static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender)->VertexData.Num() > 0;
	return DynamicDataRender!=nullptr;// && static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender)->NumInstances > 0;
}

#if WITH_EDITORONLY_DATA

const TArray<FNiagaraVariable>& NiagaraRendererSprites::GetRequiredAttributes()
{
	return Properties->GetRequiredAttributes();
}

const TArray<FNiagaraVariable>& NiagaraRendererSprites::GetOptionalAttributes()
{
	return Properties->GetOptionalAttributes();
}

#endif