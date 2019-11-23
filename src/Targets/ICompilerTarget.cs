﻿using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
using Terumi.Binder;

namespace Terumi.Targets
{
	public interface ICompilerTarget
	{
		CompilerMethod? Match(string name, List<Expression> arguments) => Match(name, arguments.Select(x => x.Type).ToArray());
		CompilerMethod? Match(string name, params IType[] types);

		void Write(IndentedTextWriter writer, List<VarCode.Method> methods);
	}
}