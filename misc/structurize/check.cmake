# Copyright (c) 2026 Erhan Bilgili
# SPDX-License-Identifier: MIT

execute_process(COMMAND "${PROGRAM}" "${INPUT}"
                RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE output
                TIMEOUT 10)

# Check the status as text so crashes and timeouts cannot be mistaken for success.
if (NOT "${status}" STREQUAL "${EXPECTED_STATUS}" OR NOT output MATCHES "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "Unexpected structurize-test result (${status}):\n${output}")
endif()
