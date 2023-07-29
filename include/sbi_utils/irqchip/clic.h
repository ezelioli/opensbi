#ifndef __IRQCHIP_CLIC_H__
#define __IRQCHIP_CLIC_H__

#include <sbi/sbi_types.h>

struct clic_data {
    unsigned long addr;
    unsigned long num_src;
    u32 first_hartid;
    u32 hart_count;
};

typedef enum {
    CLIC_INT_ATTR_SHV_OFF   = 0x00,
    CLIC_INT_ATTR_SHV_ON    = 0x01
} clic_intattr_shv_t;

typedef enum {
    CLIC_INT_ATTR_TRIG_LVL  = 0x0,
    CLIC_INT_ATTR_TRIG_EDGE = 0x1
} clic_intattr_trig_t;

typedef enum {
    CLIC_INT_ATTR_MODE_M  = 0x3,
    CLIC_INT_ATTR_MODE_S  = 0x1,
    CLIC_INT_ATTR_MODE_U  = 0x0,
} clic_intattr_mode_t;

typedef enum {
    CLIC_IP_CLEAR = 0x00,
    CLIC_IP_SET   = 0x01
} clic_ip_t;

typedef enum {
    CLIC_IE_MASK   = 0x00,
    CLIC_IE_ENABLE = 0x01
} clic_ie_t;

typedef enum {
    CLIC_V_DISABLE = 0x00,
    CLIC_V_ENABLE  = 0x01
} clic_v_t;

typedef struct {
    clic_intattr_shv_t  shv;
    clic_intattr_trig_t trig;
    clic_intattr_mode_t mode;
} clic_intattr_t;

typedef struct {
    clic_v_t       v;
    u8             vsid;
} clic_intvcfg_t;

typedef struct {
    clic_ip_t      ip;
    clic_ie_t      ie;
    clic_intattr_t attr;
    u8             ctl;
} clic_intcfg_t;

void clic_set_mcfg(const struct clic_data* clic, u8 nmbits, u8 mnlbits, u8 snlbits, u8 unlbits);
void clic_set_clicint(const struct clic_data* clic, u32 i, clic_intcfg_t cfg);
void clic_delegate(const struct clic_data* clic, u32 irq);
void clic_set_enable(u32 irq, bool en);
void clic_set_pend(u32 irq, bool pending);
void clic_set_priority(u32 irq, u8 prio);
void clic_set_trigger(u32 irq, clic_intattr_trig_t trig);
unsigned long clic_get_num_sources();
int clic_init(const struct clic_data* clic);

#endif