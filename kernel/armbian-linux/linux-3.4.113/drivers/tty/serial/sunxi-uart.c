/*
 * drivers/tty/serial/sunxi-uart.c
 * (C) Copyright 2007-2013
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Aaron.Maoye <leafy.myeh@reuuimllatech.com>
 *
 * Driver of Allwinner UART controller.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.6.6 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to support sun8i/sun9i of Allwinner.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>

#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/clk/sunxi.h>
#include <linux/pinctrl/consumer.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>

#include "sunxi-uart.h"

#define CONFIG_SW_UART_FORCE_LCR
//#define CONFIG_SW_UART_DUMP_DATA
/*
 * ********************* Note **********************
 * CONFIG_SW_UART_DUMP_DATA may cause some problems
 * with some commands of 'dmesg', 'logcat', and
 * 'cat /proc/kmsg' in the console. This problem may
 * cause kernel to dead. These commands will work fine
 * in the adb shell. So you must be very clear with
 * this problem if you want define this macro to debug.
 */

/* debug control */
#define SERIAL_DBG(fmt, arg...)	\
			do { \
				if (sw_uport->port.line != 0) \
					pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg); \
			} while (0)
#define SERIAL_MSG(fmt, arg...)	pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)

#ifdef CONFIG_SW_UART_DUMP_DATA
static void sw_uart_dump_data(struct sw_uart_port *sw_uport, char* prompt)
{
	int i, j;
	int head = 0;
	char* buf = sw_uport->dump_buff;
	u32 len = sw_uport->dump_len;
	static char pbuff[128];
	u32 idx = 0;

	BUG_ON(sw_uport->dump_len > MAX_DUMP_SIZE);
	BUG_ON(!sw_uport->dump_buff);
	#define MAX_DUMP_PER_LINE	(16)
	#define MAX_DUMP_PER_LINE_HALF	(MAX_DUMP_PER_LINE >> 1)
	printk(KERN_DEBUG "%s len %d\n", prompt, len);
	for (i = 0; i < len;) {
		if ((i & (MAX_DUMP_PER_LINE-1)) == 0) {
			idx += sprintf(&pbuff[idx], "%04x: ", i);
			head = i;
		}
		idx += sprintf(&pbuff[idx], "%02x ", buf[i]&0xff);
		if ((i & (MAX_DUMP_PER_LINE-1)) == MAX_DUMP_PER_LINE-1
			|| i==len-1) {
			for (j=i-head+1; j<MAX_DUMP_PER_LINE; j++)
				idx += sprintf(&pbuff[idx], "   ");
			idx += sprintf(&pbuff[idx], " |");
			for (j=head; j<=i; j++) {
				if (isascii(buf[j]) && isprint(buf[j]))
					idx += sprintf(&pbuff[idx], "%c", buf[j]);
				else
					idx += sprintf(&pbuff[idx], ".");
			}
			idx += sprintf(&pbuff[idx], "|\n");
			pbuff[idx] = '\0';
			printk(KERN_DEBUG "%s", pbuff);
			idx = 0;
		}
		i++;
	}
	sw_uport->dump_len = 0;
}
#define SERIAL_DUMP(up, ...) do { \
				if (DEBUG_CONDITION) \
					sw_uart_dump_data(up, __VA_ARGS__); \
			} while (0)
#else
#define SERIAL_DUMP(up, ...)	{up->dump_len = 0;}
#endif

#define UART_TO_SPORT(port)	((struct sw_uart_port*)port)

static inline unsigned char serial_in(struct uart_port *port, int offs)
{
	return __raw_readb(port->membase + offs);
}

static inline void serial_out(struct uart_port *port, unsigned char value, int offs)
{
	__raw_writeb(value, port->membase + offs);
}

static inline bool sw_is_console_port(struct uart_port *port)
{
	return port->cons && port->cons->index == port->line;
}

static inline void sw_uart_reset(struct sw_uart_port *sw_uport)
{
	sunxi_periph_reset_assert(sw_uport->mclk);
	sunxi_periph_reset_deassert(sw_uport->mclk);
}

static unsigned int sw_uart_handle_rx(struct sw_uart_port *sw_uport, unsigned int lsr)
{
	struct tty_struct *tty = sw_uport->port.state->port.tty;
	unsigned char ch = 0;
	int max_count = 256;
	char flag;

	do {
		if (likely(lsr & SW_UART_LSR_DR)) {
			ch = serial_in(&sw_uport->port, SW_UART_RBR);
#ifdef CONFIG_SW_UART_DUMP_DATA
			sw_uport->dump_buff[sw_uport->dump_len++] = ch;
#endif
		}

		flag = TTY_NORMAL;
		sw_uport->port.icount.rx++;

		if (unlikely(lsr & SW_UART_LSR_BRK_ERROR_BITS)) {
			/*
			 * For statistics only
			 */
			if (lsr & SW_UART_LSR_BI) {
				lsr &= ~(SW_UART_LSR_FE | SW_UART_LSR_PE);
				sw_uport->port.icount.brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (uart_handle_break(&sw_uport->port))
					goto ignore_char;
			} else if (lsr & SW_UART_LSR_PE)
				sw_uport->port.icount.parity++;
			else if (lsr & SW_UART_LSR_FE)
				sw_uport->port.icount.frame++;
			if (lsr & SW_UART_LSR_OE)
				sw_uport->port.icount.overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			lsr &= sw_uport->port.read_status_mask;
#ifdef CONFIG_SERIAL_SUNXI_CONSOLE
			if (sw_is_console_port(&sw_uport->port)) {
				/* Recover the break flag from console xmit */
				lsr |= sw_uport->lsr_break_flag;
			}
#endif
			if (lsr & SW_UART_LSR_BI)
				flag = TTY_BREAK;
			else if (lsr & SW_UART_LSR_PE)
				flag = TTY_PARITY;
			else if (lsr & SW_UART_LSR_FE)
				flag = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(&sw_uport->port, ch))
			goto ignore_char;
		uart_insert_char(&sw_uport->port, lsr, SW_UART_LSR_OE, ch, flag);
ignore_char:
		lsr = serial_in(&sw_uport->port, SW_UART_LSR);
	} while ((lsr & (SW_UART_LSR_DR | SW_UART_LSR_BI)) && (max_count-- > 0));

	SERIAL_DUMP(sw_uport, "Rx");
	spin_unlock(&sw_uport->port.lock);
	tty_flip_buffer_push(tty);
	spin_lock(&sw_uport->port.lock);

	return lsr;
}

