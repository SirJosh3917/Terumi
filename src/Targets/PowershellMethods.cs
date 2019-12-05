﻿using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

using Terumi.Binder;
using Terumi.VarCode;

namespace Terumi.Targets
{
	public class PowershellTarget : ICompilerTarget
	{
		public string ShellFileName => "out.ps1";

		private static string HandlePanic(List<string> a)
			=> $"Throw {a[0]}";

		public CompilerMethod? Match(string name, params IType[] types)
		{
			switch (name)
			{
				// TODO: supply code
				case TargetMethodNames.TargetName: return ReturnMethod(BuiltinType.String, _ => "powershell");
				case TargetMethodNames.Panic: return ReturnMethod(BuiltinType.Void, HandlePanic);

				case TargetMethodNames.IsSupported: return ReturnMethod(BuiltinType.Boolean, _ => "$TRUE"); // TODO: exauhstive
				case TargetMethodNames.Println: return ReturnMethod(BuiltinType.Void, a => $"Write-Host {a[0]}");
				case TargetMethodNames.Command: return ReturnMethod(BuiltinType.Void, a => $"iex \"& {a[0]}\"");

				case TargetMethodNames.OperatorNot: return ReturnMethod(BuiltinType.Boolean, a => $"-Not {a[0]}");

				case TargetMethodNames.OperatorEqualTo: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -eq {a[1]}");
				case TargetMethodNames.OperatorNotEqualTo: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -ne {a[1]}");

				case TargetMethodNames.OperatorLessThan: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -lt {a[1]}");
				case TargetMethodNames.OperatorGreaterThan: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -gt {a[1]}");
				case TargetMethodNames.OperatorLessThanOrEqualTo: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -le {a[1]}");
				case TargetMethodNames.OperatorGreaterThanOrEqualTo: return ReturnMethod(BuiltinType.Boolean, a => $"{a[0]} -ge {a[1]}");

				// TODO: verify that both operands are the same
				case TargetMethodNames.OperatorAdd: return ReturnMethod(types[0], a => $"{a[0]} + {a[1]}");
				case TargetMethodNames.OperatorSubtract: return ReturnMethod(types[0], a => $"{a[0]} - {a[1]}");
				case TargetMethodNames.OperatorMultiply: return ReturnMethod(types[0], a => $"{a[0]} * {a[1]}");
				case TargetMethodNames.OperatorDivide: return ReturnMethod(types[0], a => $"{a[0]} / {a[1]}");
				case TargetMethodNames.OperatorExponent: return ReturnMethod(types[0], a => $"[Math]::Pow({a[0]}, {a[1]})");
			}

			throw new NotImplementedException();
			CompilerMethod ReturnMethod(IType returnType, Func<List<string>, string> codegen) => new CompilerMethod(returnType, name, Match(types))
			{
				CodeGen = codegen
			};
		}

		private List<string> _run = new List<string>();

		private List<MethodParameter> Match(IType[] arguments)
			=> arguments.Select((x, i) => new MethodParameter(x, $"p{i}")).ToList();

		public CompilerMethod Panic(IType claimToReturn)
			=> new CompilerMethod(claimToReturn, TargetMethodNames.Panic, new List<MethodParameter> { new MethodParameter(BuiltinType.String, "panic_reason") })
			{
				CodeGen = HandlePanic
			};

		public void Write(IndentedTextWriter writer, List<VarCode.Method> methods)
		{
			int id = 0;
			foreach (var method in methods) method.Id = id++;

			writer.WriteLine($"$global:_gc = 0");

			foreach (var method in methods)
			{
				writer.WriteLine();
				writer.Write($"function {GetName(method.Id)}");

				if (method.Name == "<>main" && method.Parameters.Count == 0) _run.Add(GetName(method.Id));

				if (method.Parameters.Count > 0)
				{
					writer.Write($"(${GetName(0)}");

					for (var i = 1; i < method.Parameters.Count; i++)
					{
						writer.Write(", ");
						writer.Write('$');
						writer.Write(GetName(i));
					}

					writer.Write(')');
				}
				else
				{
					writer.Write("()");
				}

				writer.WriteLine('{');
				writer.Indent++;

				Write(writer, method, method.Parameters.Count);

				writer.Indent--;
				writer.WriteLine('}');
			}

			foreach (var run in _run)
			{
				writer.WriteLine();
				writer.Write(run);
			}
		}

