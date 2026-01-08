#define DECODE_MACRO_USAGE_LOGGED 0
#include <sys/syscall.h>
#include "extension.h"
#include "processor.h"
#include "decode.h"
#include "insn_macros.h"
#include "decode_macros.h"


#define MATCH_PERI_B_MUL 0x0000004b
#define MASK_PERI_B_MUL  0xfe00707f 

#define xlen 64

// Argument descriptor for disassembly (reuse from xperia)
struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr_name[insn.rs1()];
  }
} xrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr_name[insn.rs2()];
  }
} xrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr_name[insn.rd()];
  }
} xrd;

static reg_t peri_b_mul_impl(processor_t* p, insn_t insn, reg_t pc)
{
  WRITE_RD(sext_xlen(RS1 * RS2));
  return pc + 4;
}

class xperib_t : public extension_t
{
public:
  const char* name() const override { return "xperib"; }
  
  std::vector<insn_desc_t> get_instructions(const processor_t &) override {
    fprintf(stderr, "xperib get_instructions called!\n");

    std::vector<insn_desc_t> insns;
    
    insns.push_back({MATCH_PERI_B_MUL, MASK_PERI_B_MUL, 
                     peri_b_mul_impl, peri_b_mul_impl, peri_b_mul_impl, peri_b_mul_impl,
                     peri_b_mul_impl, peri_b_mul_impl, peri_b_mul_impl, peri_b_mul_impl});
    
    return insns;
  }
  
  std::vector<disasm_insn_t*> get_disasms(const processor_t *) override {
    std::vector<disasm_insn_t*> insns;
    
    insns.push_back(new disasm_insn_t("peri.b.mul", MATCH_PERI_B_MUL, MASK_PERI_B_MUL, {&xrd, &xrs1, &xrs2}));
    
    return insns;
  }
};

REGISTER_EXTENSION(perib, []() { fprintf(stderr, "xxxperib factory called!\n"); return new xperib_t; })