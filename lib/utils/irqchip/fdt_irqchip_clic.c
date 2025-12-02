#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/clic.h>

static int irqchip_clic_warm_init(void)
{
	/* Nothing to do here. */
	return 0;
}

static int irqchip_clic_cold_init(void)
{
	// TODO: implement
	return 0;
}

static const struct fdt_match irqchip_clic_match[] = {
	{ .compatible = "riscv,clic" },
	{ },
};

struct fdt_irqchip fdt_irqchip_clic = {
	.match_table = irqchip_clic_match,
	.cold_init = irqchip_clic_cold_init,
	.warm_init = irqchip_clic_warm_init,
	.exit = NULL,
};
