// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GenericPlatform/GenericPlatformOutputDevices.h"
#include "HAL/PlatformOutputDevices.h"
#include "CoreGlobals.h"
#include "Misc/Parse.h"
#include "Templates/ScopedPointer.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceMemory.h"
#include "Misc/OutputDeviceFile.h"
#include "Misc/OutputDeviceDebug.h"
#include "Misc/OutputDeviceAnsiError.h"
#include "Misc/App.h"
#include "HAL/FeedbackContextAnsi.h"
#include "Misc/OutputDeviceConsole.h"
#include "Templates/UniquePtr.h"

TCHAR FGenericPlatformOutputDevices::CachedAbsoluteFilename[1024] = { 0 };

void FGenericPlatformOutputDevices::SetupOutputDevices()
{
	check(GLog);

	CachedAbsoluteFilename[0] = 0;

	GLog->AddOutputDevice(FPlatformOutputDevices::GetLog());

	TArray<FOutputDevice*> ChannelFileOverrides;
	FPlatformOutputDevices::GetPerChannelFileOverrides(ChannelFileOverrides);

	for (FOutputDevice* ChannelFileOverride : ChannelFileOverrides)
	{
		GLog->AddOutputDevice(ChannelFileOverride);
	}

#if !NO_LOGGING
	// if console is enabled add an output device, unless the commandline says otherwise...
	if (GLogConsole && !FParse::Param(FCommandLine::Get(), TEXT("NOCONSOLE")))
	{
		GLog->AddOutputDevice(GLogConsole);
	}
	
	// If the platform has a separate debug output channel (e.g. OutputDebugString) then add an output device
	// unless logging is turned off
	if (FPlatformMisc::HasSeparateChannelForDebugOutput())
	{
		GLog->AddOutputDevice(new FOutputDeviceDebug());
	}
#endif

	GLog->AddOutputDevice(FPlatformOutputDevices::GetEventLog());
};


FString FGenericPlatformOutputDevices::GetAbsoluteLogFilename()
{
	if (!CachedAbsoluteFilename[0])
	{
		FCString::Strcpy(CachedAbsoluteFilename, ARRAY_COUNT(CachedAbsoluteFilename), *FPaths::ProjectLogDir());
		FString LogFilename;
		if (!FParse::Value(FCommandLine::Get(), TEXT("LOG="), LogFilename))
		{
			if (FParse::Value(FCommandLine::Get(), TEXT("ABSLOG="), LogFilename))
			{
				CachedAbsoluteFilename[0] = 0;
			}
		}

		FString Extension(FPaths::GetExtension(LogFilename));
		if (Extension != TEXT("log") && Extension != TEXT("txt"))
		{
			// Ignoring the specified log filename because it doesn't have a .log extension			
			LogFilename.Empty();
		}

		if (LogFilename.Len() == 0)
		{
			if (FCString::Strlen(FApp::GetProjectName()) != 0)
			{
				LogFilename = FApp::GetProjectName();
			}
			else
			{
				LogFilename = TEXT("UE4");
			}

			LogFilename += TEXT(".log");
		}

		FCString::Strcat(CachedAbsoluteFilename, ARRAY_COUNT(CachedAbsoluteFilename) - FCString::Strlen(CachedAbsoluteFilename), *LogFilename);
	}

	return CachedAbsoluteFilename;
}

#ifndef WITH_LOGGING_TO_MEMORY
	#define WITH_LOGGING_TO_MEMORY 0
#endif

class FOutputDevice* FGenericPlatformOutputDevices::GetLog()
{
	static struct FLogOutputDeviceInitializer
	{
		TUniquePtr<FOutputDevice> LogDevice;
		FLogOutputDeviceInitializer()
		{
#if WITH_LOGGING_TO_MEMORY
#if !IS_PROGRAM && !WITH_EDITORONLY_DATA
			if (!LogDevice
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				&& FParse::Param(FCommandLine::Get(), TEXT("LOGTOMEMORY"))
#else
				&& !FParse::Param(FCommandLine::Get(), TEXT("NOLOGTOMEMORY")) && !FPlatformProperties::IsServerOnly()
#endif
				)
			{
				LogDevice = MakeUnique<FOutputDeviceMemory>();
			}
#endif // !IS_PROGRAM && !WITH_EDITORONLY_DATA
#endif // WITH_LOGGING_TO_MEMORY
			if (!LogDevice)
			{
#if (!UE_BUILD_SHIPPING) || PRESERVE_LOG_BACKUPS_IN_SHIPPING
				const bool bDisableBackup = false;
#else
				const bool bDisableBackup = true;
#endif
				LogDevice = MakeUnique<FOutputDeviceFile>(nullptr, bDisableBackup);
			}
		}

	} Singleton;

	return Singleton.LogDevice.Get();
}

void FGenericPlatformOutputDevices::GetPerChannelFileOverrides(TArray<FOutputDevice*>& OutputDevices)
{
	FString Commands;
	if (FParse::Value(FCommandLine::Get(), TEXT("logcategoryfiles="), Commands))
	{
		Commands = Commands.TrimQuotes();

		TArray<FString> Parts;
		Commands.ParseIntoArray(Parts, TEXT(","), true);

		for (FString Part : Parts)
		{
			FString Filename, CategoriesString;
			if (Part.TrimStartAndEnd().Split(TEXT("="), &Filename, &CategoriesString))
			{
				// do stuff
				FOutputDeviceFile* OutputDevice = new FOutputDeviceFile(*Filename);

				TArray<FString> Categories;
				CategoriesString.ParseIntoArray(Categories, TEXT("+"), true);

				for (FString Category : Categories)
				{
					OutputDevice->IncludeCategory(FName(*Category));
				}

				OutputDevices.Add(OutputDevice);
			}
		}
	}
}

FOutputDeviceError* FGenericPlatformOutputDevices::GetError()
{
	static FOutputDeviceAnsiError Singleton;
	return &Singleton;
}

FFeedbackContext* FGenericPlatformOutputDevices::GetFeedbackContext()
{
	static FFeedbackContextAnsi Singleton;
	return &Singleton;
}
