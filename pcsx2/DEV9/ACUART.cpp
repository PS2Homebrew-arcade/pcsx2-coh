#include "ACUART.h"
#include "common/Console.h"

// 16550 UART register emulation for Namco System 246 arcade I/O
// Ref: ps2sdk iop/arcade/acuart/src/uart.c
// Register map (16-bit access, 2-byte stride from base 0x12418000):
//   +0x00  THR/RBR/DLL  (TX/RX data or divisor latch low when DLAB=1)
//   +0x02  IER/DLH      (interrupt enable or divisor latch high when DLAB=1)
//   +0x04  IIR/FCR      (interrupt ID read / FIFO control write)
//   +0x06  LCR          (line control, bit 7 = DLAB)
//   +0x08  MCR          (modem control)
//   +0x0A  LSR          (line status)
//   +0x0C  MSR          (modem status)
//   +0x0E  SCR          (scratch)

static u16 uart_ier = 0;
static u16 uart_lcr = 0;
static u16 uart_mcr = 0;
static u16 uart_scr = 0;
static u16 uart_dll = 0;
static u16 uart_dlh = 0;
static u16 uart_fcr_shadow = 0;

u16 ACUART::Read16(u32 addr) {
	const u32 reg = addr & 0xFFF;
	switch (reg) {
	case 0x000: // RBR or DLL
		if (uart_lcr & 0x80)
			return uart_dll;
		return 0; // no RX data
	case 0x002: // IER or DLH
		if (uart_lcr & 0x80)
			return uart_dlh;
		return uart_ier;
	case 0x004: // IIR (read-only)
		return 0x01; // no interrupt pending, 16550 mode
	case 0x006: // LCR
		return uart_lcr;
	case 0x008: // MCR
		return uart_mcr;
	case 0x00A: // LSR
		// bit 5 = THRE (TX holding register empty)
		// bit 6 = TEMT (TX shift register empty)
		// both set = transmitter idle, ready to accept data
		return 0x60;
	case 0x00C: // MSR
		return 0;
	case 0x00E: // SCR
		return uart_scr;
	default:
		return 0;
	}
}

void ACUART::Write16(u32 addr, u16 val) {
	const u32 reg = addr & 0xFFF;
	switch (reg) {
	case 0x000: // THR or DLL
		if (uart_lcr & 0x80)
			uart_dll = val;
		// else: TX data — silently consumed
		break;
	case 0x002: // IER or DLH — set to 0 on module stop, bits toggled during xmit (acUartModuleStop, uart_xmit)
		if (uart_lcr & 0x80)
			uart_dlh = val;
		else
			uart_ier = val;
		break;
	case 0x004: // FCR (write-only) — val=7 on module stop: enable FIFO + reset RX/TX (acUartModuleStop)
		uart_fcr_shadow = val & 0xC9; // preserve trigger level + enable bits
		break;
	case 0x006: // LCR
		uart_lcr = val;
		break;
	case 0x008: // MCR
		uart_mcr = val;
		break;
	case 0x00E: // SCR
		uart_scr = val;
		break;
	default:
		break;
	}
}
