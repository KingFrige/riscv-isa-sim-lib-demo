#ifndef _CUSTOM_CSR_H
#define _CUSTOM_CSR_H

#include <cstdint>
#include <cstddef>
#include "csrs.h"
#include "../common/csr_addr.h"

// Mail data CSR (64-bit)
class mail_data_csr_t : public basic_csr_t {
public:
    mail_data_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// Mail valid CSR (1-bit)
class mail_valid_csr_t : public basic_csr_t {
public:
    mail_valid_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual void verify_permissions(insn_t insn, bool write) const override;
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// BO done CSR (11-bit)
class bo_done_csr_t : public basic_csr_t {
public:
    bo_done_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// SE up CSR (11-bit)
class se_up_csr_t : public basic_csr_t {
public:
    se_up_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// SE query lock CSR (11-bit)
class se_query_lock_csr_t : public basic_csr_t {
public:
    se_query_lock_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// SE query count CSR
class se_query_count_csr_t : public basic_csr_t {
public:
    se_query_count_csr_t(processor_t* const proc, const reg_t addr, const reg_t init);
    virtual reg_t read() const noexcept override;
protected:
    virtual bool unlogged_write(const reg_t val) noexcept override;
private:
    reg_t val;
};

// 注入自定义 CSR 的函数
void register_custom_csrs(processor_t* proc);

#endif // _CUSTOM_CSR_H
