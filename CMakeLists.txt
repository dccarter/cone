cmake_minimum_required(VERSION 3.18)
project(conec C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CONE_COMPILER_BINARY ${CMAKE_CURRENT_BINARY_DIR}/conec)
enable_language(CONE)


find_package(LLVM 13 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(
    "${CMAKE_SOURCE_DIR}/src/c-compiler/"
)

set(LLVM_LINK_COMPONENTS
		Analysis
		BitReader
		Core
		ExecutionEngine
		InstCombine
		Interpreter
		MC
		MCDisassembler
		MCJIT
		Object
		OrcJIT
		RuntimeDyld
		ScalarOpts
		Support
		Target
		TransformUtils
		native
		nativecodegen
		AsmPrinter
		AArch64Info
		AArch64Desc
		AArch64AsmParser
		AArch64Disassembler
		AArch64CodeGen
		ARMInfo
		ARMDesc
		ARMAsmParser
		ARMDisassembler
		ARMCodeGen
		AMDGPUInfo
		AMDGPUAsmParser
		AMDGPUCodeGen
		AVRAsmParser
		AVRInfo
		AVRDesc
		AVRCodeGen
		BPFInfo
		BPFAsmParser
		BPFDisassembler
		BPFCodeGen
		HexagonInfo
		HexagonDesc
		HexagonAsmParser
		HexagonDisassembler
		HexagonCodeGen
		LanaiInfo
		LanaiDesc
		LanaiAsmParser
		LanaiDisassembler
		LanaiCodeGen
		MSP430Info
		MSP430Desc
		MSP430CodeGen
		MSP430AsmParser
		MipsInfo
		MipsDesc
		MipsAsmParser
		MipsDisassembler
		MipsCodeGen
		NVPTXInfo
		NVPTXCodeGen
		PowerPCInfo
		PowerPCDesc
		PowerPCAsmParser
		PowerPCDisassembler
		PowerPCCodeGen
		RISCVAsmParser
		RISCVInfo
		RISCVDesc
		RISCVDisassembler
		RiSCVCodeGen
		SparcInfo
		SparcDesc
		SparcAsmParser
		SparcDisassembler
		SparcCodeGen
		SystemZInfo
		SystemZDesc
		SystemZAsmParser
		SystemZDisassembler
		SystemZCodeGen
		WebAssemblyInfo
		WebAssemblyDesc
		WebAssemblyAsmParser
		WebAssemblyDisassembler
		WebAssemblyCodeGen
		X86Info
		X86Desc
		X86AsmParser
		X86Disassembler
		X86CodeGen
		XCoreInfo
		XCoreDesc
		XCoreDisassembler
		XCoreCodeGen
		)

llvm_map_components_to_libnames(llvm_libs support core irreader ${LLVM_LINK_COMPONENTS})


set(CONE_COMPILER_SOURCES
		src/c-compiler/conec.c
		src/c-compiler/coneopts.c)

set(CONE_SHARED_SOURCES
		src/c-compiler/shared/error.c
		src/c-compiler/shared/fileio.c
		src/c-compiler/shared/memory.c
		src/c-compiler/shared/options.c
		src/c-compiler/shared/timer.c
		src/c-compiler/shared/utf8.c)

set(CONE_IR_SOURCES
		src/c-compiler/ir/clone.c
		src/c-compiler/ir/flow.c
		src/c-compiler/ir/iexp.c
		src/c-compiler/ir/inode.c
		src/c-compiler/ir/instype.c
		src/c-compiler/ir/itype.c
		src/c-compiler/ir/name.c
		src/c-compiler/ir/namespace.c
		src/c-compiler/ir/nametbl.c
		src/c-compiler/ir/nodelist.c
		src/c-compiler/ir/nodes.c
		src/c-compiler/ir/typetbl.c)

set(CONE_STMT_SOURCES
		src/c-compiler/ir/stmt/break.c
		src/c-compiler/ir/stmt/const.c
		src/c-compiler/ir/stmt/continue.c
		src/c-compiler/ir/stmt/fielddcl.c
		src/c-compiler/ir/stmt/fndcl.c
		src/c-compiler/ir/stmt/import.c
		src/c-compiler/ir/stmt/intrinsic.c
		src/c-compiler/ir/stmt/module.c
		src/c-compiler/ir/stmt/program.c
		src/c-compiler/ir/stmt/return.c
		src/c-compiler/ir/stmt/swap.c
		src/c-compiler/ir/stmt/vardcl.c)

set(CONE_EXP_SOURCES
		src/c-compiler/ir/exp/allocate.c
		src/c-compiler/ir/exp/arraylit.c
		src/c-compiler/ir/exp/assign.c
		src/c-compiler/ir/exp/block.c
		src/c-compiler/ir/exp/borrow.c
		src/c-compiler/ir/exp/cast.c
		src/c-compiler/ir/exp/deref.c
		src/c-compiler/ir/exp/fncall.c
		src/c-compiler/ir/exp/if.c
		src/c-compiler/ir/exp/literal.c
		src/c-compiler/ir/exp/logic.c
		src/c-compiler/ir/exp/namedval.c
		src/c-compiler/ir/exp/nameuse.c
		src/c-compiler/ir/exp/sizeof.c
		src/c-compiler/ir/exp/typelit.c
		src/c-compiler/ir/exp/vtuple.c)

set(CONE_TYPE_SOURCES
		src/c-compiler/ir/types/array.c
		src/c-compiler/ir/types/arrayref.c
		src/c-compiler/ir/types/enum.c
		src/c-compiler/ir/types/fnsig.c
		src/c-compiler/ir/types/lifetime.c
		src/c-compiler/ir/types/number.c
		src/c-compiler/ir/types/permission.c
		src/c-compiler/ir/types/pointer.c
		src/c-compiler/ir/types/reference.c
		src/c-compiler/ir/types/region.c
		src/c-compiler/ir/types/struct.c
		src/c-compiler/ir/types/ttuple.c
		src/c-compiler/ir/types/typedef.c
		src/c-compiler/ir/types/void.c)

set(CONE_META_SOURCES
		src/c-compiler/ir/meta/macro.c
		src/c-compiler/ir/meta/genvardcl.c
		src/c-compiler/ir/meta/generic.c)

set(CONE_PARSER_SOURCES
		src/c-compiler/parser/lexer.c
		src/c-compiler/parser/parser.c
		src/c-compiler/parser/parseflow.c
		src/c-compiler/parser/parseexpr.c
		src/c-compiler/parser/parsetype.c)

set(CONE_CORELIB_SOURCES
		src/c-compiler/corelib/corelib.c
		src/c-compiler/corelib/corenumber.c)

set(CONE_LLVM_SOURCES
		src/c-compiler/genllvm/genllvm.c
		src/c-compiler/genllvm/genlstmt.c
		src/c-compiler/genllvm/genlexpr.c
		src/c-compiler/genllvm/genlalloc.c
		src/c-compiler/genllvm/genltype.c)

add_executable(conec
		${CONE_COMPILER_SOURCES}
		${CONE_SHARED_SOURCES}
		${CONE_IR_SOURCES}
		${CONE_STMT_SOURCES}
		${CONE_EXP_SOURCES}
		${CONE_TYPE_SOURCES}
		${CONE_META_SOURCES}
		${CONE_PARSER_SOURCES}
		${CONE_CORELIB_SOURCES}
		${CONE_LLVM_SOURCES}
)

target_link_libraries(conec ${llvm_libs})

set(CONE_STD_SOURCES
	src/conestd/stdio.c)

add_library(conestd STATIC
		${CONE_STD_SOURCES})

add_library(conestd-shared SHARED
		${CONE_STD_SOURCES})

add_executable(test
		test/test.cone)
