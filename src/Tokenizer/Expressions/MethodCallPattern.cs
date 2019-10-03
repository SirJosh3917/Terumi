﻿using System;
using System.Collections.Generic;
using System.Text;
using Terumi.SyntaxTree.Expressions;
using Terumi.Tokens;

namespace Terumi.Parser.Expressions
{
	public class MethodCallPattern : IPattern<MethodCall>
	{
		private readonly IAstNotificationReceiver _astNotificationReceiver;
		private readonly IPattern<MethodCallParameterGroup> _pattern;

		public MethodCallPattern
		(
			IAstNotificationReceiver astNotificationReceiver,
			IPattern<MethodCallParameterGroup> pattern
		)
		{
			_astNotificationReceiver = astNotificationReceiver;
			_pattern = pattern;
		}

		public bool TryParse(ReaderFork<Token> source, out MethodCall item)
		{
			if (!source.TryNextNonWhitespace<IdentifierToken>(out var target)
				|| target.IdentifierCase != IdentifierCase.SnakeCase)
			{
				item = default;
				return false;
			}

			if (!source.TryNextCharacter('('))
			{
				_astNotificationReceiver.Throw("Expected open parenthesis");
				// TODO: exception - expected open parenthesis
				item = default;
				return false;
			}

			if (!_pattern.TryParse(source, out var methodCallParameterGroup))
			{
				// TODO: exception - expected method parameter call group
				// this pattern should handle no expressions fine though.
				_astNotificationReceiver.Throw("Expected method parameter call group");
				item = default;
				return false;
			}

			if (!source.TryNextCharacter(')'))
			{
				_astNotificationReceiver.Throw("Expected close parenthesis");
				// TODO: exception - expected close parenthesis
				item = default;
				return false;
			}

			item = new MethodCall(target, methodCallParameterGroup);
			_astNotificationReceiver.AstCreated(source, item);
			return true;
		}
	}
}
