/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2019 FORTH-ICS/CARV
 *				Panagiotis Peristerakis <perister@ics.forth.gr>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/irqchip/clic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define CHESHIRE_UART_ADDR	          0x03002000
#define CHESHIRE_UART_FREQ	          50000000
#define CHESHIRE_UART_BAUDRATE	      115200
#define CHESHIRE_UART_REG_SHIFT	      2
#define CHESHIRE_UART_REG_WIDTH	      4
#define CHESHIRE_PLIC_ADDR	          0x0c000000UL
#define CHESHIRE_PLIC_NUM_SOURCES     20
#define CHESHIRE_CLIC_ADDR              0x08000000ul
#define CHESHIRE_CLIC_NUM_SOURCES		  64
#define CHESHIRE_HART_COUNT	          1
#define CHESHIRE_CLINT_ADDR	          0x02040000
#define CHESHIRE_ACLINT_MTIMER_FREQ   1000000
#define CHESHIRE_ACLINT_MSWI_ADDR     (CHESHIRE_CLINT_ADDR + 0x0)
#define CHESHIRE_ACLINT_MTIMER_ADDR   (CHESHIRE_CLINT_ADDR + 0xbff8)
#define CHESHIRE_ACLINT_MTIMECMP_ADDR (CHESHIRE_CLINT_ADDR + 0x4000)

static struct platform_uart_data uart = {
	CHESHIRE_UART_ADDR,
	CHESHIRE_UART_FREQ,
	CHESHIRE_UART_BAUDRATE,
};

static struct clic_data clic = {
	.addr = CHESHIRE_CLIC_ADDR,
	.num_src = CHESHIRE_CLIC_NUM_SOURCES,
};

static struct plic_data plic = {
	.addr    = CHESHIRE_PLIC_ADDR,
	.num_src = CHESHIRE_PLIC_NUM_SOURCES,
};

static struct aclint_mswi_data mswi = {
	.addr         = CHESHIRE_ACLINT_MSWI_ADDR,
	.size         = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count   = CHESHIRE_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq     = CHESHIRE_ACLINT_MTIMER_FREQ,
	.mtime_addr     = CHESHIRE_ACLINT_MTIMER_ADDR,
	.mtime_size     = 8,
	.mtimecmp_addr  = CHESHIRE_ACLINT_MTIMECMP_ADDR,
	.mtimecmp_size  = 16,
	.first_hartid   = 0,
	.hart_count     = CHESHIRE_HART_COUNT,
	.has_64bit_mmio = 0,
};

/*
 * Cheshire platform early initialization.
 */
static int cheshire_early_init(bool cold_boot)
{
	return 0;
}

/*
 * Cheshire platform final initialization.
 */
static int cheshire_final_init(bool cold_boot)
{
	return 0;
}

/*
 * Initialize the cheshire console.
 */
static int cheshire_console_init(void)
{
	return uart8250_init(uart.addr, 
						 uart.freq,
						 uart.baud,
						 CHESHIRE_UART_REG_SHIFT,
						 CHESHIRE_UART_REG_WIDTH,
						 0);
}

static int plic_cheshire_warm_irqchip_init(int m_cntx_id, int s_cntx_id)
{
	return plic_warm_irqchip_init(&plic, m_cntx_id, s_cntx_id);
}

/*
 * Initialize the cheshire interrupt controller for current HART.
 */
static int cheshire_irqchip_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	int ret;

	if (cold_boot) {
		ret = plic_cold_irqchip_init(&plic);
		if (ret)
			return ret;
	}
	ret = clic_init(&clic);
	if (ret)
		return ret;
	return plic_cheshire_warm_irqchip_init(2 * hartid, 2 * hartid + 1);
}

/*
 * Initialize IPI for current HART.
 */
static int cheshire_ipi_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = aclint_mswi_cold_init(&mswi);
		if (ret)
			return ret;
		clic_set_enable(IRQ_M_SOFT, 1);
		clic_set_priority(IRQ_M_SOFT, 255);
	}

	return aclint_mswi_warm_init();
}

/*
 * Initialize cheshire timer for current HART.
 */
static int cheshire_timer_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = aclint_mtimer_cold_init(&mtimer, NULL);
		if (ret)
			return ret;
	}

	return aclint_mtimer_warm_init();
}

static int cheshire_clic_delegate(u32 irq)
{
	clic_delegate(&clic, irq);
	return 0;
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init   = cheshire_early_init,
	.final_init   = cheshire_final_init,
	.console_init = cheshire_console_init,
	.irqchip_init = cheshire_irqchip_init,
	.ipi_init     = cheshire_ipi_init,
	.timer_init   = cheshire_timer_init,
	.irqctl_delegate = cheshire_clic_delegate,
};

const struct sbi_platform platform = {
	.opensbi_version   = OPENSBI_VERSION,
	.platform_version  = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name              = "CHESHIRE RISC-V",
	.features          = SBI_PLATFORM_HAS_MFAULTS_DELEGATION | SBI_PLATFORM_HAS_CLIC,
	.hart_count        = CHESHIRE_HART_COUNT,
	.hart_stack_size   = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops
};
