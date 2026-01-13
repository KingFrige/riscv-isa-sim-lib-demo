## 文件说明
为了保持和硬件的一致性，各模块的功能如下：
`MxFp8ActDeNorm`:对Act进行预计算，返回`i_MxFp8Act`预计算的符号数、尾数、指数

`MxFp8Product`:计算(1) two FP4 × FP8 or (2) one FP8 × FP8

`Fp32PsumAcc`:计算单个数的不断累加

`GemmTop`:每次调用iteration: (1×16) × (16×16) = (1×16) output

---

`main_gemm`给出了一个基础的demo，举例计算了(1×4096) × (4096×16)的Fp8*Fp8，也就是testbench的case