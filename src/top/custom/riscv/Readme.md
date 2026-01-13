## 文件说明
为了保持和硬件的一致性，各模块的功能如下：
`custom_expp`:自定义的BF16精度的exp计算单元，精度在`<10`以内比较好

`MxFp8ActQuant`:量化单元BF16->MXFP8

`SoftmaxCore`:自定义的softmax计算单元


---
基础 demo
`main_expp`  |  `main_quant`  |  `single_expp(for tmp Debug)`