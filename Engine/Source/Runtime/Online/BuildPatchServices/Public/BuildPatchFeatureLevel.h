// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

namespace BuildPatchServices
{
	/**
	 * An enum type to describe supported features of a certain manifest.
	 */
	enum class EFeatureLevel : int32
	{
		// The original version.
		Original = 0,
		// Support for custom fields.
		CustomFields,
		// Started storing the version number.
		StartStoringVersion,
		// Made after data files where renamed to include the hash value, these chunks now go to ChunksV2.
		DataFileRenames,
		// Manifest stores whether build was constructed with chunk or file data.
		StoresIfChunkOrFileData,
		// Manifest stores group number for each chunk/file data for reference so that external readers don't need to know how to calculate them.
		StoresDataGroupNumbers,
		// Added support for chunk compression, these chunks now go to ChunkV3. NB: Not File Data Compression yet.
		ChunkCompressionSupport,
		// Manifest stores product prerequisites info.
		StoresPrerequisitesInfo,
		// Manifest stores chunk download sizes.
		StoresChunkFileSizes,
		// Manifest can optionally be stored using UObject serialization and compressed.
		StoredAsCompressedUClass,
		// These two features were removed and never used.
		UNUSED_0,
		UNUSED_1,
		// Manifest stores chunk data SHA1 hash to use in place of data compare, for faster generation.
		StoresChunkDataShaHashes,
		// Manifest stores Prerequisite Ids.
		StoresPrerequisiteIds,
		// The first minimal binary format was added. UObject classes will no longer be saved out when binary selected.
		StoredAsBinaryData,
		// Manifest can reference chunks with dynamic data size.
		VariableSizeChunks,

		// !! Always after the latest version entry, signifies the latest version plus 1 to allow the following Latest alias.
		LatestPlusOne,
		// An alias for the actual latest version value.
		Latest = (LatestPlusOne - 1),
		// An alias to provide the latest version of a manifest supported by file data (nochunks).
		LatestNoChunks = StoresChunkFileSizes,
		// An alias to provide the latest version of a manifest supported by a json serialized format.
		LatestJson = StoresPrerequisiteIds,
		// This is for UObject default, so that we always serialize it.
		Invalid = -1
	};

	/**
	 * Returns the string representation of the EFeatureLevel value. Used for analytics and logging only.
	 * @param FeatureLevel  The feature level enum value.
	 * @return The string representation.
	 */
	BUILDPATCHSERVICES_API const TCHAR* FeatureLevelToString(const EFeatureLevel& FeatureLevel);

	/**
	 * Parses the provided string into the relevant EFeatureLevel value, if it matches.
	 * @param FeatureLevelString    The string to try parse.
	 * @param FeatureLevel          Receives the enum value if successful.
	 * @return true if successfully parsed.
	 */
	BUILDPATCHSERVICES_API bool FeatureLevelFromString(const TCHAR* FeatureLevelString, EFeatureLevel& FeatureLevel);
}
