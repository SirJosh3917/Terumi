﻿using Terumi.Ast;
using Terumi.Tokens;

namespace Terumi.Tokenizer
{
	public class ContractMethodPattern : IPattern<Method>
	{
		private readonly IAstNotificationReceiver _astNotificationReceiver;
		private readonly IPattern<ParameterGroup> _parameterPattern;

		public ContractMethodPattern
		(
			IAstNotificationReceiver astNotificationReceiver,
			IPattern<ParameterGroup> parameterPattern
		)
		{
			_astNotificationReceiver = astNotificationReceiver;
			_parameterPattern = parameterPattern;
		}

		public bool TryParse(ReaderFork<Token> source, out Method item)
		{
			if (!source.TryNextNonWhitespace<IdentifierToken>(out var identifierOrType))
			{
				item = default;
				return false;
			}

			int peeked;
			CharacterToken characterToken;

			if (source.TryPeekNonWhitespace<CharacterToken>(out characterToken, out peeked)
				&& characterToken.IsChar('('))
			{
				// TODO: incorrect identifier for identifierOrType exception if != snake case
				source.Advance(peeked);

				// now it's method body time
				return ParameterParsingStage(source, new IdentifierToken("void", IdentifierCase.SnakeCase), identifierOrType, out item);
			}
			else if (source.TryPeekNonWhitespace<IdentifierToken>(out var identifierToken, out peeked))
			{
				// TODO: incorrect identifier for identifierToken exception if != snake case
				source.Advance(peeked);

				if (!(source.TryNextNonWhitespace<CharacterToken>(out characterToken)
					&& characterToken.IsChar('(')))
				{
					// TODO: exception for no open parenthesis
					item = default;
					return false;
				}

				return ParameterParsingStage(source, identifierOrType, identifierToken, out item);
			}

			// TODO: exception - fields should've gone first so this *has* to be a method, or it's invalid
			item = default;
			return false;
		}

		private bool ParameterParsingStage(ReaderFork<Token> source, IdentifierToken type, IdentifierToken name, out Method item)
		{
			if (!_parameterPattern.TryParse(source, out var parameterGroup))
			{
				// TODO: throw an exception, we couldn't've possibly've gotten this far without parsing something
				item = default;
				return false;
			}

			if (!(source.TryNextNonWhitespace<CharacterToken>(out var characterToken)
				&& characterToken.IsChar(')')))
			{
				// TODO: throw exception, must have a closing parenthesis on a contract method
				item = default;
				return false;
			}

			if (!(source.TryNextNonPredicate(tkn => tkn is WhitespaceToken, out var tkn)
				&& tkn.IsNewline()))
			{
				// TODO: throw exception
				// you must have a newline to end contract method
				item = default;
				return false;
			}

			// contracts never have method bodies
			item = new Method(type, name, parameterGroup, null);
			_astNotificationReceiver.AstCreated(source, item);
			return true;
		}
	}
}