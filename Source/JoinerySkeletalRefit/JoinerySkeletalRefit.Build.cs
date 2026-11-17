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
				"AdvancedPreviewScene",
				"AssetRegistry",
				"AssetTools",
				"ClothingSystemRuntimeCommon",
				"ClothingSystemRuntimeInterface",
				"CoreUObject",
				"Engine",
				"HairStrandsCore",
				"InputCore",
				"JoineryCore",
				"MeshDescription",
				"PropertyEditor",
				"RenderCore",
				"SkeletalMeshDescription",
				"Slate",
				"SlateCore",
				"StaticMeshDescription",
				"StatusBar",
				"ToolMenus",
				"UnrealEd"
			}
		);
	}
}
