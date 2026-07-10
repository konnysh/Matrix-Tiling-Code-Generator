// My laptop's l1D-cache size is 32KiB so estimated block size is 
// BS <= sqrt(32768 / 24) ≒ 36.9

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

std::string emitIndent(int indent_level) {
  constexpr int num_spaces = 2;
  return std::string(num_spaces * indent_level, ' ');
}

// for (type counter = 0; counter < cmp; ++counter)
// or
// for (type counter = 0; counter < cmp; counter += diff)
std::string emitFor(std::string type, std::string counter, std::string sub,
                    std::string cmp, bool isIncrement, std::string diff,
                    int indent_level, std::string statement) {
  std::string sentence;
  if (isIncrement)
    sentence = "for (" + type + " " + counter + " = " + sub + "; " + counter +
               " < " + cmp + "; ++" + counter + ") ";
  else
    sentence = "for (" + type + " " + counter + " = " + sub + "; " + counter +
               " < " + cmp + "; " + counter + " += " + diff + ") ";
  return sentence + "{\n" + emitIndent(indent_level + 1) + statement + emitIndent(indent_level) +"}\n";
}


std::string generateUnroll(const std::string n, const int bs, const int indent_level) {
  std::string code;
  for (int i = 0; i < bs; ++i) {
    std::string index = std::to_string(i);
    if (i != 0) code += emitIndent(indent_level);
    code += "C[i*N + jj + " + index + "] += A[i*N + k] * B[k*N + jj + " + index + "];\n";
  }
  return code;
}

std::string generateLoops(const int n, const int bs,
                          const int indent_level) {
  std::string N = std::to_string(n);
  std::string BS = std::to_string(bs);
  return std::string(
  emitIndent(indent_level) + "constexpr int N = " + N + ";\n" 
  + emitIndent(indent_level) + "constexpr int BS = " + BS + ";\n\n" 
  + emitIndent(indent_level)
  + emitFor("int", "ii", "0", "N", false, "BS", indent_level,
      emitFor("int", "kk", "0", "N", false, "BS", indent_level + 1,
        emitFor("int", "jj", "0", "N", false, "BS", indent_level + 2,
          emitFor("int", "i", "ii", "ii + BS", true, "", indent_level + 3,
            emitFor("int", "k", "kk", "kk + BS", true, "", indent_level + 4,
              generateUnroll(N, bs, indent_level + 5)
            )
          )
        )
      )
    )
  );
}

std::string makeFunctionName(const int n, const int bs) {
  return "dgemmTiledN" + std::to_string(n) + "AndBs" + std::to_string(bs);
}

std::string makeIncludeGuardName(const int n, const int bs) {
  return "DGEMM_TILED_N" + std::to_string(n) + "_BS" + std::to_string(bs) + "_HPP";
}

std::string generateProgram(const int n, const int bs,
                             const std::string& header_filename) {
  std::string code;

  code += "// generated program\n";
  code += "// N = " + std::to_string(n) + "\n";
  code += "// BS = " + std::to_string(bs) + "\n\n";

  code += "#include \"" + header_filename + "\"\n\n";

  code += "void " + makeFunctionName(n, bs) +
          "(const std::vector<double>& A, "
          "const std::vector<double>& B, "
          "std::vector<double>& C) {\n";

  int indent_level = 1;
  code += generateLoops(n, bs, indent_level);
  code += "}\n";
  return code;
}

std::string generateHeader(const int n, const int bs) {
  std::string guard = makeIncludeGuardName(n, bs);

  std::string code;
  code += "#ifndef " + guard + "\n";
  code += "#define " + guard + "\n\n";

  code += "#include <vector>\n\n";

  code += "void " + makeFunctionName(n, bs) +
          "(const std::vector<double>& A, "
          "const std::vector<double>& B, "
          "std::vector<double>& C);\n\n";

  code += "#endif // " + guard + "\n";

  return code;
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    std::cerr << "usage: " << argv[0] << " N BS output_prefix\n";
    return 1;
  }

  int n = std::atoi(argv[1]);
  int bs = std::atoi(argv[2]);
  std::string prefix = argv[3];

  if (n % bs != 0) {
    std::cerr << "N must be divisible by BS\n";
    return 1;
  }

  std::string header_filename = prefix + ".hpp";
  std::string cpp_filename = prefix + ".cpp";

  {
    std::ofstream ofs(header_filename);
    ofs << generateHeader(n, bs);
  }

  {
    std::ofstream ofs(cpp_filename);
    ofs << generateProgram(n, bs, header_filename);
  }
}