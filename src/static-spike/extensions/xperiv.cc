#define DECODE_MACRO_USAGE_LOGGED 0
#include <sys/syscall.h>
#include "extension.h"
#include "processor.h"
#include "decode.h"
#include "insn_macros.h"
#include "decode_macros.h"
#include "v_ext_macros.h"

// Include C_src algorithm headers
#include "BF16.hpp"
#include "custom_expp.hpp"
#include "SoftmaxCore.hpp"
#include "MxFp8ActQuant.hpp"

#ifndef xlen
#define xlen 64
#endif

#define MATCH_PERI_V_ADD 0x0000002b
#define MASK_PERI_V_ADD  0xfc00707f

#define MATCH_PERI_V_MUL 0x0000005b
#define MASK_PERI_V_MUL  0xfc00707f

// New instruction definitions for mathematical extensions
#define MATCH_EXP        0x0600600b  // func7=0x03, func3=0x6, opcode=0x0b (vm=1)
#define MATCH_SOFTMAX    0x0600200b  // func7=0x03, func3=0x2, opcode=0x0b (vm=1)
#define MATCH_QUANT      0x0a00600b  // func7=0x05, func3=0x6, opcode=0x0b (vm=1)
#define MASK_CUSTOM0     0xfe00707f  // Match func7, func3, opcode; ignore registers

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vr_name[insn.rd()];
  }
} xvd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vr_name[insn.rs1()];
  }
} xvs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vr_name[insn.rs2()];
  }
} xvs2;

static reg_t peri_v_add_impl(processor_t* p, insn_t insn, reg_t pc)
{
  fprintf(stderr, "xperiv: peri_v_add_impl called! vd=%lu, vs1=%lu, vs2=%lu\n", 
          insn.rd(), insn.rs1(), insn.rs2());
  
  // Get vector length
  auto vlen = p->VU.get_vlen() / 8;
  auto elen = p->VU.vsew;
  auto current_vl = p->VU.vl->read();
  fprintf(stderr, "xperiv: vlen=%lu, elen=%lu, vl=%lu\n", vlen, elen, current_vl);
  
  VI_VV_LOOP
  ({
    vd = vs1 + vs2;
    fprintf(stderr, "xperiv: vd[%lu] = vs1[%lu] + vs2[%lu] = %ld + %ld = %ld\n",
            i, i, i, (long)vs1, (long)vs2, (long)vd);
  })
  fprintf(stderr, "xperiv: peri_v_add_impl returning\n");
  return pc + 4;
}

static reg_t peri_v_mul_impl(processor_t* p, insn_t insn, reg_t pc)
{
  VI_VV_LOOP
  ({
    vd = vs1 * vs2;
    fprintf(stderr, "xperiv_mul: vd[%lu] = vs1[%lu] * vs2[%lu] = %ld * %ld = %ld\n",
            i, i, i, (long)vs1, (long)vs2, (long)vd);
  })
  return pc + 4;
}

// ============================================================================
// EXP instruction implementation
// ============================================================================
static reg_t exp_impl(processor_t* p, insn_t insn, reg_t pc)
{
  fprintf(stderr, "xperiv: exp_impl called! vd=%lu, vs1=%lu, vs2=%lu\n", 
          insn.rd(), insn.rs1(), insn.rs2());
  
  // Get vector length
  auto vlen = p->VU.get_vlen() / 8;
  auto elen = p->VU.vsew;
  auto current_vl = p->VU.vl->read();
  fprintf(stderr, "xperiv: vlen=%lu, elen=%lu, vl=%lu\n", vlen, elen, current_vl);
  
  // Initialize BF16 LUTs if needed
  BF16::init_luts();
  
  // Create BF16ExppUnit_Model instance
  BF16ExppUnit_Model exp_unit;
  
  VI_VV_LOOP
  ({
    // Input is BF16 value in vs1 (lower 16 bits)
    uint16_t input = vs1 & 0xFFFF;
    uint16_t result = exp_unit.process(input);
    vd = result;  // Store BF16 result in lower 16 bits
    fprintf(stderr, "xperiv: exp vd[%lu] = exp(vs1[%lu]) = 0x%04x -> 0x%04x\n",
            i, i, input, result);
  })
  fprintf(stderr, "xperiv: exp_impl returning\n");
  return pc + 4;
}

// ============================================================================
// SOFTMAX instruction implementation
// ============================================================================
static reg_t softmax_impl(processor_t* p, insn_t insn, reg_t pc)
{
  fprintf(stderr, "xperiv: softmax_impl called! vd=%lu, vs1=%lu, vs2=%lu\n", 
          insn.rd(), insn.rs1(), insn.rs2());
  
  // Get vector length
  auto vlen = p->VU.get_vlen() / 8;
  auto elen = p->VU.vsew;
  auto current_vl = p->VU.vl->read();
  fprintf(stderr, "xperiv: vlen=%lu, elen=%lu, vl=%lu\n", vlen, elen, current_vl);
  
  // Initialize BF16 LUTs
  BF16::init_luts();
  
  // Collect input values from vs1 vector
  std::vector<uint16_t> input_values;
  for (size_t i = 0; i < current_vl; i++) {
    // Read vs1 element (16-bit BF16 value)
    uint16_t val = p->VU.elt<uint16_t>(insn.rs1(), i);
    input_values.push_back(val);
  }
  
  // Create SoftmaxCore_Model instance
  SoftmaxCore_Model softmax_model(current_vl);
  
  // Prepare input as 2D vector (single row)
  std::vector<std::vector<uint16_t>> input_row = {input_values};
  
  // Process softmax
  SoftmaxResult result = softmax_model.process(input_row);
  
  // Store results back to vd vector
  for (size_t i = 0; i < current_vl; i++) {
    uint16_t output_val = result.final_output[0][i];
    p->VU.elt<uint16_t>(insn.rd(), i) = output_val;
    fprintf(stderr, "xperiv: softmax vd[%lu] = 0x%04x\n", i, output_val);
  }
  
  fprintf(stderr, "xperiv: softmax_impl returning\n");
  return pc + 4;
}

