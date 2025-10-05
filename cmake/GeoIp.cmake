# CMake script to download and install GeoIP databases
# Will first download the GeoIPUpdate tool and then use it to download the databases

# URL Examples:
# https://github.com/maxmind/geoipupdate/releases/download/v7.1.1/geoipupdate_7.1.1_darwin_arm64.tar.gz
# https://github.com/maxmind/geoipupdate/releases/download/v7.1.1/geoipupdate_7.1.1_linux_amd64.tar.gz
# https://github.com/maxmind/geoipupdate/releases/download/v7.1.1/geoipupdate_7.1.1_linux_arm64.tar.gz

set(GEOIP_BIN_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/geoip")
if(NOT DEFINED GEOIP_FILES)
    set(GEOIP_FILES 
        GeoLite2-Country.mmdb
        GeoLite2-City.mmdb
        GeoLite2-ASN.mmdb
    )
endif()


function(download_geoip_tool_bin url archive_name)
    set(GEOIP_BIN_ARCHIVE ${archive_name})
    set(GEOIP_UPDATE_TOOL_URL ${url})
    message(STATUS "Downloading GeoIPUpdate tool from ${GEOIP_UPDATE_TOOL_URL}")
    execute_process(
        COMMAND mkdir -p ${GEOIP_BIN_BASE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    execute_process(
        COMMAND curl -vv -L -o ${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_ARCHIVE} ${GEOIP_UPDATE_TOOL_URL}
        WORKING_DIRECTORY ${GEOIP_BIN_BASE_DIR}
        ERROR_VARIABLE CURL_ERROR
        RESULT_VARIABLE CURL_RESULT
    )
    if(NOT CURL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download GeoIPUpdate tool: ${CURL_ERROR}")
    endif()
endfunction()

function(get_geoip_update_tool_bin)
    set(GEOIP_UPDATE_TOOL_VERSION "7.1.1")

    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE UNAME_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(UNAME_OUTPUT MATCHES "arm64|aarch64")
        set(GEOIP_UPDATE_TOOL_ARCH "arm64")
    else()
        set(GEOIP_UPDATE_TOOL_ARCH "amd64")
    endif()
    # Detect OS
    if(APPLE)
        set(GEOIP_UPDATE_TOOL_OS "darwin")
    else()
        set(GEOIP_UPDATE_TOOL_OS "linux") 
    endif()
    set(GEOIP_BIN_DIR "geoipupdate_${GEOIP_UPDATE_TOOL_VERSION}_${GEOIP_UPDATE_TOOL_OS}_${GEOIP_UPDATE_TOOL_ARCH}")
    set(GEOIP_BIN_ARCHIVE "${GEOIP_BIN_DIR}.tar.gz")
    if (NOT EXISTS ${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_DIR})
        if (NOT EXISTS ${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_ARCHIVE})
            set(GEOIP_UPDATE_TOOL_URL "https://github.com/maxmind/geoipupdate/releases/download/v${GEOIP_UPDATE_TOOL_VERSION}/${GEOIP_BIN_ARCHIVE}")
            download_geoip_tool_bin(${GEOIP_UPDATE_TOOL_URL} ${GEOIP_BIN_ARCHIVE})
        endif()
        message(STATUS "Extracting GeoIPUpdate tool from ${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_ARCHIVE}")
        execute_process(
            COMMAND tar -xzf ${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_ARCHIVE}
            WORKING_DIRECTORY ${GEOIP_BIN_BASE_DIR}
            ERROR_VARIABLE TAR_ERROR
            RESULT_VARIABLE TAR_RESULT
        )
        if(NOT TAR_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to extract GeoIPUpdate tool: ${TAR_ERROR}")
        endif()
    endif()
    set(GEOIP_UPDATE_TOOL_BIN "${GEOIP_BIN_BASE_DIR}/${GEOIP_BIN_DIR}/geoipupdate" PARENT_SCOPE)
endfunction()

function(download_geoip_databases config_file database_dir)
    get_geoip_update_tool_bin()
    execute_process(
        COMMAND mkdir -p ${database_dir}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Run the update only if the database files are more than 1 day old
    set(UPDATE_NEEDED false)
    foreach(GEOIP_FILE ${GEOIP_FILES})
        if(EXISTS ${database_dir}/${GEOIP_FILE})
            execute_process(
                COMMAND stat -c %Y ${database_dir}/${GEOIP_FILE}
                OUTPUT_VARIABLE GEOIP_FILE_MTIME
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(GEOIP_FILE_MTIME LESS 1)
                set(UPDATE_NEEDED true)
            endif()
        else()
            set(UPDATE_NEEDED true)
        endif()
    endforeach()
    if(UPDATE_NEEDED)
        message(STATUS "GeoIPUpdate tool binary: ${GEOIP_UPDATE_TOOL_BIN}")
        message(STATUS "Downloading GeoIP databases with config file ${config_file} to ${database_dir}")
        execute_process(
            COMMAND ${GEOIP_UPDATE_TOOL_BIN} -f ${config_file} -d ${database_dir}
            WORKING_DIRECTORY ${database_dir}
            ERROR_VARIABLE GEOIP_UPDATE_ERROR
            RESULT_VARIABLE GEOIP_UPDATE_RESULT
        )
        if(NOT GEOIP_UPDATE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download GeoIP databases: ${GEOIP_UPDATE_ERROR}")
        endif()
    endif()
    set(GEOIP_UPDATE_TOOL_BIN ${GEOIP_UPDATE_TOOL_BIN} PARENT_SCOPE)
    if(NOT DEFINED GEOIP_DATABASE_DIR)
        set(GEOIP_DATABASE_DIR ${database_dir} PARENT_SCOPE)
    endif()
endfunction()

function(configure_geoip_environment_script input_file output_file)
    if(NOT DEFINED GEOIP_DATABASE_DIR)
        message(FATAL_ERROR "GEOIP_DATABASE_DIR is not set")
    endif()
    if(NOT DEFINED GEOIP_FILES)
        message(FATAL_ERROR "GEOIP_FILES is not set")
    endif()
    if(NOT DEFINED GEOIP_UPDATE_TOOL_BIN)
        message(FATAL_ERROR "GEOIP_UPDATE_TOOL_BIN is not set")
    endif()
    # Remove $CMAKE_CURRENT_SOURCE_DIR from the variables
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" GEOIP_DATABASE_DIR ${GEOIP_DATABASE_DIR})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" GEOIP_UPDATE_TOOL_BIN ${GEOIP_UPDATE_TOOL_BIN})
    string(REPLACE ";" ":" GEOIP_FILES "${GEOIP_FILES}")
    configure_file(${input_file} ${output_file} @ONLY)
endfunction()
