# Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Determines the newest microarchitecture supported by the build machine using MSVC.
#
# This exists because unlike GCC and Clang, MSVC does not have an equivalent
# to -march=native which targets the latest supported microarchitecture.
#
# Usage:
#     determine_msvc_native_arch(
#         <output variable> # The cache variable in which to store the MSVC /arch:foo option
#     )
function(determine_msvc_native_arch _arch_option_out)
	# Return if we already know the /arch option
	if(NOT "${${_arch_option_out}}" STREQUAL "")
		return()
	endif()

	if(NOT MSVC)
		message(FATAL_ERROR "Only MSVC is supported")
	endif()

	if(CMAKE_CROSSCOMPILING)
		message(FATAL_ERROR "Cross-compiling is not supported")
	endif()

	if(NOT LMMS_HOST_X86_64 AND NOT LMMS_HOST_ARM64)
		message(WARNING "CPU feature detection is only implemented for x86_64 and arm64 - using default /arch option.")
		set("${_arch_option_out}" "" CACHE INTERNAL "Native /arch option")
		return()
	endif()

	set(_source [[
		#include <iostream>
		#if defined(_M_X64)
		#	include <immintrin.h>
		#	include <isa_availability.h>
		#elif defined(_M_ARM64)
		#	include <Windows.h>
		#else
		#	error Unsupported platform
		#endif

		auto main() -> int
		{
		#if defined(_M_X64)
			if (__check_isa_support(__IA_SUPPORT_AVX10_2, 0)) {
				std::cout << "/arch:AVX10.2";
			} else if (__check_isa_support(__IA_SUPPORT_AVX10, 0)) {
				std::cout << "/arch:AVX10.1";
			} else if (__check_isa_support(__IA_SUPPORT_VECTOR512, 0)) {
				std::cout << "/arch:AVX512";
			} else if (__check_isa_support(__IA_SUPPORT_VECTOR256, 0)) {
				std::cout << "/arch:AVX2";
			} else if (__check_isa_support(__IA_SUPPORT_SSE42, 0)) {
				std::cout << "/arch:SSE4.2";
			} else if (__check_isa_support(__IA_SUPPORT_VECTOR128, 0)) {
				std::cout << "/arch:SSE2";
			} else {
				std::cout << "";
			}
		#else
		#	if defined(PF_ARM_SME2_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_SME2_INSTRUCTIONS_AVAILABLE) != FALSE) {
				std::cout << "/arch:armv9.4";
			} else
		#	endif
		#	if defined(PF_ARM_SME_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_SME_INSTRUCTIONS_AVAILABLE) != FALSE) {
				std::cout << "/arch:armv9.2";
			} else
		#	endif
		#	if defined(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE) != FALSE) {
				std::cout << "/arch:armv9.0";
			} else
		#	endif
		#	if defined(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) && defined(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) != FALSE
				&& IsProcessorFeaturePresent(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE) != FALSE) {
				std::cout << "/arch:armv8.3";
			} else
		#	endif
		#	if defined(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) != FALSE) {
		#		if defined(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE)
				if (IsProcessorFeaturePresent(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE) != FALSE) {
					std::cout << "/arch:armv8.2+rcpc";
				} else
		#		endif
				{
					std::cout << "/arch:armv8.2";
				}
			} else
		#	endif
		#	if defined(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)
			if (IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE) != FALSE) {
		#		if defined(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE)
				if (IsProcessorFeaturePresent(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE) != FALSE) {
					std::cout << "/arch:armv8.2+rcpc";
				} else
		#		endif
				{
					std::cout << "/arch:armv8.2";
				}
			} else
		#	endif
			if (IsProcessorFeaturePresent(PF_ARM_V8_INSTRUCTIONS_AVAILABLE) != FALSE) {
				if (IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE) != FALSE) {
					std::cout << "/arch:armv8.0+lse";
				} else {
					std::cout << "/arch:armv8.0";
				}
			} else {
				std::cout << "";
			}
		#endif
		}
	]])

	# Write the source code to a temporary file
	string(SHA1 _source_hash "${_source}")
	set(_source_file "${CMAKE_CURRENT_BINARY_DIR}/${_source_hash}.cpp")
	file(WRITE "${_source_file}" "${_source}")

	# Build and run the temporary file to get the /arch option
	# TODO CMake 3.25: Use the new signature for try_run which has a NO_CACHE
	#                  option and doesn't require separate file management.
	try_run(
		_dmna_run_result _dmna_compile_result "${CMAKE_CURRENT_BINARY_DIR}"
		SOURCES "${_source_file}"
		CMAKE_FLAGS /EHsc /O2
		CXX_STANDARD 20
		RUN_OUTPUT_VARIABLE _run_output
		COMPILE_OUTPUT_VARIABLE _compile_output
	)

	# Clean up the temporary file
	file(REMOVE "${_source_file}")

	# Set the /arch option if the run was successful, using a cache variable since
	# this check may be relatively expensive. Otherwise, log the error
	# and inform the user.
	if(_dmna_run_result EQUAL "0")
		message(STATUS "Determined MSVC native /arch option: '${_run_output}'")
		set("${_arch_option_out}" "${_run_output}" CACHE INTERNAL "Native /arch option")
	else()
		message(DEBUG "${_compile_output}")
	endif()
endfunction()
