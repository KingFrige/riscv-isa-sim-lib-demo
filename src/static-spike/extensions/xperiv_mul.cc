#define DECODE_MACRO_USAGE_LOGGED 0
#include <sys/syscall.h>
#include "extension.h"
#include "processor.h"
#include "decode.h"
#include "insn_macros.h"
#include "decode_macros.h"
#include "v_ext_macros.h"

#ifndef xlen
#define xlen 64
#endif

#define MATCH_PERI_V_MUL 0x0000005b
#define MASK_PERI_V_MUL  0xfc00707f

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

static reg_t peri_v_mul_impl(processor_t* p, insn_t insn, reg_t pc)
{
  VI_VV_LOOP
  ({
    vd = vs1 * vs2;
    fprintf(stderr, "xperiv_mul: vd[%lu] = vs1[%lu] + vs2[%lu] = %ld + %ld = %ld\n",
            i, i, i, (long)vs1, (long)vs2, (long)vd);
  })
  return pc + 4;
}

class xperivmul_t : public extension_t
{
public:
  const char* name() const override { return "xperivmul"; }
  
  std::vector<insn_desc_t> get_instructions(const processor_t &) override {
    fprintf(stderr, "xperivmul get_instructions called!\n");

    std::vector<insn_desc_t> insns;
    
    insns.push_back({MATCH_PERI_V_MUL, MASK_PERI_V_MUL, 
                     peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl,
                     peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl, peri_v_mul_impl});
    
    return insns;
  }
  
  std::vector<disasm_insn_t*> get_disasms(const processor_t *) override {
    std::vector<disasm_insn_t*> insns;
    
    insns.push_back(new disasm_insn_t("peri.v.mul", MATCH_PERI_V_MUL, MASK_PERI_V_MUL, {&xvd, &xvs1, &xvs2}));
    
    return insns;
  }
};

REGISTER_EXTENSION(perivmul, []() { fprintf(stderr, "xxxperivmul factory called!\n"); return new xperivmul_t; })
