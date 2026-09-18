using UnrealBuildTool;
using System.IO;

public class Hearthward : ModuleRules
{
    public Hearthward(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput"
            });
        PrivateDependencyModuleNames.AddRange(new[] { "HTTP", "Json", "Sockets", "UMG", "Slate", "SlateCore" });
        string Bundle = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Runtime/LocalAI"));
        if (Target.Type != TargetType.Editor && !File.Exists(Path.Combine(Bundle, "models/Qwen3.5-4B-Q4_K_M.gguf")))
            throw new BuildException("Hearthward requires its local AI bundle. Run scripts/local_ai/prepare_bundle.py before packaging.");
        RuntimeDependencies.Add(Path.Combine(Bundle, "models/Qwen3.5-4B-Q4_K_M.gguf"), StagedFileType.NonUFS);
        foreach (string Backend in new[] { "cpu", "vulkan" })
        {
            if (Target.Type != TargetType.Editor && !File.Exists(Path.Combine(Bundle, "bin", Backend, "llama-server.exe")))
                throw new BuildException("Hearthward local AI runtime is incomplete. Run scripts/local_ai/prepare_bundle.py.");
            RuntimeDependencies.Add(Path.Combine(Bundle, "bin", Backend, "LICENSE-LLVM-OpenMP"), StagedFileType.NonUFS);
        }
        if (Directory.Exists(Bundle))
        {
            foreach (string FilePath in Directory.GetFiles(Bundle, "*", SearchOption.AllDirectories))
            {
                string Relative = Path.GetRelativePath(Bundle, FilePath).Replace('\\', '/');
                if (Relative.StartsWith(".downloads/") || Relative.EndsWith(".part") || Relative.EndsWith(".gguf") || Relative.EndsWith("LICENSE-LLVM-OpenMP")) continue;
                if (Relative.EndsWith(".exe") && Path.GetFileName(FilePath) != "llama-server.exe") continue;
                RuntimeDependencies.Add(FilePath, StagedFileType.NonUFS);
            }
        }
    }
}