static void sw_uart_stop_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (sw_uport->ier & SW_UART_IER_THRI) {
		sw_uport->ier &= ~SW_UART_IER_THRI;
		SERIAL_DBG("stop tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_start_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SW_UART_IER_THRI)) {
		sw_uport->ier |= SW_UART_IER_THRI;
		SERIAL_DBG("start tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_handle_tx(struct sw_uart_port *sw_uport)
{
	struct circ_buf *xmit = &sw_uport->port.state->xmit;
	int count;

	if (sw_uport->port.x_char) {
		serial_out(&sw_uport->port, sw_uport->port.x_char, SW_UART_THR);
		sw_uport->port.icount.tx++;
		sw_uport->port.x_char = 0;
#ifdef CONFIG_SW_UART_DUMP_DATA
		sw_uport->dump_buff[sw_uport->dump_len++] = sw_uport->port.x_char;
		SERIAL_DUMP(sw_uport, "Tx");
#endif
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&sw_uport->port)) {
		sw_uart_stop_tx(&sw_uport->port);
		return;
	}
	count = sw_uport->port.fifosize / 2;
	do {
#ifdef CONFIG_SW_UART_DUMP_DATA
		sw_uport->dump_buff[sw_uport->dump_len++] = xmit->buf[xmit->tail];
#endif
		serial_out(&sw_uport->port, xmit->buf[xmit->tail], SW_UART_THR);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sw_uport->port.icount.tx++;
		if (uart_circ_empty(xmit)) {
			break;
		}
	} while (--count > 0);

	SERIAL_DUMP(sw_uport, "Tx");
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&sw_uport->port.lock);
		uart_write_wakeup(&sw_uport->port);
		spin_lock(&sw_uport->port.lock);
	}
	if (uart_circ_empty(xmit))
		sw_uart_stop_tx(&sw_uport->port);
}

static unsigned int sw_uart_modem_status(struct sw_uart_port *sw_uport)
{
	unsigned int status = serial_in(&sw_uport->port, SW_UART_MSR);

	status |= sw_uport->msr_saved_flags;
	sw_uport->msr_saved_flags = 0;

	if (status & SW_UART_MSR_ANY_DELTA && sw_uport->ier & SW_UART_IER_MSI &&
	    sw_uport->port.state != NULL) {
		if (status & SW_UART_MSR_TERI)
			sw_uport->port.icount.rng++;
		if (status & SW_UART_MSR_DDSR)
			sw_uport->port.icount.dsr++;
		if (status & SW_UART_MSR_DDCD)
			uart_handle_dcd_change(&sw_uport->port, status & SW_UART_MSR_DCD);
		if (!(sw_uport->mcr & SW_UART_MCR_AFE) && status & SW_UART_MSR_DCTS)
			uart_handle_cts_change(&sw_uport->port, status & SW_UART_MSR_CTS);

		wake_up_interruptible(&sw_uport->port.state->port.delta_msr_wait);
	}

	SERIAL_DBG("modem status: %x\n", status);
	return status;
}

static void sw_uart_force_lcr(struct sw_uart_port *sw_uport, unsigned msecs)
{
	unsigned long expire = jiffies + msecs_to_jiffies(msecs);
	struct uart_port *port = &sw_uport->port;

	serial_out(port, SW_UART_HALT_FORCECFG, SW_UART_HALT);
	serial_out(port, sw_uport->dll, SW_UART_DLL);
	serial_out(port, sw_uport->dlh, SW_UART_DLH);
	serial_out(port, sw_uport->lcr, SW_UART_LCR);
	serial_out(port, SW_UART_HALT_FORCECFG|SW_UART_HALT_LCRUP, SW_UART_HALT);
	while (time_before(jiffies, expire) && (serial_in(port, SW_UART_HALT) & SW_UART_HALT_LCRUP));
	/* do not call printk here, yemao 2013-5-20 15:00:38
	if (serial_in(port, SW_UART_HALT) & SW_UART_HALT_LCRUP)
		SERIAL_MSG("uart%d, Force LCR failed\n", sw_uport->id);
	*/
	serial_out(port, 0, SW_UART_HALT);
}

static irqreturn_t sw_uart_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int iir = 0, lsr = 0;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	iir = serial_in(port, SW_UART_IIR) & SW_UART_IIR_IID_MASK;
	lsr = serial_in(port, SW_UART_LSR);
	SERIAL_DBG("irq: iir %x lsr %x\n", iir, lsr);

	if (iir == SW_UART_IIR_IID_BUSBSY) {
		/* handle busy */
		//SERIAL_MSG("uart%d busy...\n", sw_uport->id);
		serial_in(port, SW_UART_USR);

		#ifdef CONFIG_SW_UART_FORCE_LCR
		sw_uart_force_lcr(sw_uport, 10);
		#else
		serial_out(port, sw_uport->lcr, SW_UART_LCR);
		#endif
	} else {
		if (lsr & (SW_UART_LSR_DR | SW_UART_LSR_BI))
			lsr = sw_uart_handle_rx(sw_uport, lsr);
		sw_uart_modem_status(sw_uport);
		#ifdef CONFIG_SW_UART_PTIME_MODE
		if (iir == SW_UART_IIR_IID_THREMP)
		#else
		if (lsr & SW_UART_LSR_THRE)
		#endif
			sw_uart_handle_tx(sw_uport);
	}

	spin_unlock_irqrestore(&port->lock, flags);

	return IRQ_HANDLED;
}

/*
 * uart buadrate and apb2 clock config selection
 * We should select an apb2 clock as low as possible
 * for lower power comsumpition, which can satisfy the
 * different baudrates of different ttyS applications.
 * 
 * the reference table as follows:
 * pll6 600M
 * apb2div      0        20       19       18       17       16       15       14       13       12       11       10       9        8        7        6         5
 * apbclk       24000000 30000000 31578947 33333333 35294117 37500000 40000000 42857142 46153846 50000000 54545454 60000000 66666666 75000000 85714285 100000000 120000000 
 * 115200            *      *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 230400                   *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 380400            *      *         *                 *        *                 *        *        *        *        *        *        *       *         *         *
 * 460800                   *                                    *                 *        *        *        *        *        *        *       *         *         *
 * 921600                   *                                                      *        *                          *                 *       *         *         *
 * 1000000                            *        *                                            *        *                          *                          *
 * 1500000           *                                                                      *        *                                   *                 *         *
 * 1750000                                                                                                    *                                  *
 * 2000000                            *        *                                                                                *                          *
 * 2500000                                                                *                                                                                          *
 * 3000000                                                                                  *        *                                                     *
 * 3250000                                                                                                    *                                            *         
 * 3500000                                                                                                    *
 * 4000000                                                                                                                      *
 */
