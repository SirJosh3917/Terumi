﻿using System;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Text;

namespace Terumi.Lexer
{
	// https://www.craftinginterpreters.com/scanning.html

	public enum TokenType
	{
		IdentifierToken,
		NumberToken,
		StringToken,
		Comment,
		Whitespace,

		Comma, // ,
		OpenParen, CloseParen, // ( )
		OpenBracket, CloseBracket, // [ ]
		OpenBrace, CloseBrace, // { }
		EqualTo, Assignment, // == =
		NotEqualTo, Not, // != !
		GreaterThan, GreaterThanOrEqualTo, // > >=
		LessThan, LessThanOrEqualTo, // < <=
		Increment, Add, // ++ +
		Decrement, Subtract, // -- -
		Exponent, Multiply, // ** *
		Divide, // /
		At, Dot, // @ .

		Using, Namespace, Class, Contract,
		True, False,
		If, Else, For, While,
		Readonly,
		This,
	}

	public class Token
	{
		public Token
		(
			TokenType type,
			LexerMetadata positionStart,
			LexerMetadata positionEnd,

			// i don't really like this whole 'object' thing but
			// generics don't really help us out here
			object? data
		)
		{
			Type = type;
			PositionStart = positionStart;
			PositionEnd = positionEnd;
			Data = data;

			// some preventative measures since we have a yucky object
#if DEBUG
			switch (Type)
			{
				case TokenType.IdentifierToken when !(Data is string): throw new InvalidOperationException($"Cannot have identifier token without string data");
				case TokenType.IdentifierToken: break;

				case TokenType.StringToken when !(Data is StringData): throw new InvalidOperationException($"Must pass in a StringData for a string token");
				case TokenType.StringToken: break;

				case TokenType.Comment when !(Data is string): throw new InvalidOperationException($"Must pass in a string for a comment token");
				case TokenType.Comment: break;

				default: if (Data != null) throw new InvalidOperationException($"Token type {Type} is not allowed to have data"); break;
			}
#endif
		}

		public TokenType Type { get; }
		public LexerMetadata PositionStart { get; }
		public LexerMetadata PositionEnd { get; }
		public object? Data { get; }

		public override string ToString() => $"Token '{Type}' beginning {PositionStart} ending {PositionEnd} with data: {PositionEnd}";
	}

	// used to wrap around BigInteger
	// to prevent `using System.Numerics;`
	public class Number
	{
		public Number(BigInteger number)
		{
			Value = number;
		}

		public BigInteger Value { get; }
	}

	public class StringData
	{
		public class Interpolation
		{
			public Interpolation(int position, List<Token> tokens)
			{
				Position = position;
				Tokens = tokens;
			}

			public int Position { get; }
			public List<Token> Tokens { get; }
		}

		public StringData(StringBuilder stringValue, List<Interpolation> interpolations)
		{
			StringValue = stringValue;
			Interpolations = interpolations;
		}

		public StringBuilder StringValue { get; }
		public List<Interpolation> Interpolations { get; }
	}

