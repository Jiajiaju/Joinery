using UnrealBuildTool;

public class JoinerySkeletalRefit : ModuleRules
{
	public JoinerySkeletalRefit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AnimationCore",
				"AssetDefinition",
				"AssetTools",
				"CoreUObject",
				"Engine",
				"InputCore",
				"JoineryWorkbench",
				"MeshDescription",
				"PropertyEditor",
				"RenderCore",
				"SkeletalMeshDescription",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd"
			}
		);
	}
}