struct baudset {
	u32 baud;
	u32 uartclk_min;
	u32 uartclk_max;
};

static inline int sw_uart_check_baudset(struct uart_port *port, unsigned int baud)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	static struct baudset  baud_set[] = {
		{115200, 24000000, 120000000},
		{230400, 30000000, 120000000},
		{380400, 24000000, 120000000},
		{460800, 30000000, 120000000},
		{921600, 30000000, 120000000},
		{1000000, 31000000, 120000000}, //31578947
		{1500000, 24000000, 120000000},
		{1750000, 54000000, 120000000}, //54545454
		{2000000, 31000000, 120000000}, //31578947
		{2500000, 40000000, 120000000}, //40000000
		{3000000, 46000000, 120000000}, //46153846
		{3250000, 54000000, 120000000}, //54545454
		{3500000, 54000000, 120000000}, //54545454
		{4000000, 66000000, 120000000}, //66666666
	};
	struct baudset *setsel;
	int i;

	if (baud < 115200) {
		if (port->uartclk < 24000000) {
			SERIAL_MSG("uart%d, uartclk(%d) too small for baud %d\n",
				sw_uport->id, port->uartclk, baud);
			return -1;
		}
	} else {
		for (i=0;
		     i<sizeof(baud_set)/sizeof(baud_set[0]) && baud != baud_set[i].baud;
		     i++);
		if (i==sizeof(baud_set)/sizeof(baud_set[0])) {
			SERIAL_MSG("uart%d, baud %d beyond rance\n", sw_uport->id, baud);
			return -1;
		}
		setsel = &baud_set[i];
		if (port->uartclk < setsel->uartclk_min
			|| port->uartclk > setsel->uartclk_max) {
			SERIAL_MSG("uart%d, select set %d, baud %d, uartclk %d beyond rance[%d, %d]\n",
				sw_uport->id, i, baud, port->uartclk,
				setsel->uartclk_min, setsel->uartclk_max);
			return -1;
		}
	}
	return 0;
}

#define BOTH_EMPTY    (SW_UART_LSR_TEMT | SW_UART_LSR_THRE)
static inline void wait_for_xmitr(struct sw_uart_port *sw_uport)
{
	unsigned int status, tmout = 10000;
	#ifdef CONFIG_SW_UART_PTIME_MODE
	unsigned int offs = SW_UART_USR;
	unsigned char mask = SW_UART_USR_TFNF;
	#else
	unsigned int offs = SW_UART_LSR;
	unsigned char mask = BOTH_EMPTY;
	#endif

	/* Wait up to 10ms for the character(s) to be sent. */
	do {
		status = serial_in(&sw_uport->port, offs);
		if (serial_in(&sw_uport->port, SW_UART_LSR) & SW_UART_LSR_BI)
			sw_uport->lsr_break_flag = SW_UART_LSR_BI;
		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & mask) != mask);

	/* CTS is unsupported by the 2-line UART, so ignore it. */
	if (sw_uport->pdata->io_num == 2)
		return;

	/* Wait up to 500ms for flow control if necessary */
	if (sw_uport->port.flags & UPF_CONS_FLOW) {
		tmout = 500000;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(&sw_uport->port, SW_UART_MSR);

			sw_uport->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & SW_UART_MSR_CTS)
				break;

			udelay(1);
		}
	}
}

static unsigned int sw_uart_tx_empty(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags = 0;
	unsigned int ret = 0;

	spin_lock_irqsave(&sw_uport->port.lock, flags);
	ret = (serial_in(port, SW_UART_USR) & SW_UART_USR_TFE) ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&sw_uport->port.lock, flags);
	return ret;
}

static void sw_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= SW_UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= SW_UART_MCR_DTR;
	if (mctrl & TIOCM_LOOP)
		mcr |= SW_UART_MCR_LOOP;
	sw_uport->mcr &= ~(SW_UART_MCR_RTS|SW_UART_MCR_DTR|SW_UART_MCR_LOOP);
	sw_uport->mcr |= mcr;
	SERIAL_DBG("set mcr %x\n", mcr);
	serial_out(port, sw_uport->mcr, SW_UART_MCR);
}

static unsigned int sw_uart_get_mctrl(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int msr;
	unsigned int ret = 0;

	msr = sw_uart_modem_status(sw_uport);
	if (msr & SW_UART_MSR_DCD)
		ret |= TIOCM_CAR;
	if (msr & SW_UART_MSR_RI)
		ret |= TIOCM_RNG;
	if (msr & SW_UART_MSR_DSR)
		ret |= TIOCM_DSR;
	if (msr & SW_UART_MSR_CTS)
		ret |= TIOCM_CTS;
	SERIAL_DBG("get msr %x\n", msr);
	return ret;
}

