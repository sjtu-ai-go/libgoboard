# libgoboard

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
