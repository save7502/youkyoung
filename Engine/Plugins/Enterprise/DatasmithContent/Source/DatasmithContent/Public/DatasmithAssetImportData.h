// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DatasmithImportOptions.h"

#include "EditorFramework/AssetImportData.h"

#include "DatasmithAssetImportData.generated.h"

UCLASS()
class DATASMITHCONTENT_API UDatasmithAssetImportData : public UAssetImportData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(BlueprintReadWrite, Category = Asset, meta = (ShowOnlyInnerProperties))
	FDatasmithAssetImportOptions AssetImportOptions;
#endif		// WITH_EDITORONLY_DATA
};

UCLASS()
class DATASMITHCONTENT_API UDatasmithStaticMeshImportData : public UDatasmithAssetImportData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, Category = StaticMesh, meta = (ShowOnlyInnerProperties))
	FDatasmithStaticMeshImportOptions ImportOptions;

public:
	typedef TPair< FDatasmithStaticMeshImportOptions, FDatasmithAssetImportOptions > DefaultOptionsPair;
	static UDatasmithStaticMeshImportData* GetImportDataForStaticMesh( UStaticMesh* StaticMesh, TOptional< DefaultOptionsPair > DefaultImportOptions = TOptional< DefaultOptionsPair >() );

#endif		// WITH_EDITORONLY_DATA
};

UCLASS(HideCategories = (InternalProperty))
class DATASMITHCONTENT_API UDatasmithStaticMeshCADImportData : public UDatasmithStaticMeshImportData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, Category = StaticMesh, meta = (ShowOnlyInnerProperties))
	FDatasmithTessellationOptions TessellationOptions;

	UPROPERTY(VisibleDefaultsOnly, Category = InternalProperty)
	double	ModelUnit;

	UPROPERTY(VisibleDefaultsOnly, Category = InternalProperty)
	double	ModelTolerance;

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = InternalProperty)
	FString	ResourcePath;

	UPROPERTY(VisibleDefaultsOnly, Category = InternalProperty)
	FString	ResourceFilename;

	UPROPERTY(VisibleDefaultsOnly, Category = InternalProperty)
	TArray<FString>	AuxiliaryFilenames;

public:
	typedef TTuple<FDatasmithTessellationOptions, FDatasmithStaticMeshImportOptions, FDatasmithAssetImportOptions> DefaultOptionsTuple;
	static UDatasmithStaticMeshCADImportData* GetCADImportDataForStaticMesh( UStaticMesh* StaticMesh, TOptional< DefaultOptionsTuple > DefaultImportOptions = TOptional< DefaultOptionsTuple >() );

	void SetResourcePath(const FString& FilePath);
	const FString& GetResourcePath();
#endif		// WITH_EDITORONLY_DATA

protected:
	/** Overridden serialize function to read in/write out the unexposed data */
	virtual void Serialize(FArchive& Ar) override;
};

/**
 * Base class for import data and options used when importing any asset from Datasmith
 */
UCLASS()
class DATASMITHCONTENT_API UDatasmithSceneImportData : public UAssetImportData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, Category = "Options", meta = (ShowOnlyInnerProperties))
	FDatasmithImportBaseOptions BaseOptions;

#endif // WITH_EDITORONLY_DATA
};

/**
 * Import data and options specific to tessellated Datasmith scenes
 */
UCLASS()
class DATASMITHCONTENT_API UDatasmithCADImportSceneData : public UDatasmithSceneImportData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, Category = "Tessellation", meta = (ShowOnlyInnerProperties))
	FDatasmithTessellationOptions TessellationOptions;
#endif		// WITH_EDITORONLY_DATA
};

UCLASS()
class DATASMITHCONTENT_API UDatasmithMDLSceneImportData : public UDatasmithSceneImportData
{
	GENERATED_BODY()
};

UCLASS(EditInlineNew)
class DATASMITHCONTENT_API UDatasmithDeltaGenAssetImportData : public UDatasmithAssetImportData
{
	GENERATED_BODY()
};

UCLASS(EditInlineNew)
class DATASMITHCONTENT_API UDatasmithDeltaGenSceneImportData : public UDatasmithSceneImportData
{
	GENERATED_BODY()
};

UENUM(BlueprintType)
enum class EVREDDataTableType : uint8
{
	NotDatatable,
	Variants,
	AnimClips,
	AnimNodes
};

UCLASS(EditInlineNew)
class DATASMITHCONTENT_API UDatasmithVREDAssetImportData : public UDatasmithAssetImportData
{
	GENERATED_BODY()

public:
	EVREDDataTableType DataTableType;
};

UCLASS(EditInlineNew)
class DATASMITHCONTENT_API UDatasmithVREDSceneImportData : public UDatasmithSceneImportData
{
	GENERATED_BODY()

// TODO
/*#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, Category="VREDOptions", meta=(ShowOnlyInnerProperties))
	class UDatasmithVREDImportOptions* VREDOptions;

#endif // WITH_EDITORONLY_DATA*/
};