static void sw_uart_stop_rx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (sw_uport->ier & SW_UART_IER_RLSI) {
		sw_uport->ier &= ~SW_UART_IER_RLSI;
		SERIAL_DBG("stop rx, ier %x\n", sw_uport->ier);
		sw_uport->port.read_status_mask &= ~SW_UART_LSR_DR;
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_enable_ms(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SW_UART_IER_MSI)) {
		sw_uport->ier |= SW_UART_IER_MSI;
		SERIAL_DBG("en msi, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state == -1)
		sw_uport->lcr |= SW_UART_LCR_SBC;
	else
		sw_uport->lcr &= ~SW_UART_LCR_SBC;
	serial_out(port, sw_uport->lcr, SW_UART_LCR);
	spin_unlock_irqrestore(&port->lock, flags);
}

static int sw_uart_startup(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG("start up ...\n");

	ret = request_irq(port->irq, sw_uart_irq, 0, sw_uport->name, port);
	if (unlikely(ret)) {
		SERIAL_MSG("uart%d cannot get irq %d\n", sw_uport->id, port->irq);
		return ret;
	}

	sw_uport->msr_saved_flags = 0;
	/*
	 * PTIME mode to select the THRE trigger condition:
	 * if PTIME=1(IER[7]), the THRE interrupt will be generated when the
	 * the water level of the TX FIFO is lower than the threshold of the
	 * TX FIFO. and if PTIME=0, the THRE interrupt will be generated when
	 * the TX FIFO is empty.
	 * In addition, when PTIME=1, the THRE bit of the LSR register will not
	 * be set when the THRE interrupt is generated. You must check the
	 * interrupt id of the IIR register to decide whether some data need to
	 * send.
	 */
	sw_uport->ier = SW_UART_IER_RLSI | SW_UART_IER_RDI;
	#ifdef CONFIG_SW_UART_PTIME_MODE
	sw_uport->ier |= SW_UART_IER_PTIME;
	#endif

	return 0;
}

static void sw_uart_shutdown(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG("shut down ...\n");
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	free_irq(port->irq, port);
}

static void sw_uart_flush_buffer(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG("flush buffer...\n");
	sw_uport->fcr = serial_in(port, SW_UART_FCR);
	serial_out(port, sw_uport->fcr|SW_UART_FCR_TXFIFO_RST, SW_UART_FCR);
}

static void sw_uart_set_termios(struct uart_port *port, struct ktermios *termios,
			    struct ktermios *old)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;
	unsigned int baud, quot, lcr = 0, dll, dlh;
	unsigned int lcr_fail = 0;

	SERIAL_DBG("set termios ...\n");
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr |= SW_UART_LCR_WLEN5;
		break;
	case CS6:
		lcr |= SW_UART_LCR_WLEN6;
		break;
	case CS7:
		lcr |= SW_UART_LCR_WLEN7;
		break;
	case CS8:
	default:
		lcr |= SW_UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		lcr |= SW_UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		lcr |= SW_UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		lcr |= SW_UART_LCR_EPAR;

	/* set buadrate */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk / 16);
	sw_uart_check_baudset(port, baud);
	quot = uart_get_divisor(port, baud);
	dll = quot & 0xff;
	dlh = quot >> 8;
	SERIAL_DBG("set baudrate %d, quot %d\n", baud, quot);

	spin_lock_irqsave(&port->lock, flags);
	uart_update_timeout(port, termios->c_cflag, baud);

	/* Update the per-port timeout. */
	port->read_status_mask = SW_UART_LSR_OE | SW_UART_LSR_THRE | SW_UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= SW_UART_LSR_FE | SW_UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= SW_UART_LSR_BI;

	/* Characteres to ignore */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= SW_UART_LSR_PE | SW_UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= SW_UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= SW_UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= SW_UART_LSR_DR;

	/*
	 * if lcr & baud are changed, reset controller to disable transfer
	 */
	if (lcr != sw_uport->lcr || dll != sw_uport->dll || dlh != sw_uport->dlh) {
//		SERIAL_DBG("LCR & BAUD changed, reset controller...\n");
		sw_uart_reset(sw_uport);
	}
	sw_uport->dll = dll;
	sw_uport->dlh = dlh;

	/* flow control */
	sw_uport->mcr &= ~SW_UART_MCR_AFE;
	if (termios->c_cflag & CRTSCTS)
		sw_uport->mcr |= SW_UART_MCR_AFE;
	serial_out(port, sw_uport->mcr, SW_UART_MCR);

	/*
	 * CTS flow control flag and modem status interrupts
	 */
	sw_uport->ier &= ~SW_UART_IER_MSI;
	if (UART_ENABLE_MS(port, termios->c_cflag))
		sw_uport->ier |= SW_UART_IER_MSI;
	serial_out(port, sw_uport->ier, SW_UART_IER);

	sw_uport->fcr = SW_UART_FCR_RXTRG_1_2 | SW_UART_FCR_TXTRG_1_2
			| SW_UART_FCR_FIFO_EN;
	serial_out(port, sw_uport->fcr, SW_UART_FCR);

	sw_uport->lcr = lcr;
	serial_out(port, sw_uport->lcr|SW_UART_LCR_DLAB, SW_UART_LCR);
	if (serial_in(port, SW_UART_LCR) != (sw_uport->lcr|SW_UART_LCR_DLAB)) {
		lcr_fail = 1;
	} else {
		sw_uport->lcr = lcr;
		serial_out(port, sw_uport->dll, SW_UART_DLL);
		serial_out(port, sw_uport->dlh, SW_UART_DLH);
		serial_out(port, sw_uport->lcr, SW_UART_LCR);
		if (serial_in(port, SW_UART_LCR) != sw_uport->lcr) {
			lcr_fail = 2;
		}
	}

	#ifdef CONFIG_SW_UART_FORCE_LCR
	if (lcr_fail) {
		sw_uart_force_lcr(sw_uport, 50);
		serial_in(port, SW_UART_USR);
	}
	#endif
	/* clear rxfifo after set lcr & baud to discard redundant data */
	serial_out(port, sw_uport->fcr|SW_UART_FCR_RXFIFO_RST, SW_UART_FCR);
	port->ops->set_mctrl(port, port->mctrl);

	/* Must save the current config for the resume of console(no tty user). */
	if (sw_is_console_port(port))
		port->cons->cflag = termios->c_cflag;

	spin_unlock_irqrestore(&port->lock, flags);
	/* lately output force lcr information */
	if (lcr_fail == 1) {
		SERIAL_MSG("uart%d write LCR(pre-dlab) failed, lcr %x reg %x\n",
			sw_uport->id, sw_uport->lcr|(u32)SW_UART_LCR_DLAB,
			serial_in(port, SW_UART_LCR));
	} else if (lcr_fail == 2) {
		SERIAL_MSG("uart%d write LCR(post-dlab) failed, lcr %x reg %x\n",
			sw_uport->id, sw_uport->lcr, serial_in(port, SW_UART_LCR));
	}
	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
	SERIAL_DBG("termios lcr 0x%x fcr 0x%x mcr 0x%x dll 0x%x dlh 0x%x\n",
			sw_uport->lcr, sw_uport->fcr, sw_uport->mcr,
			sw_uport->dll, sw_uport->dlh);
}

static const char *sw_uart_type(struct uart_port *port)
{
	return "SUNXI";
}

static int sw_uart_select_gpio_state(struct pinctrl *pctrl, char *name, u32 no)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;
	
	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		SERIAL_MSG("UART%d pinctrl_lookup_state(%s) failed! return %p \n", no, name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		SERIAL_MSG("UART%d pinctrl_select_state(%s) failed! return %d \n", no, name, ret);

	return ret;
}

