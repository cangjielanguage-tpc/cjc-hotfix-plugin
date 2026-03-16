Intro:
-
Plugin for Cangjie compiler (`cjc`) that makes certain transformations during CHIR generation phase 
to prepare the source code for possible hot fixes in the future.  

To build a shared library run go to `src` subdirectory and run:   
1. `cmake . -DLSP_PATH=dirname "<lsp_path>" -DCMAKE_BUILD_TYPE=<build_type>`  
*<lsp_path> - path to `libcangjie-lsp.so`*  
*<build_type> - one of `Debug`/`Release`/`Test`*    
1. `make -j`

To run the all tests go to `test` subdirectory and run:  
`./run_tests.sh <Cangjie_toolchain_dir>`

Structure description:
-

`include` - list of header files are used in plugin source code and tests.  
`libs` - external libraries.  
Now only toml++ locates there as one-header file library.  
Probably should be removed if `cjc` provides proper implementation (TODO).  
`src` - contains source code  
`test` - contains unit tests, test data and script `run_tests.sh` to run integration tests.

Source code:
-
- C++17
- Uniform initialization