		public void Write(IndentedTextWriter writer, VarCode.Method method, int offset)
			=> Write(writer, method.Code, offset);

		public void Write(IndentedTextWriter writer, List<Instruction> body, int offset)
		{
			foreach (var i in body)
			{
				Write(writer, i, offset);
			}
		}

		public void Write(IndentedTextWriter writer, Instruction instruction, int offset)
		{
			switch (instruction)
			{
				case Instruction.Load.String o:
				{
					writer.WriteLine($"${GetName(o.Store)} = \"{o.Value}\"");
				}
				break;

				case Instruction.Load.Parameter o:
				{
					writer.WriteLine($"${GetName(o.Store)} = ${PowershellTarget.GetName(o.ParameterNumber)}");
				}
				break;

				case Instruction.Load.Boolean o:
				{
					writer.WriteLine($"${GetName(o.Store)} = ${(o.Value ? "TRUE" : "FALSE")}");
				}
				break;

				case Instruction.Load.Number o:
				{
					writer.WriteLine($"${GetName(o.Store)} = {o.Value.Value.ToString()}");
				}
				break;

				case Instruction.Assign o:
				{
					writer.WriteLine($"${GetName(o.Store)} = ${GetName(o.Value)}");
				}
				break;

				case Instruction.New o:
				{
					writer.WriteLine($"${GetName(o.StoreId)} = $global:_gc++");
				}
				break;

				// TODO: figure this out lol
				case Instruction.SetField o:
				{
					writer.WriteLine($"Set-Variable -Scope 'Global' -Name \"${GetName(o.VariableId)}.{PowershellTarget.GetName(o.FieldId)}\" -Value ${GetName(o.ValueId)}");
				}
				break;

				case Instruction.GetField o:
				{
					writer.WriteLine($"${GetName(o.StoreId)} = Get-Variable -Scope 'Global' -ValueOnly -Name \"${GetName(o.VariableId)}.{PowershellTarget.GetName(o.FieldId)}\"");
				}
				break;

				case Instruction.Call o:
				{
					var args = o.Arguments.Count == 0 ? "" : o.Arguments.Select(x => "$" + GetName(x)).Aggregate((a, b) => $"{a} {b}");

					if (o.Store == Instruction.Nowhere)
					{
						writer.WriteLine($"{PowershellTarget.GetName(o.Method.Id)} {args}");
					}
					else
					{
						writer.WriteLine($"${GetName(o.Store)} = {PowershellTarget.GetName(o.Method.Id)} {args}");
					}
				}
				break;

				case Instruction.CompilerCall o:
				{
					var toNames = o.Arguments.Select(x => "$" + GetName(x)).ToList();

					if (o.Store == Instruction.Nowhere)
					{
						writer.WriteLine($"{o.CompilerMethod.CodeGen(toNames)}");
					}
					else
					{
						writer.WriteLine($"${GetName(o.Store)} = {o.CompilerMethod.CodeGen(toNames)}");
					}
				}
				break;

				case Instruction.Return o:
				{
					if (o.ValueId == Instruction.Nowhere)
					{
						writer.WriteLine("return");
					}
					else
					{
						writer.WriteLine($"return ${GetName(o.ValueId)}");
					}
				}
				break;

				case Instruction.If o:
				{
					writer.WriteLine($"if (${GetName(o.Variable)}) {{");

					writer.Indent++;
					Write(writer, o.Clause, offset);
					writer.Indent--;

					writer.WriteLine('}');
				}
				break;

				case Instruction.While o:
				{
					writer.WriteLine($"while (${GetName(o.Comparison)}) {{");

					writer.Indent++;
					Write(writer, o.Clause, offset);
					writer.Indent--;

					writer.WriteLine('}');
				}
				break;

				default: throw new NotImplementedException();
			}

			string GetName(int id) => PowershellTarget.GetName(id + offset);
		}

		private static string GetName(int id)
		{
			return new string(id.ToString().Select(ToChar).Prepend('t').ToArray());

			static char ToChar(char i)
				=> (i - '0') switch
				{
					0 => 'a',
					1 => 'b',
					2 => 'c',
					3 => 'd',
					4 => 'e',
					5 => 'f',
					6 => 'g',
					7 => 'h',
					8 => 'i',
					9 => 'j'
				};
		}
	}
}