static int sw_uart_request_gpio(struct sw_uart_port *sw_uport)
{
#ifdef SUNXI_S_UART
	if (sw_uport->id == (SUNXI_UART_NUM - 1)) {
		/* use name s_uart0 to get pinctrl */
		snprintf(sw_uport->name, 16, SUNXI_S_UART_DEV_NAME"%d", 0);
	}
#endif

	sw_uport->pctrl = devm_pinctrl_get(sw_uport->port.dev);

#ifdef SUNXI_S_UART
	if (sw_uport->id == (SUNXI_UART_NUM - 1)) {
		snprintf(sw_uport->name, 16, SUNXI_UART_DEV_NAME"%d", sw_uport->id);
	}
#endif

	if (IS_ERR_OR_NULL(sw_uport->pctrl)) {
		SERIAL_MSG("UART%d devm_pinctrl_get() failed! return %ld\n", sw_uport->id, PTR_ERR(sw_uport->pctrl));
		return -1;
	}

	return sw_uart_select_gpio_state(sw_uport->pctrl, PINCTRL_STATE_DEFAULT, sw_uport->id);
}

static void sw_uart_release_gpio(struct sw_uart_port *sw_uport)
{
	devm_pinctrl_put(sw_uport->pctrl);
	sw_uport->pctrl = NULL;
}

static void sw_uart_release_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG("release port(iounmap & release io)\n");
	/* release memory resource */
	release_mem_region(port->mapbase, SUNXI_UART_MEM_RANGE);
	iounmap(port->membase);
	port->membase = NULL;
	
	/* release io resource */
	sw_uart_release_gpio(sw_uport);
}

static int sw_uart_request_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG("request port(ioremap & request io) %d\n", port->line);
	/* request memory resource */
	if (!request_mem_region(port->mapbase, SUNXI_UART_MEM_RANGE, SUNXI_UART_DEV_NAME)) {
		SERIAL_MSG("uart%d, request mem region failed\n", sw_uport->id);
		return -EBUSY;
	}
	port->membase = ioremap(port->mapbase, SUNXI_UART_MEM_RANGE);
	if (!port->membase) {
		SERIAL_MSG("uart%d, ioremap failed\n", sw_uport->id);
		release_mem_region(port->mapbase, SUNXI_UART_MEM_RANGE);
		return -EBUSY;
	}

	/* request io resource */
	ret = sw_uart_request_gpio(sw_uport);
	if (ret < 0) {
		release_mem_region(port->mapbase, SUNXI_UART_MEM_RANGE);
		return ret;
	}

	return 0;
}

static void sw_uart_config_port(struct uart_port *port, int flags)
{
	int ret;

	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_SUNXI;
		ret = sw_uart_request_port(port);
		if (ret)
			return;
	}
}

static int sw_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_SUNXI))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	return 0;
}

static void sw_uart_pm(struct uart_port *port, unsigned int state,
		      unsigned int oldstate)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG("PM state %d -> %d\n", oldstate, state);

	switch (state) {
	case 0: /* Power up */
		if (sw_uport->mclk->enable_count > 0) {
			SERIAL_MSG("uart%d clk is already enable\n", sw_uport->id);
			break;
		}

		ret = clk_prepare_enable(sw_uport->mclk);
		if (ret) {
			SERIAL_MSG("uart%d release reset failed\n", sw_uport->id);
		}
		break;
	case 3: /* Power down */
		if (sw_uport->mclk->enable_count == 0) {
			SERIAL_MSG("uart%d clk is already disable\n", sw_uport->id);
			break;
		}

		clk_disable_unprepare(sw_uport->mclk);
		break;
	default:
		SERIAL_MSG("uart%d, Unknown PM state %d\n", sw_uport->id, state);
	}
}

static struct uart_ops sw_uart_ops = {
	.tx_empty = sw_uart_tx_empty,
	.set_mctrl = sw_uart_set_mctrl,
	.get_mctrl = sw_uart_get_mctrl,
	.stop_tx = sw_uart_stop_tx,
	.start_tx = sw_uart_start_tx,
	.stop_rx = sw_uart_stop_rx,
	.enable_ms = sw_uart_enable_ms,
	.break_ctl = sw_uart_break_ctl,
	.startup = sw_uart_startup,
	.shutdown = sw_uart_shutdown,
	.flush_buffer = sw_uart_flush_buffer,
	.set_termios = sw_uart_set_termios,
	.type = sw_uart_type,
	.release_port = sw_uart_release_port,
	.request_port = sw_uart_request_port,
	.config_port = sw_uart_config_port,
	.verify_port = sw_uart_verify_port,
	.pm = sw_uart_pm,
};

