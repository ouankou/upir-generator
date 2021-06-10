//===- toyc.cpp - The Toy Compiler ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the entry point for the Toy compiler.
//
//===----------------------------------------------------------------------===//

#include "toy/Dialect.h"
#include "toy/MLIRGen.h"
#include "toy/Parser.h"
#include <memory>

#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"

using namespace toy;
namespace cl = llvm::cl;

static cl::opt<std::string> inputFilename(cl::Positional,
                                          cl::desc("<input toy file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));

namespace {
enum InputType { Toy, MLIR };
}
static cl::opt<enum InputType> inputType(
    "x", cl::init(Toy), cl::desc("Decided the kind of output desired"),
    cl::values(clEnumValN(Toy, "toy", "load the input file as a Toy source.")),
    cl::values(clEnumValN(MLIR, "mlir",
                          "load the input file as an MLIR file")));

namespace {
enum Action { None, DumpAST, DumpMLIR };
}
static cl::opt<enum Action> emitAction(
    "emit", cl::desc("Select the kind of output desired"),
    cl::values(clEnumValN(DumpAST, "ast", "output the AST dump")),
    cl::values(clEnumValN(DumpMLIR, "mlir", "output the MLIR dump")));

/// Returns a Toy AST resulting from parsing the file or a nullptr on error.
std::unique_ptr<toy::ModuleAST> parseInputFile(llvm::StringRef filename) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(filename);
  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Could not open input file: " << ec.message() << "\n";
    return nullptr;
  }
  auto buffer = fileOrErr.get()->getBuffer();
  LexerBuffer lexer(buffer.begin(), buffer.end(), std::string(filename));
  Parser parser(lexer);
  return parser.parseModule();
}

int dumpMLIR() {
  mlir::MLIRContext context;
  // Load our Dialect in this MLIR Context.
  context.getOrLoadDialect<mlir::toy::ToyDialect>();

  // Handle '.toy' input to the compiler.
  if (inputType != InputType::MLIR &&
      !llvm::StringRef(inputFilename).endswith(".mlir")) {
    auto moduleAST = parseInputFile(inputFilename);
    if (!moduleAST)
      return 6;
    mlir::OwningModuleRef module = mlirGen(context, *moduleAST);
    if (!module)
      return 1;

    module->dump();
    return 0;
  }

  // Otherwise, the input is '.mlir'.
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(inputFilename);
  if (std::error_code EC = fileOrErr.getError()) {
    llvm::errs() << "Could not open input file: " << EC.message() << "\n";
    return -1;
  }

  // Parse the input mlir.
  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());
  mlir::OwningModuleRef module = mlir::parseSourceFile(sourceMgr, &context);
  if (!module) {
    llvm::errs() << "Error can't load file " << inputFilename << "\n";
    return 3;
  }

  module->dump();
  return 0;
}

void convertREX() {
    std::cout << "Set up MLIR environment...." << std::endl;
    mlir::MLIRContext context;
    context.getOrLoadDialect<mlir::toy::ToyDialect>();
    context.getOrLoadDialect<mlir::StandardOpsDialect>();
    context.getOrLoadDialect<mlir::scf::SCFDialect>();
    mlir::OpBuilder builder = mlir::OpBuilder(&context);

    std::cout << "Prepare a dummy code location...." << std::endl;
    mlir::Location location = builder.getUnknownLoc();

    std::cout << "Prepare base function parameters...." << std::endl;
    llvm::ArrayRef<std::unique_ptr<llvm::StringRef>> args;
    llvm::SmallVector<mlir::Type, 4> arg_types(args.size(), builder.getNoneType());
    auto func_type = builder.getFunctionType(arg_types, llvm::None);
    llvm::ArrayRef<std::pair<mlir::Identifier, mlir::Attribute> > attrs = {};

    std::cout << "Prepare base function name...." << std::endl;
    llvm::StringRef func_name = std::string("foo");

    std::cout << "Create a base function...." << std::endl;
    mlir::FuncOp func = mlir::FuncOp::create(location, func_name, func_type, attrs);

    std::cout << "Create the body of base function...." << std::endl;
    mlir::Block &entryBlock = *func.addEntryBlock();

    builder.setInsertionPointToStart(&entryBlock);

    std::cout << "Insert a SPMD region to the base function...." << std::endl;
    mlir::Value num_threads = builder.create<mlir::ConstantIntOp>(location, 6, 32);
    mlir::toy::SpmdOp spmd = builder.create<mlir::toy::SpmdOp>(location, num_threads);
    mlir::Region &spmd_body = spmd.getRegion();
    builder.createBlock(&spmd_body);

    std::cout << "Insert a for loop to the SPMD region...." << std::endl;
    mlir::Value upper_bound = builder.create<mlir::ConstantIndexOp>(location, 0);
    mlir::Value lower_bound = builder.create<mlir::ConstantIndexOp>(location, 10);
    mlir::Value step = builder.create<mlir::ConstantIndexOp>(location, 1);
    mlir::ValueRange loop_value = {};
    mlir::scf::ForOp loop = builder.create<mlir::scf::ForOp>(location, upper_bound, lower_bound, step, loop_value);
    mlir::Region &loop_body = loop.getLoopBody();
    mlir::Block &loop_block = loop_body.front();
    builder.setInsertionPointToStart(&loop_block);

    std::cout << "Insert a printf function call to the for loop...." << std::endl;
    llvm::StringRef print_name = std::string("printf");
    mlir::StringAttr print_string = builder.getStringAttr(llvm::StringRef("This is a test.\n"));
    mlir::Value print_value = builder.create<mlir::ConstantOp>(location, print_string);
    mlir::ValueRange print_value_range = mlir::ValueRange(print_value);
    mlir::TypeRange print_type = mlir::TypeRange(print_value_range);
    builder.create<mlir::CallOp>(location, print_name, print_type, print_value_range);

    std::cout << "Create a module that contains multiple functions...." << std::endl;
    mlir::ModuleOp theModule = mlir::ModuleOp::create(builder.getUnknownLoc());
    theModule.push_back(func);

    mlir::OwningModuleRef module = theModule;
    assert (module);

    std::cout << "Dump the MLIR AST...." << std::endl;
    module->dump();
    std::cout << "All done...." << std::endl;
}

int dumpAST() {
  if (inputType == InputType::MLIR) {
    llvm::errs() << "Can't dump a Toy AST when the input is MLIR\n";
    return 5;
  }

  auto moduleAST = parseInputFile(inputFilename);
  if (!moduleAST)
    return 1;

  dump(*moduleAST);
  return 0;
}

int main(int argc, char **argv) {
  // Register any command line options.
  mlir::DialectRegistry registry;
  registry.insert<mlir::StandardOpsDialect>();
  registry.insert<mlir::scf::SCFDialect>();

  mlir::registerAsmPrinterCLOptions();
  mlir::registerMLIRContextCLOptions();
  cl::ParseCommandLineOptions(argc, argv, "toy compiler\n");

  switch (emitAction) {
  case Action::DumpAST:
    return dumpAST();
  case Action::DumpMLIR:
    return dumpMLIR();
  default:
    llvm::errs() << "No action specified (parsing only?), use -emit=<action>\n";
  }

  convertREX();

  return 0;
}