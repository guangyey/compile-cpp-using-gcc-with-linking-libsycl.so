# compile-cpp-using-gcc-with-linking-libsycl.so

Compile cpp files using gcc compiler with linking libsycl.so

when use 
```bash
dpcpp -o test test_sycl_runtime.cpp && ./test
```
It works.

```bash
gcc -I/home/gta/intel/oneapi/compiler/latest/linux/include -I/home/gta/intel/oneapi/compiler/latest/linux/include/sycl -o test_gcc test_sycl_runtime.cpp -lsycl -lsvml -lirng -limf -lintlc -L/home/gta/intel/oneapi/compiler/latest/linux/lib/ -L/home/gta/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin -std=c++17 -lstdc++
```
It works.
<img width="438" alt="image" src="https://user-images.githubusercontent.com/106960996/207304925-7e82a416-ea0f-4bf3-badc-3b1a449e089d.png">


**Note**: If you want to trick off a kernel that is a lamda fuction, **it will fail**.
<img width="872" alt="image" src="https://user-images.githubusercontent.com/106960996/207289604-6f013f73-c5d4-462e-8dd7-95192d139c2f.png">
