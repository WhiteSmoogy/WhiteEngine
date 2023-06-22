namespace WSharpTool
{
	internal class Program
	{
		static void Main(string[] args)
		{
			var engineModule = new BuildModuleCPP();

			engineModule.IncludePaths.Add("Engine/Plugins");
			engineModule.IncludePaths.Add("SDKs");
			engineModule.IncludePaths.Add("SDKs/metis/5.1.0");
			engineModule.IncludePaths.Add("WBase");
			engineModule.IncludePaths.Add(".");

			engineModule.Definitions.Add("SPDLOG_COMPILED_LIB");
			engineModule.Definitions.Add("NGINE_TOOL");
			engineModule.Definitions.Add("INITGUID");
			engineModule.Definitions.Add("DEBUG");
			engineModule.Definitions.Add("WINDOWS");

			var toolchain = new LibClangToolChain();

			var rspFiles = engineModule.Compile(toolchain);

			var export = new CppSharpExport();

			var sharpFiles = export.Compile(rspFiles);

			var sharpInterface = new BuildModuleCSharp();
			var dllFiles = sharpInterface.Compile(sharpFiles);

			var import = new CSharpCppFunctionImport();

			var cppFiles = import.Compile(dllFiles);

			Console.WriteLine("Hello, World!");
		}
	}
}