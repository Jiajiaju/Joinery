using UnrealBuildTool;

public class JoineryWorkbench : ModuleRules
{
	public JoineryWorkbench(ReadOnlyTargetRules Target) : base(Target)
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
				"CoreUObject",
				"Engine",
				"Projects",
				"Slate",
				"SlateCore",
				"ToolMenus"
			}
		);
	}
}