// ============================================================================
// QUANT instruction implementation
// ============================================================================
static reg_t quant_impl(processor_t* p, insn_t insn, reg_t pc)
{
  fprintf(stderr, "xperiv: quant_impl called! vd=%lu, vs1=%lu, vs2=%lu\n", 
          insn.rd(), insn.rs1(), insn.rs2());
  
  // Get vector length
  auto vlen = p->VU.get_vlen() / 8;
  auto elen = p->VU.vsew;
  auto current_vl = p->VU.vl->read();
  fprintf(stderr, "xperiv: vlen=%lu, elen=%lu, vl=%lu\n", vlen, elen, current_vl);
  
  // Initialize BF16 LUTs
  BF16::init_luts();
  
  // Collect input values from vs1 vector
  std::vector<uint16_t> input_values;
  for (size_t i = 0; i < current_vl; i++) {
    uint16_t val = p->VU.elt<uint16_t>(insn.rs1(), i);
    input_values.push_back(val);
  }
  
  // Create MxFp8ActQuant_Model instance
  MxFp8ActQuant_Model quant_model(current_vl);
  
  // Process quantization
  quant_model.process(input_values.data());
  
  // Store results back to vd vector (8-bit values in 16-bit elements)
  for (size_t i = 0; i < current_vl; i++) {
    uint8_t quantized = quant_model.o_MxFp8Act[i];
    // Store 8-bit value in 16-bit element (upper 8 bits are zero)
    p->VU.elt<uint16_t>(insn.rd(), i) = quantized;
    fprintf(stderr, "xperiv: quant vd[%lu] = 0x%02x\n", i, quantized);
  }
  
  fprintf(stderr, "xperiv: quant_impl returning\n");
  return pc + 4;
}

class xperiv_t : public extension_t
{
public:
  const char* name() const override { return "xperiv"; }
  
  std::vector<insn_desc_t> get_instructions(const processor_t &) override {
    fprintf(stderr, "xperiv get_instructions called!\n");

    std::vector<insn_desc_t> insns;
    
    insns.push_back({MATCH_PERI_V_ADD, MASK_PERI_V_ADD, 
                     peri_v_add_impl, peri_v_add_impl, peri_v_add_impl, peri_v_add_impl,
                     peri_v_add_impl, peri_v_add_impl, peri_v_add_impl, peri_v_add_impl});

    insns.push_back({MATCH_PERI_V_MUL, MASK_PERI_V_MUL,
                     peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl,
                     peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl});
    
    // Add new mathematical instructions
    insns.push_back({MATCH_EXP, MASK_CUSTOM0,
                     exp_impl, exp_impl, exp_impl, exp_impl,
                     exp_impl, exp_impl, exp_impl, exp_impl});
    
    insns.push_back({MATCH_SOFTMAX, MASK_CUSTOM0,
                     softmax_impl, softmax_impl, softmax_impl, softmax_impl,
                     softmax_impl, softmax_impl, softmax_impl, softmax_impl});
    
    insns.push_back({MATCH_QUANT, MASK_CUSTOM0,
                     quant_impl, quant_impl, quant_impl, quant_impl,
                     quant_impl, quant_impl, quant_impl, quant_impl});
    
    return insns;
  }
  
  std::vector<disasm_insn_t*> get_disasms(const processor_t *) override {
    std::vector<disasm_insn_t*> insns;
    
    insns.push_back(new disasm_insn_t("peri.v.add", MATCH_PERI_V_ADD, MASK_PERI_V_ADD, {&xvd, &xvs1, &xvs2}));
    insns.push_back(new disasm_insn_t("peri.v.mul", MATCH_PERI_V_MUL, MASK_PERI_V_MUL, {&xvd, &xvs1, &xvs2}));
    
    // Add disassembly for new instructions
    insns.push_back(new disasm_insn_t("exp", MATCH_EXP, MASK_CUSTOM0, {&xvd, &xvs1, &xvs2}));
    insns.push_back(new disasm_insn_t("softmax", MATCH_SOFTMAX, MASK_CUSTOM0, {&xvd, &xvs1, &xvs2}));
    insns.push_back(new disasm_insn_t("quant", MATCH_QUANT, MASK_CUSTOM0, {&xvd, &xvs1, &xvs2}));
    
    return insns;
  }
};

REGISTER_EXTENSION(periv, []() { fprintf(stderr, "xperiv factory called!\n"); return new xperiv_t; })
