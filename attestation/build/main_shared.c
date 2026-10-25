#include <string.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "aes.h"
#include "clock.h"
#include "uart.h"

#define TRIG_PORT GPIOA
#define TRIG_PIN  GPIO14
#define UART_BAUD 115200

#define STM32_UID_BASE 0x1FFF7A10UL
#define UID_LEN        12

#define KEY_OBFUSCATION 0xa5
#define KB(c) ((uint8_t)((c) ^ KEY_OBFUSCATION))
static uint8_t aes_key[AES_BLOCK_SIZE] = {
#if defined(AES_KEY_FLAG)
	KB('E'), KB('C'), KB('S'), KB('C'), KB('{'), KB('g'), KB('l'), KB('1'),
	KB('t'), KB('c'), KB('h'), KB('-'), KB('A'), KB('E'), KB('S'), KB('}'),
#else /* AES_KEY_TEST (default) */
	KB('T'), KB('E'), KB('S'), KB('T'), KB('T'), KB('E'), KB('S'), KB('T'),
	KB('T'), KB('E'), KB('S'), KB('T'), KB('T'), KB('E'), KB('S'), KB('T'),
#endif
};
#undef KB

static uint8_t plaintext[AES_BLOCK_SIZE];
static aes128_ctx ctx;

static void print_hex_line(const char *label, const uint8_t *data, int len)
{
	static const char hex[] = "0123456789ABCDEF";
	char line[24 + 2 * AES_BLOCK_SIZE + 2];
	int pos = 0;

	while (*label) {
		line[pos++] = *label++;
	}
	for (int i = 0; i < len; i++) {
		line[pos++] = hex[data[i] >> 4];
		line[pos++] = hex[data[i] & 0x0f];
	}
	line[pos++] = '\r';
	line[pos++] = '\n';

	uart_write(line, pos);
}

__ramfunc void aes_last_round_trigger(void)
{
	GPIO_BSRR(TRIG_PORT) = TRIG_PIN; // trigger high
	for (volatile int i = 0; i < 200; i++) {
		__asm__ volatile("nop");
	}
	GPIO_BSRR(TRIG_PORT) = (TRIG_PIN << 16); //trigger low
}

static void do_encrypt(void)
{
	uint8_t state[AES_BLOCK_SIZE];

	memcpy(state, plaintext, sizeof(state));

	print_hex_line("Attesting identity: ", plaintext, UID_LEN);

	cm_disable_interrupts();
	aes128_encrypt_block(&ctx, state);
	cm_enable_interrupts();

	delay_us(1000);

	print_hex_line("Attestation code: ", state, AES_BLOCK_SIZE);
}

int main(void)
{
	clock_init();
	systick_init();

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_clear(TRIG_PORT, TRIG_PIN);
	gpio_mode_setup(TRIG_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TRIG_PIN);
	gpio_set_output_options(TRIG_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, TRIG_PIN);
	gpio_clear(TRIG_PORT, TRIG_PIN);

	uart_init(UART_BAUD);

	const volatile uint8_t *uid = (const volatile uint8_t *)STM32_UID_BASE;
	for (int i = 0; i < UID_LEN; i++) {
		plaintext[i] = uid[i];
	}
	for (int i = UID_LEN; i < AES_BLOCK_SIZE; i++) {
		plaintext[i] = 0x00;
	}

	for (unsigned i = 0; i < sizeof(aes_key); i++) {
		aes_key[i] ^= KEY_OBFUSCATION;
	}

	aes128_init(&ctx, aes_key);
	memset(aes_key, 0, sizeof(aes_key));

	do_encrypt();

	for (;;) {
	}
}
