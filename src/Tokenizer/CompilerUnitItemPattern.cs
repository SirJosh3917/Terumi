﻿using Terumi.Ast;
using Terumi.Tokens;

namespace Terumi.Tokenizer
{
	public class CompilerUnitItemPattern : IPattern<CompilerUnitItem>
	{
		private readonly IPattern<CompilerUnitItem> _coagulation;

		public CompilerUnitItemPattern
		(
			IPattern<TypeDefinition> typeDefinitionPattern,
			IPattern<PackageLevel> packagePattern
		)
		{
			_coagulation = new CoagulatedPattern<TypeDefinition, PackageLevel, CompilerUnitItem>
			(
				typeDefinitionPattern,
				packagePattern
			);
		}

		public bool TryParse(ReaderFork<Token> source, out CompilerUnitItem item)
			=> _coagulation.TryParse(source, out item);
	}
}