static int sw_uart_regulator_request(struct sw_uart_port* sw_uport, struct sw_uart_pdata *pdata)
{
	struct regulator *regu = NULL;

	/* Consider "n***" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((pdata->regulator_id[0] == 'n') || (pdata->regulator_id[0] == 0))
		return 0;

	regu = regulator_get(NULL, pdata->regulator_id);
	if (IS_ERR(regu)) {
		SERIAL_MSG("get regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	pdata->regulator = regu;
	return 0;
}

static void sw_uart_regulator_release(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return;

	regulator_put(pdata->regulator);
	pdata->regulator = NULL;
}

static int sw_uart_regulator_enable(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_enable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static int sw_uart_regulator_disable(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_disable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static struct sw_uart_pdata sw_uport_pdata[SUNXI_UART_NUM];
static struct platform_device sw_uport_device[SUNXI_UART_NUM];
static struct sw_uart_port sw_uart_ports[SUNXI_UART_NUM];

static void __init sunxi_uart_device_scan(void)
{
	int i;

	memset(sw_uport_device, 0, sizeof(sw_uport_device));
	memset(sw_uport_pdata, 0, sizeof(sw_uport_pdata));
	memset(sw_uart_ports, 0, sizeof(sw_uart_ports));

	for (i=0; i<SUNXI_UART_NUM; i++) {
		sw_uport_pdata[i].base = SUNXI_UART_MEM_START(i);
		sw_uport_pdata[i].irq  = SUNXI_UART_IRQ(i);
		sw_uport_pdata[i].max_ios = gs_uart_io_num[i];

		sw_uport_device[i].name = SUNXI_UART_DEV_NAME;
		sw_uport_device[i].id   = i;
		sw_uport_device[i].dev.platform_data = &sw_uport_pdata[i];

		sw_uart_ports[i].port.iotype = UPIO_MEM;
		sw_uart_ports[i].port.ops = &sw_uart_ops;
		sw_uart_ports[i].port.fifosize = 64;
		sw_uart_ports[i].port.line = i;
		sw_uart_ports[i].pdata = &sw_uport_pdata[i];
	}

#ifdef SUNXI_S_UART
	do {
		sw_uport_pdata[SUNXI_UART_NUM - 1].base = SUNXI_S_UART_MEM_START;
		sw_uport_pdata[SUNXI_UART_NUM - 1].irq  = SUNXI_S_UART_IRQ;
	} while(0);
#endif

}

static ssize_t sunxi_uart_dev_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct sw_uart_pdata *pdata = (struct sw_uart_pdata *)dev->platform_data;

	return snprintf(buf, PAGE_SIZE,
	   	"id     = %d \n"
	   	"name   = %s \n"
	   	"irq    = %d \n"
	   	"io_num = %d \n"
	   	"port->mapbase = 0x%08x \n"
	   	"port->membase = 0x%p \n"
		"port->iobase  = 0x%08lx \n"
		"pdata->regulator    = 0x%p \n"
		"pdata->regulator_id = %s \n",
		sw_uport->id, sw_uport->name, port->irq, 
		sw_uport->pdata->io_num,
		port->mapbase, port->membase, port->iobase,
		pdata->regulator, pdata->regulator_id);
}
static struct device_attribute sunxi_uart_dev_info_attr =
	__ATTR(dev_info, S_IRUGO, sunxi_uart_dev_info_show, NULL);

static ssize_t sunxi_uart_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, 
	   	"uartclk = %d \n"
	   	"The Uart controller register[Base: 0x%p]: \n"
	   	"[RTX] 0x%02x = 0x%08x, [IER] 0x%02x = 0x%08x, [FCR] 0x%02x = 0x%08x \n"
	   	"[LCR] 0x%02x = 0x%08x, [MCR] 0x%02x = 0x%08x, [LSR] 0x%02x = 0x%08x \n"
	   	"[MSR] 0x%02x = 0x%08x, [SCH] 0x%02x = 0x%08x, [USR] 0x%02x = 0x%08x \n"
	   	"[TFL] 0x%02x = 0x%08x, [RFL] 0x%02x = 0x%08x, [HALT] 0x%02x = 0x%08x \n",
	   	port->uartclk, port->membase,
		SUNXI_UART_RBR, readl(port->membase + SUNXI_UART_RBR),
		SUNXI_UART_IER, readl(port->membase + SUNXI_UART_IER),
		SUNXI_UART_FCR, readl(port->membase + SUNXI_UART_FCR),
		SUNXI_UART_LCR, readl(port->membase + SUNXI_UART_LCR),
		SUNXI_UART_MCR, readl(port->membase + SUNXI_UART_MCR),
		SUNXI_UART_LSR, readl(port->membase + SUNXI_UART_LSR),
		SUNXI_UART_MSR, readl(port->membase + SUNXI_UART_MSR),
		SUNXI_UART_SCH, readl(port->membase + SUNXI_UART_SCH),
		SUNXI_UART_USR, readl(port->membase + SUNXI_UART_USR),
		SUNXI_UART_TFL, readl(port->membase + SUNXI_UART_TFL),
		SUNXI_UART_RFL, readl(port->membase + SUNXI_UART_RFL),
		SUNXI_UART_HALT, readl(port->membase + SUNXI_UART_HALT));
}
static struct device_attribute sunxi_uart_status_attr =
	__ATTR(status, S_IRUGO, sunxi_uart_status_show, NULL);

static ssize_t sunxi_uart_loopback_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int mcr = 0;
	struct uart_port *port = dev_get_drvdata(dev);

	mcr = readl(port->membase + SUNXI_UART_MCR);
	return snprintf(buf, PAGE_SIZE,
	   	"MCR: 0x%08x, Loopback: %d\n", mcr, mcr&SW_UART_MCR_LOOP ? 1 : 0);
}

static ssize_t sunxi_uart_loopback_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int mcr = 0;
	int enable = 0;
	struct uart_port *port = dev_get_drvdata(dev);

	if (!strncmp(buf, "enable", 6))
		enable = 1;
	
	pr_debug("Set loopback: %d \n", enable);

	mcr = readl(port->membase + SUNXI_UART_MCR);
	if (enable)
		writel(mcr|SW_UART_MCR_LOOP, port->membase + SUNXI_UART_MCR);
	else
		writel(mcr&(~SW_UART_MCR_LOOP), port->membase + SUNXI_UART_MCR);
	
	return count;
}
static struct device_attribute sunxi_uart_loopback_attr =
	__ATTR(loopback, S_IRUGO|S_IWUSR, sunxi_uart_loopback_show, sunxi_uart_loopback_store);

static ssize_t sunxi_uart_ctrl_info_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	u32 dl = (u32)sw_uport->dlh << 8 | (u32)sw_uport->dll;

	if (dl == 0)
		dl = 1000;

	return snprintf(buf, PAGE_SIZE, 
		" ier  : 0x%02x\n"
		" lcr  : 0x%02x\n"
		" mcr  : 0x%02x\n"
		" fcr  : 0x%02x\n"
		" dll  : 0x%02x\n"
		" dlh  : 0x%02x\n"
		" last baud : %d (dl = %d)\n\n"
		"TxRx Statistics:\n"
		" tx     : %d\n"
		" rx     : %d\n"
		" parity : %d\n"
		" frame  : %d\n"
		" overrun: %d\n", 
		sw_uport->ier, sw_uport->lcr, sw_uport->mcr,
		sw_uport->fcr, sw_uport->dll, sw_uport->dlh,
		(sw_uport->port.uartclk>>4)/dl, dl,
		sw_uport->port.icount.tx, 
		sw_uport->port.icount.rx,
		sw_uport->port.icount.parity,
		sw_uport->port.icount.frame,
		sw_uport->port.icount.overrun);
}
static struct device_attribute sunxi_uart_ctrl_info_attr =
	__ATTR(ctrl_info, S_IRUGO, sunxi_uart_ctrl_info_show, NULL);

static void sunxi_uart_sysfs(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_uart_dev_info_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_status_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_loopback_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_ctrl_info_attr);
}

#ifdef CONFIG_SERIAL_SUNXI_CONSOLE
static struct uart_port *sw_console_get_port(struct console *co)
{
	struct uart_port *port = NULL;
	struct sw_uart_pdata *pdata;
	int i;

	for (i=0; i<SUNXI_UART_NUM; i++) {
		pdata = sw_uart_ports[i].pdata;
		if ((pdata->used) && (pdata->port_no == co->index)) {
			port = &sw_uart_ports[i].port;
			break;
		}
	}
	return port;
}

static void sw_console_putchar(struct uart_port *port, int c)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	wait_for_xmitr(sw_uport);
	serial_out(port, c, SW_UART_THR);
}

static void sw_console_write(struct console *co, const char *s,
			      unsigned int count)
{
	struct uart_port *port = NULL;
	struct sw_uart_port *sw_uport;
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	BUG_ON(co->index < 0 || co->index >= SUNXI_UART_NUM);

	port = sw_console_get_port(co);
	if (port == NULL)
		return;
	sw_uport = UART_TO_SPORT(port);

	local_irq_save(flags);
	if (port->sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock(&port->lock);
	else
		spin_lock(&port->lock);
	ier = serial_in(port, SW_UART_IER);
	serial_out(port, 0, SW_UART_IER);

	uart_console_write(port, s, count, sw_console_putchar);
	wait_for_xmitr(sw_uport);
	serial_out(port, ier, SW_UART_IER);
	if (locked)
		spin_unlock(&port->lock);
	local_irq_restore(flags);
}

static int __init sw_console_setup(struct console *co, char *options)
{
	struct uart_port *port = NULL;
	struct sw_uart_port *sw_uport;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= SUNXI_UART_NUM || co->index < 0))
		return -ENXIO;

	port = sw_console_get_port(co);
	if (port == NULL)
		return -ENODEV;
	sw_uport = UART_TO_SPORT(port);
	if (!port->iobase && !port->membase)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	SERIAL_MSG("console setup baud %d parity %c bits %d, flow %c\n",
			baud, parity, bits, flow);
	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver sw_uart_driver;
static struct console sw_console = {
	.name = "ttyS",
	.write = sw_console_write,
	.device = uart_console_device,
	.setup = sw_console_setup,
	.flags = CON_PRINTBUFFER | CON_ANYTIME,
	.index = -1,
	.data = &sw_uart_driver,
};

#define SW_CONSOLE	(&sw_console)
#else
#define SW_CONSOLE	NULL
#endif

static struct uart_driver sw_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = SUNXI_UART_DEV_NAME,
	.dev_name = "ttyS",
	.nr = SUNXI_UART_NUM,
	.cons = SW_CONSOLE,
};

static int sw_uart_request_resource(struct sw_uart_port* sw_uport, struct sw_uart_pdata *pdata)
{
	struct uart_port *port = &sw_uport->port;

	SERIAL_DBG("get system resource(clk & IO)\n");

	if (sw_uart_regulator_request(sw_uport, pdata) < 0) {
		SERIAL_MSG("uart%d request regulator failed!\n", sw_uport->id);
		return -ENXIO;
	}
	sw_uart_regulator_enable(pdata);

	sw_uport->mclk = clk_get(port->dev, sw_uport->name);
	if (IS_ERR_OR_NULL(sw_uport->mclk)) {
		SERIAL_MSG("uart%d get mclk failed\n", sw_uport->id);
		return PTR_ERR(sw_uport->mclk);
	}

	#ifdef CONFIG_SW_UART_DUMP_DATA
	sw_uport->dump_buff = (char*)kmalloc(MAX_DUMP_SIZE, GFP_KERNEL);
	if (!sw_uport->dump_buff) {
		SERIAL_MSG("uart%d fail to alloc dump buffer\n", sw_uport->id);
	}
	#endif

	return 0;
}

static int sw_uart_release_resource(struct sw_uart_port* sw_uport, struct sw_uart_pdata *pdata)
{
	SERIAL_DBG("put system resource(clk & IO)\n");

	#ifdef CONFIG_SW_UART_DUMP_DATA
	kfree(sw_uport->dump_buff);
	sw_uport->dump_buff = NULL;
	sw_uport->dump_len = 0;
	#endif

	clk_disable_unprepare(sw_uport->mclk);
	clk_put(sw_uport->mclk);

	sw_uart_regulator_disable(pdata);
	sw_uart_regulator_release(pdata);

	return 0;
}

struct platform_device *sw_uart_get_pdev(int uart_id)
{
	if (sw_uart_ports[uart_id].port.dev)
		return to_platform_device(sw_uart_ports[uart_id].port.dev);
	else
		return NULL;
}
EXPORT_SYMBOL(sw_uart_get_pdev);

static int __devinit sw_uart_probe(struct platform_device *pdev)
{
	struct uart_port *port;
	struct sw_uart_port *sw_uport;
	int ret = -1;

	port = &sw_uart_ports[pdev->id].port;
	port->dev = &pdev->dev;
	sw_uport = UART_TO_SPORT(port);
	sw_uport->id = pdev->id;
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	sw_uport->dll = 0;
	sw_uport->dlh = 0;
	snprintf(sw_uport->name, 16, SUNXI_UART_DEV_NAME"%d", pdev->id);
	pdev->dev.init_name = sw_uport->name;

	SERIAL_DBG("uart.%d probe ... \n", pdev->id);

	/* request system resource and init them */
	ret = sw_uart_request_resource(sw_uport, pdev->dev.platform_data);
	if (unlikely(ret)) {
		SERIAL_MSG("uart%d error to get resource\n", pdev->id);
		return -ENXIO;
	}

	port->uartclk = clk_get_rate(sw_uport->mclk);

	/* bug: clk_get_rate can't get s_uart clk rate in real-time,
		 so set it to actual value */
#ifdef SUNXI_S_UART
#ifdef CONFIG_ARCH_SUN8IW5P1
	if (pdev->id == (SUNXI_UART_NUM - 1))
		port->uartclk = 200000000;
#endif
#endif

	port->type = PORT_SUNXI;
	port->flags = UPF_BOOT_AUTOCONF;
	port->mapbase = sw_uport->pdata->base;
	port->irq = sw_uport->pdata->irq;
	port->line = sw_uport->pdata->port_no;
	platform_set_drvdata(pdev, port);

	SERIAL_DBG("add uart%d port, port_type %d, uartclk %d\n",
			pdev->id, port->type, port->uartclk);
	return uart_add_one_port(&sw_uart_driver, port);
}

