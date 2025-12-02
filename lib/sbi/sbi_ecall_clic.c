#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/irqchip/clic.h>

static int sbi_ecall_clic_probe(unsigned long extid,
          unsigned long *out_val)
{
  struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
  const struct sbi_platform *plat = sbi_platform_ptr(scratch);
  *out_val = (sbi_platform_get_features(plat) & SBI_PLATFORM_HAS_CLIC) != 0UL ? 1UL : 0UL;
  return 0;
}

static void sbi_ecall_clic_enable()
{
  unsigned long _mtvec = csr_read(mtvec);
  _mtvec |= 0x03ul;
  csr_write(mtvec, _mtvec);
  _mtvec = csr_read(mtvec);
}

static int sbi_ecall_clic_handler(unsigned long extid, unsigned long funcid,
            const struct sbi_trap_regs *regs,
            unsigned long *out_val,
            struct sbi_trap_info *out_trap)
{
  int ret = 0;

  struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

  if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_CLIC))
    return SBI_ENOTSUPP;

  switch (funcid) {
    case SBI_EXT_CLIC_ENABLE:
      // sbi_printf("[SBI] ECALL to SBI_EXT_CLIC_ENABLE\n");
      sbi_ecall_clic_enable();
      break;
    case SBI_EXT_CLIC_DELEGATE:
      unsigned long irq = regs->a0;
      // sbi_printf("[SBI] ECALL to SBI_EXT_CLIC_DELEGATE (irq=%lu)\n", irq);
      if(irq == IRQ_M_SOFT || irq == IRQ_M_TIMER || irq == IRQ_M_EXT) {
        // sbi_printf("[SBI] WARN: refusing delegation of M-mode interrupt\n");
        ret = SBI_EDENIED;
      } else {
        sbi_platform_irqctl_delegate(sbi_platform_ptr(scratch), irq);
      }
      break;
    case SBI_EXT_CLIC_GET_NUMSRCS:
      *out_val = clic_get_num_sources();
      break;
    default:
      ret = SBI_ENOTSUPP;
  }

  return ret;
}

struct sbi_ecall_extension ecall_clic = {
  .extid_start = SBI_EXT_CLIC_START,
  .extid_end   = SBI_EXT_CLIC_END,
  .probe       = sbi_ecall_clic_probe,
  .handle      = sbi_ecall_clic_handler,
};