#include <sbi/riscv_asm.h>
#include <sbi/sbi_hartmask.h>
#include <sbi_utils/irqchip/clic.h>
#include <sbi/sbi_console.h>

#define CLIC_CLICCFG_OFFSET          0x0000000ul
#define CLIC_CLICINT_OFFSET(i)      (0x0001000ul + 4*(i))
#define CLIC_CLICINTV_OFFSET(i)     (0x0005000ul + 4*(i))

#define CLIC_CLICCFG_MNLBITS_SHIFT   0
#define CLIC_CLICCFG_NMBITS_SHIFT    4
#define CLIC_CLICCFG_SNLBITS_SHIFT  16
#define CLIC_CLICCFG_UNLBITS_SHIFT  24

#define CLIC_CLICCFG_MNLBITS_MASK   0x0000000Fu
#define CLIC_CLICCFG_NMBITS_MASK    0x00000030u
#define CLIC_CLICCFG_SNLBITS_MASK   0x000F0000u
#define CLIC_CLICCFG_UNLBITS_MASK   0x0F000000u

#define CLIC_CLICINT_IP_OFFSET         0
#define CLIC_CLICINT_IE_OFFSET         8
#define CLIC_CLICINT_ATTR_SHV_OFFSET  16
#define CLIC_CLICINT_ATTR_TRIG_OFFSET 17
#define CLIC_CLICINT_ATTR_MODE_OFFSET 22
#define CLIC_CLICINT_CTL_OFFSET       24

#define CLIC_CLICINT_IP_MASK         0x00000001
#define CLIC_CLICINT_IE_MASK         0x00000100
#define CLIC_CLICINT_ATTR_SHV_MASK   0x00010000
#define CLIC_CLICINT_ATTR_TRIG_MASK  0x00060000
#define CLIC_CLICINT_ATTR_MODE_MASK  0x00C00000
#define CLIC_CLICINT_CTL_MASK        0xFF000000

#define CLIC_CLICINTV_V_OFFSET         0
#define CLIC_CLICINTV_VSID_OFFSET      2

#define CLIC_CLICINTV_V_MASK         0x00000001
#define CLIC_CLICINTV_VSID_MASK      0x000000FC

/* Emits the code to fill a `clic_intcfg_t` struct given a `uint32_t` variable containing the value of a CLICINT register */
#define CLIC_PARSE_CLICINT(cfg, clicint) \
    cfg.ip        = (clicint & CLIC_CLICINT_IP_MASK)        >> CLIC_CLICINT_IP_OFFSET;        \
    cfg.ie        = (clicint & CLIC_CLICINT_IE_MASK)        >> CLIC_CLICINT_IE_OFFSET;        \
    cfg.attr.shv  = (clicint & CLIC_CLICINT_ATTR_SHV_MASK)  >> CLIC_CLICINT_ATTR_SHV_OFFSET;  \
    cfg.attr.trig = (clicint & CLIC_CLICINT_ATTR_TRIG_MASK) >> CLIC_CLICINT_ATTR_TRIG_OFFSET; \
    cfg.attr.mode = (clicint & CLIC_CLICINT_ATTR_MODE_MASK) >> CLIC_CLICINT_ATTR_MODE_OFFSET; \
    cfg.ctl       = (clicint & CLIC_CLICINT_CTL_MASK)       >> CLIC_CLICINT_CTL_OFFSET;

const struct clic_data* clic_dev;

static inline void write32(u32 val, volatile void *addr)
{
  asm volatile("sw %0, 0(%1)" : : "r"(val), "r"(addr));
}

static inline u32 read32(const volatile void *addr)
{
  u32 val;

  asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
  return val;
}


void clic_set_mcfg(const struct clic_data* clic, u8 nmbits, u8 mnlbits, u8 snlbits, u8 unlbits)
{
  u32 clic_cfg_val = (((u32) nmbits  << CLIC_CLICCFG_NMBITS_SHIFT)  & CLIC_CLICCFG_NMBITS_MASK)  | 
                      (((u32) mnlbits << CLIC_CLICCFG_MNLBITS_SHIFT) & CLIC_CLICCFG_MNLBITS_MASK) | 
                      (((u32) snlbits << CLIC_CLICCFG_SNLBITS_SHIFT) & CLIC_CLICCFG_SNLBITS_MASK) |
                      (((u32) unlbits << CLIC_CLICCFG_UNLBITS_SHIFT) & CLIC_CLICCFG_UNLBITS_MASK);
  write32(clic_cfg_val, (void *) (clic->addr + CLIC_CLICCFG_OFFSET));
}

