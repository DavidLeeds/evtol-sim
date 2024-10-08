include(FetchContent)

# -------------------------------
# nlohmann_json
# -------------------------------

set(EVTOL_JSON_VERSION "3.11.3" CACHE STRING "nlohmann_json version to use in the project")

message(STATUS "evtol-sim: downloading nlohmann_json")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v${EVTOL_JSON_VERSION}
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(nlohmann_json)

# -------------------------------
# fmt
# -------------------------------

set(EVTOL_FMT_VERSION "11.0.2" CACHE STRING "fmt version to use in the project")

message(STATUS "evtol-sim: downloading fmt")
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG ${EVTOL_FMT_VERSION}
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(fmt)
