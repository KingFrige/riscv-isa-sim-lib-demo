#include "custom_csr.h"
#include "processor.h"
#include <cassert>

// Mail data CSR 实现
mail_data_csr_t::mail_data_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

reg_t mail_data_csr_t::read() const noexcept {
    return val;
}

bool mail_data_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val;
    return true;
}

// Mail valid CSR 实现
mail_valid_csr_t::mail_valid_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

void mail_valid_csr_t::verify_permissions(insn_t insn, bool write) const {
    csr_t::verify_permissions(insn, write);
}

reg_t mail_valid_csr_t::read() const noexcept {
    return val & 0x1;
}

bool mail_valid_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val & 0x1;
    return true;
}

// BO done CSR 实现
bo_done_csr_t::bo_done_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

reg_t bo_done_csr_t::read() const noexcept {
    return val & 0x7FF;  // 11-bit mask: [10:6]=wg_index, [5:0]=bar_index
}

bool bo_done_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val & 0x7FF;
    return true;
}

// SE up CSR 实现
se_up_csr_t::se_up_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

reg_t se_up_csr_t::read() const noexcept {
    return val & 0x7FF;
}

bool se_up_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val & 0x7FF;
    return true;
}

// SE query lock CSR 实现
se_query_lock_csr_t::se_query_lock_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

reg_t se_query_lock_csr_t::read() const noexcept {
    return val & 0x7FF;
}

bool se_query_lock_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val & 0x7FF;
    return true;
}

// SE query count CSR 实现
se_query_count_csr_t::se_query_count_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
    : basic_csr_t(proc, addr, init), val(init) {
}

reg_t se_query_count_csr_t::read() const noexcept {
    return val;
}

bool se_query_count_csr_t::unlogged_write(const reg_t val) noexcept {
    this->val = val;
    return true;
}

// 注册自定义 CSR 到处理器
void register_custom_csrs(processor_t* proc) {
    state_t* state = proc->get_state();
    
    // 添加 M 模式的 CSR (地址 0xF2x 对应 M-mode: 0xF20 | 0x300 = 0xF20)
    // CSR 地址格式: [11:10]=rw, [9:8]=0, [7:6]=0, [5:4]=priv, [3:0]=reg
    // 0xF20 = 0b1111_0010_0000 (M-mode, read/write)
    
    fprintf(stderr, "[CSR] Registering custom CSRs for processor %p\n", (void*)proc);
    
    state->add_csr(CSR_MAIL_DATA0, std::make_shared<mail_data_csr_t>(proc, CSR_MAIL_DATA0, 0));
    state->add_csr(CSR_MAIL_DATA1, std::make_shared<mail_data_csr_t>(proc, CSR_MAIL_DATA1, 0));
    state->add_csr(CSR_MAIL_DATA2, std::make_shared<mail_data_csr_t>(proc, CSR_MAIL_DATA2, 0));
    state->add_csr(CSR_MAIL_DATA3, std::make_shared<mail_data_csr_t>(proc, CSR_MAIL_DATA3, 0));
    state->add_csr(CSR_MAIL_VALID, std::make_shared<mail_valid_csr_t>(proc, CSR_MAIL_VALID, 0));
    state->add_csr(CSR_BO_DONE, std::make_shared<bo_done_csr_t>(proc, CSR_BO_DONE, 0));
    state->add_csr(CSR_SE_UP, std::make_shared<se_up_csr_t>(proc, CSR_SE_UP, 0));
    state->add_csr(CSR_SE_QUERY_LOCK, std::make_shared<se_query_lock_csr_t>(proc, CSR_SE_QUERY_LOCK, 0));
    state->add_csr(CSR_SE_QUERY_COUNT, std::make_shared<se_query_count_csr_t>(proc, CSR_SE_QUERY_COUNT, 0));
    
    // 验证 CSR 是否被添加
    fprintf(stderr, "[CSR] Verifying CSR registration...\n");
    fprintf(stderr, "[CSR] CSR_MAIL_DATA0 (0x%x) found: %d\n", CSR_MAIL_DATA0, 
            state->csrmap.find(CSR_MAIL_DATA0) != state->csrmap.end());
    fprintf(stderr, "[CSR] CSR_MAIL_DATA1 (0x%x) found: %d\n", CSR_MAIL_DATA1, 
            state->csrmap.find(CSR_MAIL_DATA1) != state->csrmap.end());
    
    fprintf(stderr, "[CSR] Custom CSRs registered successfully\n");
}