	public ref struct TerumiLexer
	{
		private const MethodImplOptions MaxOpt = MethodImplOptions.AggressiveInlining | MethodImplOptions.AggressiveOptimization;
		public const int TabSize = 4;

		private Span<byte> _source;
		private readonly string _file;
		private int _line;
		private int _column;
		private int _offset;

		public LexerMetadata Metadata => new LexerMetadata { BinaryOffset = _offset, Line = _line, Column = _column, File = _file };

		public TerumiLexer(string file, Span<byte> source)
		{
			_source = source;
			_line = 1;
			_column = 1;
			_offset = 0;
			_file = file;
		}

		[MethodImpl(MaxOpt)]
		public Token NextToken()
		{
			var b = Peek();

			if (IsWhitespace(b)) return Whitespace();

			switch (b)
			{
				case (byte)'@': return Char(TokenType.At);
				case (byte)'.': return Char(TokenType.Dot);
				case (byte)'(': return Char(TokenType.OpenParen);
				case (byte)')': return Char(TokenType.CloseParen);
				case (byte)'[': return Char(TokenType.OpenBracket);
				case (byte)']': return Char(TokenType.CloseBracket);
				case (byte)'{': return Char(TokenType.OpenBrace);
				case (byte)'}': return Char(TokenType.CloseBrace);
				case (byte)'=': return IsNext((byte)'=') ? Char(TokenType.EqualTo) : Char(TokenType.Assignment);
				case (byte)'!': return IsNext((byte)'=') ? Char(TokenType.NotEqualTo) : Char(TokenType.Not);
				case (byte)'>': return IsNext((byte)'=') ? Char(TokenType.GreaterThanOrEqualTo) : Char(TokenType.GreaterThan);
				case (byte)'<': return IsNext((byte)'=') ? Char(TokenType.LessThanOrEqualTo) : Char(TokenType.LessThan);
				case (byte)'+': return IsNext((byte)'+') ? Char(TokenType.Increment) : Char(TokenType.Add);
				case (byte)'-': return IsNext((byte)'-') ? Char(TokenType.Decrement) : Char(TokenType.Subtract);
				case (byte)'*': return IsNext((byte)'*') ? Char(TokenType.Exponent) : (IsNext((byte)'/') ? MultilineComment() : Char(TokenType.Multiply));
				case (byte)'/': return IsNext((byte)'/') ? SinglelineComment() : Char(TokenType.Divide);
				case (byte)'"': return String();
				default:
				{
					// try for numbers
					if (IsNumber(b)) return Number();

					// try for keywords
					Token result = null;
					if (TryString(TokenType.Using, "using", ref result)
						|| TryString(TokenType.Namespace, "namespace", ref result)
						|| TryString(TokenType.Class, "class", ref result)
						|| TryString(TokenType.Contract, "contract", ref result)
						|| TryString(TokenType.True, "true", ref result)
						|| TryString(TokenType.False, "false", ref result)
						|| TryString(TokenType.If, "if", ref result)
						|| TryString(TokenType.Else, "else", ref result)
						|| TryString(TokenType.For, "for", ref result)
						|| TryString(TokenType.While, "while", ref result)
						|| TryString(TokenType.Readonly, "readonly", ref result)
						|| TryString(TokenType.This, "this", ref result))
					{
						return result;
					}

					// nothing else matched, must be an identifier
					if (IsIdentifierStart(b)) return Identifier();

					Unsupported($"Unrecognized byte '{(char)b}' at {Metadata}");
				}
				break;
			}

			Unsupported($"Unreachable");
			return null;
		}

		[MethodImpl(MaxOpt)]
		private Token Char(TokenType type)
		{
			var start = Metadata;
			Next();
			return new Token(type, start, Metadata, null);
		}

		[MethodImpl(MaxOpt)]
		private bool IsNext(byte b)
		{
			if (Peek() == b) { Next(); return true; }
			return false;
		}

		/* comments */

		[MethodImpl(MaxOpt)]
		public Token MultilineComment()
		{
			var now = Metadata;
			var capture = _source;
			bool hitEnd = false;

			// we want to capture the ending /*, so we only need to go past the /
			Next();

			while (!AtEnd() && !(hitEnd = EndOfMultilineComment())) Next();

			if (!hitEnd)
			{
				Unsupported($"Didn't get the end of a multiline comment");
			}

			var commentData = capture.Slice(0, _offset - now.BinaryOffset);
			var stringData = Encoding.UTF8.GetString(commentData);

			return new Token(TokenType.Comment, now, Metadata, stringData);
		}

		// modifies state to put us at the end of the comment
		[MethodImpl(MaxOpt)]
		private bool EndOfMultilineComment()
		{
			byte b = (byte)'\0';

			if (TryPeek(ref b) && b == (byte)'*'
				&& TryPeek(ref b, 1) && b == (byte)'/')
			{
				_source = _source.Slice(2);
				return true;
			}

			return false;
		}

		/* singleline noise */

		[MethodImpl(MaxOpt)]
		public Token SinglelineComment()
		{
			var now = Metadata;
			var capture = _source;
			bool hitEnd = false;

			// skip the //
			Next(); Next();

			while (!AtEnd() && !(hitEnd = Peek() == (byte)'\n')) Next();

			if (!hitEnd)
			{
				// on second thought, not getting the end of a singleline comment is ok
				// Unsupported($"Didn't get newline to end singleline comment");
			}

			var commentData = capture.Slice(0, _offset - now.BinaryOffset);
			var stringData = Encoding.UTF8.GetString(commentData);

			return new Token(TokenType.Comment, now, Metadata, stringData);
		}

		/* whitespace lol */

		[MethodImpl(MaxOpt)]
		public Token Whitespace()
		{
			var start = Metadata;

			while (!AtEnd() && IsWhitespace(Peek()))
			{
				Next();
			}

			return new Token(TokenType.Whitespace, start, Metadata, null);
		}

		/* string */

		[MethodImpl(MaxOpt)]
		public Token String()
		{
			var now = Metadata;
			bool hitEnd = false;

			var strb = new StringBuilder();
			var interpolations = new List<StringData.Interpolation>();

			// skip the first '"'
			Next();

			while (!AtEnd())
			{
				var b = Peek();
				Next();

				// TODO: in switch
				if (b == (byte)'"')
				{
					hitEnd = true;
					break;
				}

				switch (b)
				{
					case (byte)'{':
					{
						var tokens = new List<Token>();

						while (Peek() != (byte)'}')
						{
							tokens.Add(NextToken());
						}

						Next();

						interpolations.Add(new StringData.Interpolation(strb.Length, tokens));
					}
					break;

					case (byte)'\\':
					{
						if (AtEnd()) break;

						switch (Peek())
						{
							// TODO: full escaping of stuff
							case (byte)'"': strb.Append('"'); break;
							case (byte)'\\': strb.Append('\\'); break;
							case (byte)'{': strb.Append('{'); break;
							case (byte)'}': strb.Append('}'); break;
							default:
							{
								// TODO: error about unsupported escape sequence
								Unsupported($"");
							}
							break;
						}

						Next();
					}
					break;

					default: strb.Append((char)b); break;
				}
			}

			if (!hitEnd)
			{
				Unsupported($"Didn't hit matching end beginning at {now}");
			}

			return new Token(TokenType.StringToken, now, Metadata, new StringData(strb, interpolations));
		}

		/* numbers */

		[MethodImpl(MaxOpt)]
		public Token Number()
		{
			var start = Metadata;

			var strb = new StringBuilder();

			// TODO: minor opt?
			byte b = Peek();
			while (IsNumber(b))
			{
				strb.Append((char)b);
				Next();
			}

			var end = Metadata;
			var data = new Number(BigInteger.Parse(strb.ToString()));
			return new Token(TokenType.NumberToken, start, end, data);
		}

		/* try to consume multiple characters & get out a token type */

		[MethodImpl(MaxOpt)]
		public bool TryString(TokenType type, string tryFor, ref Token result)
		{
			if (_source.Length < tryFor.Length) return false;

			var cmp = _source.Slice(0, tryFor.Length + 1);

			// quickly fail if the first one fails
			// we don't want to setup a for loop if the string is bound to fail
			if (cmp[0] != (byte)tryFor[0]) return false;

			for (var i = 1; i < tryFor.Length; i++)
			{
				if (cmp[i] != (byte)tryFor[i]) return false;
			}

			// if we have search for 'for' but come across 'forever' we don't want to match 'forever' as 'for' 'ever'
			if (IsIdentifierStart(cmp[^1])) return false;

			var now = Metadata;
			NextMany(tryFor.Length);
			var end = Metadata;

			result = new Token(type, now, end, null);
			return true;
		}

		/* identifiers */

		[MethodImpl(MaxOpt)]
		public Token Identifier()
		{
			var start = Metadata;

			var strb = new StringBuilder();
			strb.Append((char)Peek());
			Next();

			var current = Peek();
			while (IsIdentifierStart(current) || IsNumber(current))
			{
				strb.Append((char)current);

				Next();
				current = Peek();
			}

			var end = Metadata;
			return new Token(TokenType.IdentifierToken, start, end, strb.ToString());
		}

		/* basic */

		[MethodImpl(MaxOpt)]
		public void NextMany(int amount)
		{
			for (var i = 0; i < amount; i++) Next();
		}

		[MethodImpl(MaxOpt)]
		public void Next()
		{
			switch (Peek())
			{
				case (byte)'\n':
				{
					_line++;
					_column = 1;
				}
				break;

				case (byte)'\t':
				{
					_column += TabSize;
				}
				break;

				default: _column++; break;
			}

			_offset++;
			_source = _source.Slice(1);
		}

		[MethodImpl(MaxOpt)]
		public byte Peek(int amt = 0)
		{
			if (AtEnd(amt)) Unsupported($"Cannot peek at end");
			return _source[amt];
		}

		[MethodImpl(MaxOpt)]
		public bool TryPeek(ref byte result, int amt = 0)
		{
			if (AtEnd(amt)) return false;
			result = _source[amt];
			return true;
		}

		[MethodImpl(MaxOpt)]
		public bool AtEnd() => AtEnd(amt: 0);

		[MethodImpl(MaxOpt)]
		private bool AtEnd(int amt) => _source.Length < amt + 1;

		[MethodImpl(MaxOpt)]
		private bool IsNumber(byte b) => b >= (byte)'0' && b <= (byte)'9';

		[MethodImpl(MaxOpt)]
		private bool IsWhitespace(byte b)
			=> b == (byte)' '
			|| b == (byte)'\t'
			|| b == (byte)'\r'
			|| b == (byte)'\n';

		[MethodImpl(MaxOpt)]
		private bool IsIdentifierStart(byte b)
			=> (b >= (byte)'a' && b <= (byte)'z')
			|| (b >= (byte)'A' && b <= (byte)'Z')
			|| b == (byte)'_';

		[MethodImpl(MethodImplOptions.NoInlining)]
		private void Unsupported(string reason) => throw new InvalidOperationException(reason);
	}
}
