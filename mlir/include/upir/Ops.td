//===- Ops.td - Upir dialect operation definitions ---------*- tablegen -*-===//
//
//
//===----------------------------------------------------------------------===//
//
// Defines the operations of the Upir dialect.
//
//===----------------------------------------------------------------------===//

include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

// Provide a definition of the 'upir' dialect in the ODS framework so that we
// can define our operations.
def Upir_Dialect : Dialect {
  let name = "upir";
  let cppNamespace = "::mlir::upir";
}

// Base class for upir dialect operations. This operation inherits from the base
// `Op` class in OpBase.td, and provides:
//   * The parent dialect of the operation.
//   * The mnemonic for the operation, or the name without the dialect prefix.
//   * A list of traits for the operation.
class Upir_Op<string mnemonic, list<Trait> traits = []> :
    Op<Upir_Dialect, mnemonic, traits>;

// Provide a definition for the Toy StructType for use in ODS. This allows for
// using StructType in a similar way to Tensor or MemRef. We use `DialectType`
// to demarcate the StructType as belonging to the Toy dialect.
def Upir_ParallelDataType :
    DialectType<Upir_Dialect, CPred<"$_self.isa<ParallelDataType>()">,
                "Upir parallel data type">;

//===----------------------------------------------------------------------===//
// Upir Operations
//===----------------------------------------------------------------------===//

// We define a upir operation by inheriting from our base 'Upir_Op' class above.
// Here we provide the mnemonic and a list of traits for the operation.
// An operation could be marked as 'NoSideEffect' as it is a pure operation
// and may be removed if dead.

def SpmdOp : Upir_Op<"spmd", [AttrSizedOperandSegments]> {
  let summary = "spmd operation";
  let description = [{
    Create a SPMD region and perform parallel computing.
  }];

  let regions = (region AnyRegion:$region);

  let arguments = (ins Optional<AnyType>:$num_units,
             Optional<AnyType>:$nested_parallelism_level,
             Variadic<AnyType>:$nested_parallelism_list,
             Variadic<AnyType>:$data,
             Optional<AnyType>:$cost);

  let builders = [
    OpBuilder<(ins CArg<"ArrayRef<NamedAttribute>", "{}">:$attributes)>
  ];

  let printer = [{ return ::printSpmdOp(p, *this); }];

}

def WorkshareOp : Upir_Op<"workshare", [AttrSizedOperandSegments, AllTypesMatch<["lower_bound", "upper_bound", "step"]>]> {
  let summary = "worksharing operation";
  let description = [{
    Distribute workload among the threads in a parallel region.
  }];

  let regions = (region AnyRegion:$region);

  let arguments = (ins OptionalAttr<StrAttr>:$source_block,
             Variadic<Index>:$lower_bound,
             Variadic<Index>:$upper_bound,
             Variadic<Index>:$step,
             OptionalAttr<StrAttr>:$schedule_policy,
             Optional<AnyType>:$chunk_size,
             Optional<AnyType>:$collapse,
             Variadic<AnyType>:$distribute,
             Optional<AnyType>:$simdlen,
             Variadic<AnyType>:$data,
             Optional<AnyType>:$cost);

  let builders = [
    OpBuilder<(ins CArg<"ArrayRef<NamedAttribute>", "{}">:$attributes)>
  ];

  let printer = [{ return ::printWorkshareOp(p, *this); }];

}

def TaskOp : Upir_Op<"task", [AttrSizedOperandSegments]> {
  let summary = "task operation";
  let description = [{
    Create a task region, which can be handled on the host or devices.
  }];

  let regions = (region AnyRegion:$region);

  let arguments = (ins Optional<AnyType>:$device_id,
             OptionalAttr<StrAttr>:$device,
             Optional<AnyType>:$condition,
             Variadic<AnyType>:$depend,
             Variadic<AnyType>:$data,
             Optional<AnyType>:$cost);

  let builders = [
    OpBuilder<(ins CArg<"ArrayRef<NamedAttribute>", "{}">:$attributes)>
  ];

  let printer = [{ return ::printTaskOp(p, *this); }];

}

def DataOp : Upir_Op<"data", [AttrSizedOperandSegments]> {
  let summary = "data operation";
  let description = [{
    Create a data environment and/or perform data movements.
  }];

  let regions = (region AnyRegion:$region);

  let arguments = (ins Optional<AnyType>:$device_id,
             OptionalAttr<StrAttr>:$device,
             Variadic<AnyType>:$enter_vars,
             Variadic<AnyType>:$update_vars,
             Optional<AnyType>:$exit,
             Optional<AnyType>:$cost);

  let builders = [
    OpBuilder<(ins CArg<"ArrayRef<NamedAttribute>", "{}">:$attributes)>
  ];

  let printer = [{ return ::printDataOp(p, *this); }];

}

def BarrierOp : Upir_Op<"barrier", [AttrSizedOperandSegments]> {
  let summary = "barrier operation";
  let description = [{
    Create a barrier for parallel units.
  }];

  let arguments = (ins Optional<AnyType>:$task_id,
             Optional<AnyType>:$implicit,
             Optional<AnyType>:$cost);

  let builders = [
    OpBuilder<(ins CArg<"ArrayRef<NamedAttribute>", "{}">:$attributes)>
  ];

  let printer = [{ return ::printBarrierOp(p, *this); }];

}

def ParallelDataInfoOp : Upir_Op<"parallel_data_info", [NoSideEffect]> {
  let summary = "parallel data info";
  let description = [{
    Create a parallel data info for parallel units.
  }];

  let arguments = (ins ArrayAttr:$data,
             Optional<AnyType>:$value);
  let results = (outs AnyType);

  let printer = [{ return ::printParallelDataInfoOp(p, *this); }];

}

