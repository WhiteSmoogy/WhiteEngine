using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WSharpTool
{
	internal class BuildModuleCPP
	{
		public readonly HashSet<string> Definitions;

		public readonly HashSet<string> IncludePaths;

		public readonly HashSet<string> CPPFiles;

		public BuildModuleCPP()
		{
			Definitions = new HashSet<string>();
			IncludePaths = new HashSet<string>();
			CPPFiles = new HashSet<string>();
		}

		public List<string> Compile(LibClangToolChain ToolChain)
		{
			return CPPFiles.ToList();
		}
	}
}
