/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004-2007 Cavium Networks
 */
#include <linux/console.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>

#include <asm/time.h>

#include <asm/octeon/octeon.h>

#if defined(CONFIG_CAVIUM_GDB)

#ifdef CONFIG_GDB_CONSOLE
#define DEBUG_UART 0
#else
#define DEBUG_UART 1
#endif
static irqreturn_t interrupt_debug_char(int cpl, void *dev_id)
{
	unsigned long lsrval;
	unsigned long tmp;

	lsrval = cvmx_read_csr(CVMX_MIO_UARTX_LSR(DEBUG_UART));
	if (lsrval & 1) {
#ifdef CONFIG_KGDB
		/*
		 * The Cavium EJTAG bootmonitor stub is not compatible
		 * with KGDB.  We should never get here.
		 */
#error Illegal to use both CONFIG_KGDB and CONFIG_CAVIUM_GDB
#endif
		/*
		 * Pulse MCD0 signal on Ctrl-C to stop all the
		 * cores. Also set the MCD0 to be not masked by this
		 * core so we know the signal is received by
		 * someone.
		 */
		octeon_write_lcd("brk");
		asm volatile ("dmfc0 %0, $22\n\t"
			      "ori   %0, %0, 0x10\n\t"
			      "dmtc0 %0, $22" : "=r" (tmp));
		octeon_write_lcd("");
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

/* Enable uart1 interrupts for debugger Control-C processing */

static int octeon_setup_debug_uart(void)
{
	if (request_irq(OCTEON_IRQ_UART0 + DEBUG_UART, interrupt_debug_char,
			IRQF_SHARED, "KGDB", interrupt_debug_char)) {
		panic("request_irq(%d) failed.", OCTEON_IRQ_UART0 + DEBUG_UART);
	}
	cvmx_write_csr(CVMX_MIO_UARTX_IER(DEBUG_UART),
		       cvmx_read_csr(CVMX_MIO_UARTX_IER(DEBUG_UART)) | 1);
	return 0;
}

/* Install this as early as possible to be able to debug the boot
   sequence.  */
core_initcall(octeon_setup_debug_uart);
#endif	/* CONFIG_CAVIUM_GDB */

unsigned int octeon_serial_in(struct uart_port *up, int offset)
{
	int rv = cvmx_read_csr((uint64_t)(up->membase + (offset << 3)));
	if (offset == UART_IIR && (rv & 0xf) == 7) {
		/* Busy interrupt, read the USR (39) and try again. */
		cvmx_read_csr((uint64_t)(up->membase + (39 << 3)));
		rv = cvmx_read_csr((uint64_t)(up->membase + (offset << 3)));
	}
	return rv;
}

void octeon_serial_out(struct uart_port *up, int offset, int value)
{
	/*
	 * If bits 6 or 7 of the OCTEON UART's LCR are set, it quits
	 * working.
	 */
	if (offset == UART_LCR)
		value &= 0x9f;
	cvmx_write_csr((uint64_t)(up->membase + (offset << 3)), (u8)value);
}

/*
 * Allocated in .bss, so it is all zeroed.
 */
#define OCTEON_MAX_UARTS 3
static struct plat_serial8250_port octeon_uart8250_data[OCTEON_MAX_UARTS + 1];
static struct platform_device octeon_uart8250_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= octeon_uart8250_data,
	},
};

static void __init octeon_uart_set_common(struct plat_serial8250_port *p)
{
	p->flags = ASYNC_SKIP_TEST | UPF_SHARE_IRQ | UPF_FIXED_TYPE;
	p->type = PORT_OCTEON;
	p->iotype = UPIO_MEM;
	p->regshift = 3;	/* I/O addresses are every 8 bytes */
	if (octeon_is_simulation())
		/* Make simulator output fast*/
		p->uartclk = 115200 * 16;
	else
		p->uartclk = octeon_get_io_clock_rate();
	p->serial_in = octeon_serial_in;
	p->serial_out = octeon_serial_out;
}

static int __init octeon_serial_init(void)
{
	int enable_uart0;
	int enable_uart1;
	int enable_uart2;
	struct plat_serial8250_port *p;

#ifdef CONFIG_CAVIUM_OCTEON_2ND_KERNEL
	/*
	 * If we are configured to run as the second of two kernels,
	 * disable uart0 and enable uart1. Uart0 is owned by the first
	 * kernel
	 */
	enable_uart0 = 0;
	enable_uart1 = 1;
#else
	/*
	 * We are configured for the first kernel. We'll enable uart0
	 * if the bootloader told us to use 0, otherwise will enable
	 * uart 1.
	 */
	enable_uart0 = (octeon_get_boot_uart() == 0);
	enable_uart1 = (octeon_get_boot_uart() == 1);
#ifdef CONFIG_KGDB
	enable_uart1 = 1;
#endif
#endif
#ifdef CONFIG_CAVIUM_GDB
	enable_uart1 = 0;
#endif
	/* Right now CN52XX is the only chip with a third uart */
	enable_uart2 = OCTEON_IS_MODEL(OCTEON_CN52XX);

	p = octeon_uart8250_data;
	if (enable_uart0) {
		/* Add a ttyS device for hardware uart 0 */
		octeon_uart_set_common(p);
		p->membase = (void *) CVMX_MIO_UARTX_RBR(0);
		p->mapbase = CVMX_MIO_UARTX_RBR(0) & ((1ull << 49) - 1);
		p->irq = OCTEON_IRQ_UART0;
		p++;
	}

	if (enable_uart1) {
		/* Add a ttyS device for hardware uart 1 */
		octeon_uart_set_common(p);
		p->membase = (void *) CVMX_MIO_UARTX_RBR(1);
		p->mapbase = CVMX_MIO_UARTX_RBR(1) & ((1ull << 49) - 1);
		p->irq = OCTEON_IRQ_UART1;
		p++;
	}
	if (enable_uart2) {
		/* Add a ttyS device for hardware uart 2 */
		octeon_uart_set_common(p);
		p->membase = (void *) CVMX_MIO_UART2_RBR;
		p->mapbase = CVMX_MIO_UART2_RBR & ((1ull << 49) - 1);
		p->irq = OCTEON_IRQ_UART2;
		p++;
	}

	BUG_ON(p > &octeon_uart8250_data[OCTEON_MAX_UARTS]);

	return platform_device_register(&octeon_uart8250_device);
}

device_initcall(octeon_serial_init);
