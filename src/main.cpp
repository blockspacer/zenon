#include <fstream>
#include "debug.h"

#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "reader.h"
#include "ProgramOptions.h"
#include "lsp.h"

auto installDir = INSTALL_DIR;

static po::parser getCommandLineFlags() {
  po::parser flags;
  flags["help"].abbreviation('h').description("Print help");
  flags["o"].type(po::string).description("Binary output path.");
  flags["cpp"].type(po::string).fallback("g++").description("C++ compiler path (default: g++).");
  flags["c"].description("Do not link binary.");
  flags[""].type(po::string).description("Path to the input program.");
  return flags;
}

static int compileCpp(string command, const string& program, const string& output) {
  command += " -xc++ - -std=c++17 -c -o " + output;
  FILE* p = popen(command.c_str(), "w");
  fwrite(program.c_str(), 1, program.size(), p);
  string out;
  return pclose(p) / 256;
}

static int linkObjs(const string& cmd, const vector<string>& objs, const string& output) {
  auto command = cmd + " " + combine(objs, " ") + " -o " + output;
  return system(command.data());
}

void initLogging(ofstream& logFile) {
  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
  InfoLog.addOutput(DebugOutput::toStream(logFile));
  ErrorLog.addOutput(DebugOutput::exitProgram(1));
  ErrorLog.addOutput(DebugOutput::toStream(std::cerr));
}

template <typename T>
T getOrCompileError(const WithErrorLine<T>& value) {
  if (value)
    return *value;
  else
    value.get_error().loc.error(value.get_error().error);
}

int main(int argc, char* argv[]) {
  po::parser flags = getCommandLineFlags();
  if (!flags.parseArgs(argc, argv))
    return -1;
  if (flags["help"].was_set() || argc == 1) {
    std::cout << flags << endl;
    return 0;
  }
  if (flags["lsp"].was_set()) {
    startLsp();
    return 0;
  }
  auto gccCmd = flags["cpp"].get().string;
  bool fullCompile = !flags["c"].was_set();
  bool printCpp = !flags["o"].was_set();
  ofstream logFile("log.out");
  initLogging(logFile);
  vector<ModuleInfo> toCompile;
  set<string> finished;
  for (auto pathElem : flags[""])
    toCompile.push_back({fs::canonical(pathElem.string), false});
  vector<string> objFiles;
  auto buildDir = ".build_cache";
  if (!fs::is_directory(buildDir))
    fs::create_directory(buildDir);
  while (!toCompile.empty()) {
    auto path = toCompile.begin()->path;
    bool builtInModule = toCompile.begin()->builtIn;
    cerr << "Compiling " << path << endl;
    toCompile.erase(toCompile.begin());
    finished.insert(path);
    auto program = readFromFile(path.c_str());
    if (!program)
      ERROR << program.get_error();
    INFO << "Parsing:\n\n" << program->value;
    auto tokens = getOrCompileError(lex(program->value, CodeLoc(path, 0, 0), "end of file"));
    auto ast = parse(tokens);
    auto context = createNewContext();
    auto imported = getOrCompileError(correctness(ast, context, {installDir}, builtInModule));
    auto cppCode = codegen(ast, context, installDir + "/codegen_includes/all.h"s, !printCpp);
    if (printCpp) {
      cout << cppCode << endl;
      return 0;
    } else
      logFile << cppCode;
    if (fullCompile)
      for (auto& import : imported)
        if (!finished.count(import.path))
          toCompile.push_back(import);
    auto objFile = fullCompile
        ? buildDir + "/"s + to_string(std::hash<string>()(cppCode)) + ".znn.o"
        : flags["o"].get().string;
    if ((!fullCompile || !fs::exists(objFile)) && compileCpp(gccCmd, cppCode, objFile)) {
      cerr << "C++ compilation failed, which is a Zenon bug :(\n\n" << endl;
      cerr << cppCode << endl;
      return 2;
    } else if (fullCompile)
      objFiles.push_back(objFile);
  }
  if (!objFiles.empty())
    if (linkObjs(gccCmd, objFiles, flags["o"].get().string) != 0) {
      cerr << "Linking failed" << endl;
      return 2;
    }
}
