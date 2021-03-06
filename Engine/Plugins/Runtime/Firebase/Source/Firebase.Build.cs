// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class Firebase : ModuleRules
	{
		public Firebase(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "Launch"
            });

            PublicDefinitions.Add("WITH_FIREBASE=1");

            if (Target.Platform == UnrealTargetPlatform.Android)
			{
				PrivateDependencyModuleNames.Add("Launch");

				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "Firebase.upl.xml"));
			}

			PublicIncludePathModuleNames.Add("Launch");
		}
	}
}
