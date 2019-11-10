﻿using Terumi.Binder;

namespace Terumi.Ast
{
	public class VariableAssignment : ICodeExpression
	{
		public VariableAssignment(string variableName, ICodeExpression value)
		{
			VariableName = variableName;
			Value = value;
		}

		public UserType Type => Value.Type;

		public string VariableName { get; }
		public ICodeExpression Value { get; }
	}
}