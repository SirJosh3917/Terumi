﻿using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Text;

namespace Terumi.Workspace.TypePasser
{
	public class InfoItem
	{
		/// <summary>
		/// If this is true, <see cref="Namespace"/> is meaningless.
		/// </summary>
		public bool IsCompilerDefined { get; set; }

		public ICollection<ICollection<string>> NamespaceReferences = new List<ICollection<string>>(4);

		public ICollection<string> Namespace { get; set; } = new List<string>(5);

		public string Name { get; set; }

		public ICollection<Field> Fields { get; set; } = new List<Field>(5);

		public ICollection<Method> Methods { get; set; } = new List<Method>(10);

		public SyntaxTree.TypeDefinition TerumiBacking { get; set; }

		public bool IsContract { get; set; }

		public class Field
		{
			public InfoItem Type { get; set; }

			public string Name { get; set; }

			public SyntaxTree.Field TerumiBacking { get; set; }
		}

		public class Method
		{
			public string Name { get; set; }

			public InfoItem ReturnType { get; set; }

			public ICollection<Parameter> Parameters { get; set; }

			public ICollection<Ast.Code.CodeStatement> Statements { get; set; } = new List<Ast.Code.CodeStatement>();

			public SyntaxTree.Method TerumiBacking { get; set; }

			public class Parameter
			{
				public InfoItem Type { get; set; }

				public string Name { get; set; }

				public SyntaxTree.Parameter TerumiBacking { get; set; }
			}
		}
	}
}