static int __devexit sw_uart_remove(struct platform_device *pdev)
{
	struct sw_uart_port *sw_uport = platform_get_drvdata(pdev);

	SERIAL_DBG("release uart%d port\n", sw_uport->id);
	sw_uart_release_resource(sw_uport, pdev->dev.platform_data);
	return 0;
}

/* UART power management code */
#ifdef CONFIG_PM_SLEEP

#define SW_UART_NEED_SUSPEND(port) \
	((sw_is_console_port(port) && (console_suspend_enabled)) \
		|| !sw_is_console_port(port))

static int sw_uart_suspend(struct device *dev)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (port) {
		SERIAL_MSG("uart%d suspend\n", port->line);
		uart_suspend_port(&sw_uart_driver, port);

		if (SW_UART_NEED_SUSPEND(port)) {
			sw_uart_select_gpio_state(sw_uport->pctrl, PINCTRL_STATE_SUSPEND, sw_uport->id);
			sw_uart_regulator_disable(dev->platform_data);
		}
	}

	return 0;
}

static int sw_uart_resume(struct device *dev)
{
#ifdef CONFIG_EVB_PLATFORM
	unsigned long flags = 0;
#endif
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (port) {
		if (SW_UART_NEED_SUSPEND(port)) {
			sw_uart_regulator_enable(dev->platform_data);
			sw_uart_select_gpio_state(sw_uport->pctrl, PINCTRL_STATE_DEFAULT, sw_uport->id);
		}
#ifdef CONFIG_EVB_PLATFORM
		/* It's used only in super-standby mode. 
		  FPGA maybe fall into sw_uart_force_lcr(), so comment it. */
		if (sw_is_console_port(port) && !console_suspend_enabled) {
			spin_lock_irqsave(&port->lock, flags);
			sw_uart_reset(sw_uport);
			serial_out(port, sw_uport->fcr, SW_UART_FCR);
			serial_out(port, sw_uport->mcr, SW_UART_MCR);
			serial_out(port, sw_uport->lcr|SW_UART_LCR_DLAB, SW_UART_LCR);
			serial_out(port, sw_uport->dll, SW_UART_DLL);
			serial_out(port, sw_uport->dlh, SW_UART_DLH);
			serial_out(port, sw_uport->lcr, SW_UART_LCR);
			serial_out(port, sw_uport->ier, SW_UART_IER);
			spin_unlock_irqrestore(&port->lock, flags);
		}
#endif
		uart_resume_port(&sw_uart_driver, port);
		SERIAL_MSG("uart%d resume. DLH: %d, DLL: %d. \n", port->line, sw_uport->dlh, sw_uport->dll);
	}

	return 0;
}

