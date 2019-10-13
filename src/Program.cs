﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using Terumi.Binder;
using Terumi.Lexer;
using Terumi.Tokens;
using Terumi.Workspace;

namespace Terumi
{
	internal class Program
	{
		private static IEnumerable<IPattern> GetPatterns()
		{
			yield return new CharacterPattern('\n');
			yield return new WhitespacePattern();
			yield return new CommentPattern();

			yield return new KeywordPattern(new KeyValuePair<string, Keyword>[]
			{
				KeyValuePair.Create("contract", Keyword.Contract),
				KeyValuePair.Create("class", Keyword.Class),
				KeyValuePair.Create("readonly", Keyword.Readonly),
				KeyValuePair.Create("namespace", Keyword.Namespace),
				KeyValuePair.Create("using", Keyword.Using),
				KeyValuePair.Create("return", Keyword.Return),
				KeyValuePair.Create("this", Keyword.This),
			});

			yield return new CharacterPattern(';');
			yield return new CharacterPattern('@');

			yield return new CharacterPattern('=');
			yield return new CharacterPattern(',');
			yield return new CharacterPattern('.');

			yield return new CharacterPattern('(');
			yield return new CharacterPattern(')');

			yield return new CharacterPattern('[');
			yield return new CharacterPattern(']');

			yield return new CharacterPattern('{');
			yield return new CharacterPattern('}');

			yield return new CharacterPattern('+');
			yield return new CharacterPattern('-');
			yield return new CharacterPattern('/');
			yield return new CharacterPattern('*');

			yield return new IdentifierPattern(IdentifierCase.SnakeCase);
			yield return new IdentifierPattern(IdentifierCase.PascalCase);
			yield return new NumericPattern();
		}

		private static IEnumerable<Token> DebugTokenInfo(IEnumerable<Token> tokens)
		{
			foreach (var token in tokens)
			{
				Console.WriteLine(token.ToString());
				yield return token;
			}
		}

		/// <summary>
		/// Terumi application - WIP
		/// </summary>
		private static void Main(string[] args)
		{
			string file = default;
#if DEBUG
			if (file == default)
			{
				file = "sample_project";
			}
#endif
			var fs = new System.IO.Abstractions.FileSystem();

			if (!Project.TryLoad(file, fs, Git.Instance, out var project))
			{
				Console.WriteLine("Couldn't load project.");
				return;
			}

			Console.WriteLine("Loaded project.");

			var lexer = new StreamLexer(GetPatterns());
			var parser = new Parser.StreamParser();

			var parsedSourceFiles = project.ParseAllSourceFiles(lexer, parser).ToList();

			var binder = new BinderEnvironment(parsedSourceFiles);

			binder.PassOverTypeDeclarations();
			binder.PassOverMembers();
			binder.PassOverMethodBodies();

			// now we should be able to infer every type in every code body

#if DEBUG
			var jsonSerialized = Newtonsoft.Json.JsonConvert.SerializeObject(binder.TypeInformation, Newtonsoft.Json.Formatting.Indented, new Newtonsoft.Json.JsonSerializerSettings
			{
				ReferenceLoopHandling = Newtonsoft.Json.ReferenceLoopHandling.Serialize,
				PreserveReferencesHandling = Newtonsoft.Json.PreserveReferencesHandling.Objects
			});
			File.WriteAllText("binder_info.json", jsonSerialized);
#endif
		}
	}
}