# libgoboard
[![Build Status](https://travis-ci.org/sjtu-ai-go/libgoboard.svg)](https://travis-ci.org/sjtu-ai-go/libgoboard)
[![GNU3 License](https://img.shields.io/github/license/sjtu-ai-go/libgoboard.svg)](https://github.com/sjtu-ai-go/libgoboard/blob/master/LICENSE)

## Usage
```
git submodule add {{repo_url}} vendor/libgoboard
git submodule update --recursive --init
```
Then, in `CMakeLists.txt`:
```
add_subdirectory(vendor/libgoboard)
include_directories(${libgoboard_INCLUDE_DIR})

# After add_executable(your_prog)
target_link_libraries(your_prog goboard)
```

Enable test with `libgoboard_enable_tests`, default `OFF`.