static const struct dev_pm_ops sw_uart_pm_ops = {
	.suspend = sw_uart_suspend,
	.resume = sw_uart_resume,
};
#define SERIAL_SW_PM_OPS	(&sw_uart_pm_ops)

#else /* !CONFIG_PM_SLEEP */

#define SERIAL_SW_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver sw_uport_platform_driver = {
	.probe  = sw_uart_probe,
	.remove = sw_uart_remove,
	.driver = {
		.name  = SUNXI_UART_DEV_NAME,
		.pm    = SERIAL_SW_PM_OPS,
		.owner = THIS_MODULE,
	},
};

static int sw_uart_get_devinfo(void)
{
	u32 i;
	char uart_para[16] = {0};
	struct sw_uart_pdata *pdata = NULL;
	script_item_u val = {0};
	script_item_value_type_e type = 0;

	for (i=0; i<SUNXI_UART_NUM; i++) {
		pdata = &sw_uport_pdata[i];
		sprintf(uart_para, SUNXI_UART_DEV_NAME"%d", i);
		/* get used information */
		type = script_get_item(uart_para, "uart_used", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SERIAL_MSG("get uart%d's usedcfg failed\n", i);
			continue;
		}
		pdata->used = val.val;
		if (pdata->used == 0)
			continue;

		/* get type information */
		type = script_get_item(uart_para, "uart_type", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SERIAL_MSG("get uart%d's type failed\n", i);
			return -1;
		}
		if (val.val > pdata->max_ios) {
			SERIAL_MSG("io type error: (%d > max_io_num %d)\n", val.val, pdata->max_ios);
			return -1;
		}
		pdata->io_num = val.val;

		/* get port information */
		type = script_get_item(uart_para, "uart_port", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SERIAL_MSG("get uart%d's port failed\n", i);
			pdata->port_no = i;
		}
		else
			pdata->port_no = val.val;

		type = script_get_item(uart_para, "uart_regulator", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			SERIAL_MSG("uart%d has no uart_regulator.\n", i);
			continue;
		}
		strncpy(pdata->regulator_id, val.str, 16);

	}

#ifdef SUNXI_S_UART
	do {
		pdata = &sw_uport_pdata[SUNXI_UART_NUM - 1];
		sprintf(uart_para, SUNXI_S_UART_DEV_NAME"%d", 0);
		/* get used information */
		type = script_get_item(uart_para, "s_uart_used", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SERIAL_MSG("get s_uart0 usedcfg failed\n");
			continue;
		}
		if (val.val == 2)
			pdata->used = 1;

		pdata->io_num = gs_uart_io_num[SUNXI_UART_NUM - 1];

	} while(0);
#endif

	return 0;
}

static int __init sunxi_uart_init(void)
{
	int ret;
	u32 i;
	struct sw_uart_pdata *pdata;

	sunxi_uart_device_scan();
	ret = sw_uart_get_devinfo();
	if (unlikely(ret))
		return ret;

	ret = uart_register_driver(&sw_uart_driver);
	if (unlikely(ret)) {
		SERIAL_MSG("driver initializied\n");
		return ret;
	}

	for (i=0; i<SUNXI_UART_NUM; i++) {
		pdata = &sw_uport_pdata[i];
		if (!pdata->used)
			continue;
		platform_device_register(&sw_uport_device[i]);
		sunxi_uart_sysfs(&sw_uport_device[i]);
	}

	return platform_driver_register(&sw_uport_platform_driver);
}

static void __exit sunxi_uart_exit(void)
{
	SERIAL_MSG("driver exit\n");
#ifdef CONFIG_SERIAL_SUNXI_CONSOLE
	unregister_console(&sw_console);
#endif
	platform_driver_unregister(&sw_uport_platform_driver);
	uart_unregister_driver(&sw_uart_driver);
}

module_init(sunxi_uart_init);
module_exit(sunxi_uart_exit);

MODULE_AUTHOR("Aaron<leafy.myeh@allwinnertech.com>");
MODULE_DESCRIPTION("Driver for Allwinner UART controller");
MODULE_LICENSE("GPL");
