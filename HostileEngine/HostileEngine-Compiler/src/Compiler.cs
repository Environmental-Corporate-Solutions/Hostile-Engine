using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Text;
using System.Linq;

namespace HostileEngine
{
    public static class Compiler
    {

        private static readonly CultureInfo Culture = CultureInfo.GetCultureInfo("en-US");

        private static readonly IEnumerable<string> DefaultNamespaces = new[]
        {
            "System",
            "System.IO",
            "System.Text",
        };

        private static readonly CSharpCompilationOptions DefaultCompilationOptions =
            new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
                .WithOverflowChecks(true).WithOptimizationLevel(OptimizationLevel.Release)
                .WithUsings(DefaultNamespaces);

        public static SyntaxTree Parse(string text, string filename = "", CSharpParseOptions options = null)
        {
            var stringText = SourceText.From(text, Encoding.Unicode);
            return SyntaxFactory.ParseSyntaxTree(stringText, options, filename);
        }

        public static SyntaxTree ParseFromFile(string fileToCompile, CSharpParseOptions options = null)
        {
            var source = File.ReadAllText(fileToCompile);
            var dir = new DirectoryInfo(fileToCompile);

            return Parse(source, dir.Name, options);
        }
        private static string GetMessagePrefix(Diagnostic diagnostic)
        {
            string prefix = "";
            switch (diagnostic.Severity)
            {
                case DiagnosticSeverity.Hidden:
                    prefix = "hidden";
                    break;
                case DiagnosticSeverity.Info:
                    prefix = "info";
                    break;
                case DiagnosticSeverity.Warning:
                    prefix = "warning";
                    break;
                case DiagnosticSeverity.Error:
                    prefix = "error";
                    break;
            }

            return string.Format("{0} {1}", prefix, diagnostic.Id);
        }
        private static string FormatErrorMessage(Diagnostic diagnostic)
        {

            switch (diagnostic.Location.Kind)
            {
                case LocationKind.SourceFile:
                case LocationKind.XmlFile:
                case LocationKind.ExternalFile:
                    var span = diagnostic.Location.GetLineSpan();
                    var mappedSpan = diagnostic.Location.GetMappedLineSpan();
                    if (!span.IsValid || !mappedSpan.IsValid)
                    {
                        goto default;
                    }

                    string path, basePath;
                    if (mappedSpan.HasMappedPath)
                    {
                        path = mappedSpan.Path;
                        basePath = span.Path;
                    }
                    else
                    {
                        path = span.Path;
                        basePath = null;
                    }

                    return string.Format("{0}{1}: {2}: {3}",
                        path,
                        string.Format("({0},{1})", mappedSpan.Span.Start.Line + 1, mappedSpan.Span.Start.Character + 1),
                        GetMessagePrefix(diagnostic),
                        diagnostic.GetMessage(Culture));

                default:
                    return string.Format("{0}: {1}",
                        GetMessagePrefix(diagnostic),
                        diagnostic.GetMessage(Culture));
            }
        }
        private static void _Compile(string basePath, List<string> dirs, IEnumerable<MetadataReference> references)
        {
            if (dirs.Count == 0) return;

            List<SyntaxTree> syntaxTrees = new List<SyntaxTree>(dirs.Count);

            foreach (string dir in dirs)
            {
                Console.WriteLine($"Parsing : {dir}");
                syntaxTrees.Add(ParseFromFile(dir,
                    CSharpParseOptions.Default));
            }

            var compilation
                = CSharpCompilation.Create("HostileEngineApp", syntaxTrees, references,
                    DefaultCompilationOptions);
            
            Console.WriteLine($"Compiling ...");

            var result = compilation.Emit($"{basePath}/HostileEngineApp.dll");

            if (!result.Success)
            {
                DiagnosticFormatter formatter=new DiagnosticFormatter();
                
                Console.Error.WriteLine("Compile Failed");
                IEnumerable<Diagnostic> failures = result.Diagnostics.Where(diagnostic =>
                    diagnostic.IsWarningAsError ||
                    diagnostic.Severity == DiagnosticSeverity.Error);

                foreach (Diagnostic diagnostic in failures)
                {
                    Console.WriteLine(FormatErrorMessage(diagnostic));
                }

                return;
            }

            Console.WriteLine($"Success! Exported at {basePath}/HostileEngineApp.dll");
        }

        public static int Compile(string basePath = ".", string monoRuntimePath = "mono/lib/mono/4.5")
        {
            try
            {
                Console.WriteLine($"Compile! Base Path : {basePath}");

                IEnumerable<MetadataReference> DefaultReferences = new[]
                {
                    MetadataReference.CreateFromFile($"{basePath}/HostileEngine-ScriptCore.dll"),
                    MetadataReference.CreateFromFile(typeof(Object).Assembly.Location), //mscorlib
                    MetadataReference.CreateFromFile($"{monoRuntimePath}/System.dll")
                };

                string[] list = Directory.GetFiles(basePath, "*.cs",
                    SearchOption.AllDirectories);
                _Compile(basePath, list.ToList(), DefaultReferences);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                return -1;
            }

            return 0;
        }

        //jun: only for debugging this will not be called when we load this on memory
        static public void Main(string[] args)
        {
            Compile();
        }
    }
}