void clic_set_clicint(const struct clic_data* clic, u32 i, clic_intcfg_t cfg)
{
  u32 clic_int_val = (((u32) cfg.ip        << CLIC_CLICINT_IP_OFFSET)         & CLIC_CLICINT_IP_MASK)        |
                     (((u32) cfg.ie        << CLIC_CLICINT_IE_OFFSET)         & CLIC_CLICINT_IE_MASK)        |
                     (((u32) cfg.attr.shv  << CLIC_CLICINT_ATTR_SHV_OFFSET)   & CLIC_CLICINT_ATTR_SHV_MASK)  |  
                     (((u32) cfg.attr.trig << CLIC_CLICINT_ATTR_TRIG_OFFSET)  & CLIC_CLICINT_ATTR_TRIG_MASK) |  
                     (((u32) cfg.attr.mode << CLIC_CLICINT_ATTR_MODE_OFFSET)  & CLIC_CLICINT_ATTR_MODE_MASK) |  
                     (((u32) cfg.ctl       << CLIC_CLICINT_CTL_OFFSET)        & CLIC_CLICINT_CTL_MASK);
  write32(clic_int_val, (void *) (clic->addr + CLIC_CLICINT_OFFSET(i)));
}

/* Reads the CLICINT configuration register for interrupt `irq_id`. Returns a `clic_intcfg_t` struct */
clic_intcfg_t clic_read_clicint(const struct clic_data* clic, u32 irq)
{
    u32 clicint = read32((void *) (clic->addr + CLIC_CLICINT_OFFSET(irq)));
    clic_intcfg_t cfg;
    CLIC_PARSE_CLICINT(cfg, clicint);
    return cfg;
}

/* Set enable bit for interrupt `irq_id` */
void clic_set_enable(u32 irq, bool en)
{
    clic_intcfg_t intcfg = clic_read_clicint(clic_dev, irq);
    intcfg.ie = en ? CLIC_IE_ENABLE : CLIC_IE_MASK;
    clic_set_clicint(clic_dev, irq, intcfg);
}

/* Set interrupt pending bit */
void clic_set_pend(u32 irq, bool pending)
{
    clic_intcfg_t intcfg = clic_read_clicint(clic_dev, irq);
    intcfg.ip = pending ? CLIC_IP_SET : CLIC_IP_CLEAR;
    clic_set_clicint(clic_dev, irq, intcfg);
}

/* Set priority / level for interrupt `irq_id` */
void clic_set_priority(u32 irq, u8 prio)
{
    clic_intcfg_t intcfg = clic_read_clicint(clic_dev, irq);
    intcfg.ctl = prio;
    clic_set_clicint(clic_dev, irq, intcfg);
}

/* Set priority / level for interrupt `irq_id` */
void clic_set_trigger(u32 irq, clic_intattr_trig_t trig)
{
    clic_intcfg_t intcfg = clic_read_clicint(clic_dev, irq);
    intcfg.attr.trig = trig;
    clic_set_clicint(clic_dev, irq, intcfg);
}

void clic_delegate(const struct clic_data* clic, u32 irq)
{
  clic_intcfg_t cfg = {
    .ip        = CLIC_IP_CLEAR,
    .ie        = CLIC_IE_MASK,
    .attr.shv  = CLIC_INT_ATTR_SHV_OFF,
    .attr.trig = CLIC_INT_ATTR_TRIG_EDGE, // CLIC_INT_ATTR_TRIG_LVL,
    .attr.mode = CLIC_INT_ATTR_MODE_S,
    .ctl       = 0x00
  };
  clic_set_clicint(clic, irq, cfg);
}

void clic_send_ipi(u32 target_hart)
{
  sbi_printf("[SBI] Writing CLIC IP 1\n");
  clic_set_pend(1, 1);
}

void clic_clear_ipi(u32 target_hart)
{
  clic_set_pend(1, 0);
}

unsigned long clic_get_num_sources()
{
  return clic_dev->num_src;
}

int clic_init(const struct clic_data* clic)
{

  clic_dev = clic;

  clic_set_mcfg(clic, 0xFF, 0xFF, 0xFF, 0xFF);
  clic_intcfg_t cfg = {
      .ip        = CLIC_IP_CLEAR,
      .ie        = CLIC_IE_MASK,
      .attr.shv  = CLIC_INT_ATTR_SHV_OFF,
      .attr.trig = CLIC_INT_ATTR_TRIG_LVL,
      .attr.mode = CLIC_INT_ATTR_MODE_M,
      .ctl       = 0x00
  };
  for(uint32_t i = 0; i < clic->num_src; ++i) {
    clic_set_clicint(clic, i, cfg);
  }
  return 0;
}