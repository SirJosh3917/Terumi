﻿using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Terumi.Workspace
{
	public class Project
	{
		public const string TerumiFileEnding = "trm";
		public const string TerumiConfigEnding = "toml";

		public static bool TryLoad(string basePath, string projectName, out Project sourceProject)
		{
			sourceProject = default;

			Log.Debug($"Attempting to load '{projectName}'@'{basePath}'");

			if (!File.Exists(basePath))
			{
				Log.Error($"Path doesn't exist: '{basePath}'");
				return false;
			}

			var projectPath = Path.GetFullPath(Path.Combine(basePath, projectName));

			if (!File.Exists(projectPath))
			{
				Log.Error($"Path to project doesn't exist: '{projectPath}'");
				return false;
			}

			if (!GetSourceFiles(projectPath).Any())
			{
				Log.Error($"No source files in project path: '{projectPath}'. Ensure that there is at least one file ending in '.{TerumiFileEnding}'");
				return false;
			}

			var configPath = Path.GetFullPath(Path.Combine(basePath, projectName + $".{TerumiConfigEnding}"));
			var config = Configuration.Default;

			if (File.Exists(configPath))
			{
				Log.Debug($"Loaded config for '{projectName}'@'{configPath}'");

				config = Configuration.ReadFile(configPath);
			}

			sourceProject = new Project(basePath, projectName, config, projectPath);
			return true;
		}

		public Project
		(
			string basePath,
			string projectName,
			Configuration configuration,
			string? projectPath = null
		)
		{
			BasePath = basePath;
			ProjectName = projectName;
			ProjectPath = projectPath ?? Path.GetFullPath(Path.Combine(BasePath, ProjectName));
			Configuration = configuration;
		}

		public string BasePath { get; }
		public string ProjectName { get; }
		public string ProjectPath { get; }
		public Configuration Configuration { get; }

		public IEnumerable<Project> ResolveDependencies(DependencyResolver resolver)
		{
			foreach (var dependency in Configuration.Libraries)
			{
				foreach (var project in resolver.Resolve(dependency))
				{
					yield return project;
				}
			}
		}

		public IEnumerable<ProjectFile> GetSources()
		{
			foreach(var file in GetSourceFiles(ProjectPath))
			{
				var source = File.ReadAllText(file);

				// extract out the base path from the file path
				var packageLevel =

					// take out the base path to the project
					file.Substring(BasePath.Length)

					// now we should have something like 'terumi_sdk/json/reader.trm'
					.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)

					// get rid of 'reader.trm'
					.ExcludeLast();

				yield return new ProjectFile(source, packageLevel);
			}
		}

		private static IEnumerable<string> GetSourceFiles(string projectPath)
		{
			foreach (var file in RecursivelySearch(projectPath))
			{
				if (Path.GetExtension(file) == TerumiFileEnding)
				{
					yield return file;
				}
			}
		}

		private static IEnumerable<string> RecursivelySearch(string folder)
		{
			foreach (var file in Directory.GetFiles(folder))
			{
				yield return file;
			}

			foreach (var directory in Directory.GetDirectories(folder))
			{
				foreach (var file in RecursivelySearch(directory))
				{
					yield return file;
				}
			}
		}
	}
}
