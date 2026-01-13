#include <cstdio>
#include <cstdint>
#include "riscv/MxFp8ActQuant.hpp"

int main() {
    // Test inputs: BF16 values for 1.0, 2.0, 3.0, 4.0
    uint16_t inputs[4] = {0x3F80, 0x4000, 0x4040, 0x4080};
    
    MxFp8ActQuant_Model quant_model(4);
    quant_model.process(inputs);
    
    printf("Quantization results:\n");
    printf("Scale: 0x%02x\n", quant_model.o_MxFp8ActScale);
    for (int i = 0; i < 4; i++) {
        printf("Input 0x%04x -> Output 0x%02x\n", 
               inputs[i], quant_model.o_MxFp8Act[i]);
    }
    
    // Decode outputs
    for (int i = 0; i < 4; i++) {
        uint8_t val = quant_model.o_MxFp8Act[i];
        uint8_t sign = (val >> 7) & 0x1;
        uint8_t exp = (val >> 3) & 0xF;
        uint8_t mant = val & 0x7;
        printf("Output[%d]: 0x%02x = sign=%d, exp=%d, mant=%d\n",
               i, val, sign, exp, mant);
    }
    return 0;
}
