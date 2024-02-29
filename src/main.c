/**
 * @file main.c
 * @brief Exmaple application to demonstrate Hardware SPI API
 * @version 0.1
 * @date 2022-07-05
 * 
 * @copyright Copyright (c) 2022 Waybyte Solutions
 * 
 */

#include <stdio.h>
#include <unistd.h>

#include <lib.h>
#include <ril.h>
#include <os_api.h>
#include <hw/gpio.h>
#include <hw/spi.h>

#ifndef SOC_RDA8910
#define STDIO_PORT "/dev/ttyS0"
#else
#define STDIO_PORT "/dev/ttyUSB0"
#endif

#ifdef PLATFORM_LOGICROM_SPARK
// #define USE_SPI_HW_CHIPSELECT
#ifdef USE_SPI_HW_CHIPSELECT
#else
#define CS_GPIO	GPIO_23
#endif
#define SPI_HW_PORT SPI_PORT_1
#else
#define CS_GPIO	GPIO_0
#define SPI_HW_PORT SPI_PORT_0
#endif

#define SPIF_ENABLE_RESET 0x66
#define SPIF_RESET_DEVICE 0x99
#define SPIF_READ_JEDECID 0x9F

static const struct _spif_vendor_t {
	uint8_t id;
	const char *vendor;
} spif_vlist[] = {
	{ 0x1F, "Atmel" },   { 0xC8, "GigaDevice" }, { 0x2C, "Micron" },
	{ 0xBF, "SST" },     { 0xC2, "Macronix" },   { 0xEF, "Winbond" },
	{ 0xDA, "Winbond" }, { 0x20, "XMC"},
};

/**
 * URC Handler
 * @param param1	URC Code
 * @param param2	URC Parameter
 */
static void urc_callback(unsigned int param1, unsigned int param2)
{
	switch (param1) {
	case URC_SYS_INIT_STATE_IND:
		if (param2 == SYS_STATE_SMSOK) {
			/* Ready for SMS */
		}
		break;
	case URC_SIM_CARD_STATE_IND:
		switch (param2) {
		case SIM_STAT_NOT_INSERTED:
			debug(DBG_OFF, "SYSTEM: SIM card not inserted!\n");
			break;
		case SIM_STAT_READY:
			debug(DBG_INFO, "SYSTEM: SIM card Ready!\n");
			break;
		case SIM_STAT_PIN_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PIN required!\n");
			break;
		case SIM_STAT_PUK_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PUK required!\n");
			break;
		case SIM_STAT_NOT_READY:
			debug(DBG_OFF, "SYSTEM: SIM card not recognized!\n");
			break;
		default:
			debug(DBG_OFF, "SYSTEM: SIM ERROR: %d\n", param2);
		}
		break;
	case URC_GSM_NW_STATE_IND:
		debug(DBG_OFF, "SYSTEM: GSM NW State: %d\n", param2);
		break;
	case URC_GPRS_NW_STATE_IND:
		break;
	case URC_CFUN_STATE_IND:
		break;
	case URC_COMING_CALL_IND:
		debug(DBG_OFF, "Incoming voice call from: %s\n", ((struct ril_callinfo_t *)param2)->number);
		/* Take action here, Answer/Hang-up */
		break;
	case URC_CALL_STATE_IND:
		switch (param2) {
		case CALL_STATE_BUSY:
			debug(DBG_OFF, "The number you dialed is busy now\n");
			break;
		case CALL_STATE_NO_ANSWER:
			debug(DBG_OFF, "The number you dialed has no answer\n");
			break;
		case CALL_STATE_NO_CARRIER:
			debug(DBG_OFF, "The number you dialed cannot reach\n");
			break;
		case CALL_STATE_NO_DIALTONE:
			debug(DBG_OFF, "No Dial tone\n");
			break;
		default:
			break;
		}
		break;
	case URC_NEW_SMS_IND:
		debug(DBG_OFF, "SMS: New SMS (%d)\n", param2);
		/* Handle New SMS */
		break;
	case URC_MODULE_VOLTAGE_IND:
		debug(DBG_INFO, "VBatt Voltage: %d\n", param2);
		break;
	case URC_ALARM_RING_IND:
		break;
	case URC_FILE_DOWNLOAD_STATUS:
		break;
	case URC_FOTA_STARTED:
		break;
	case URC_FOTA_FINISHED:
		break;
	case URC_FOTA_FAILED:
		break;
	case URC_STKPCI_RSP_IND:
		break;
	default:
		break;
	}
}

/**
 * Application main entry point
 */
int main(int argc, char *argv[])
{
	int ret, count, i;
#ifndef USE_SPI_HW_CHIPSELECT
	int cs_handle = 0;
#endif
	uint8_t buf[4] = { 0 };

	/*
	 * Initialize library and Setup STDIO
	 */
	logicrom_init(STDIO_PORT, urc_callback);

	printf("System Ready\n");

	/* SPI Test */
	do {
		ret = spi_hw_init(SPI_HW_PORT, FALSE, 10000, SPI_MODE0, SPI_CSPOL_LOW);
		printf("SPI Init: %d\n\n", ret);
		if (ret)
			break;
		
		/* Following is an example to read device ID information from SPI Flash memory */
#ifdef USE_SPI_HW_CHIPSELECT
		/**
		 * RDA8910 SPI Hardware chipselect line is not a GPIO but
		 * can be controlled manually
		 */
		spi_hw_cscontrol(SPI_HW_PORT, 0);
#else
		cs_handle = gpio_request(CS_GPIO, GPIO_FLAG_OUTPUT | GPIO_FLAG_DEFHIGH);
		printf("GPIO request: %x\n", cs_handle);
		if (cs_handle == 0)
			break;
		
		gpio_write(cs_handle, 0);
#endif
		buf[0] = SPIF_ENABLE_RESET;
		spi_hw_transfer(SPI_HW_PORT, buf, NULL, 1);
#ifdef USE_SPI_HW_CHIPSELECT
		spi_hw_cscontrol(SPI_HW_PORT, 1);
#else
		gpio_write(cs_handle, 1);
#endif

		/* Send Device reset */
#ifdef USE_SPI_HW_CHIPSELECT
		spi_hw_cscontrol(SPI_HW_PORT, 0);
#else
		gpio_write(cs_handle, 0);
#endif
		buf[0] = SPIF_RESET_DEVICE;
		spi_hw_transfer(SPI_HW_PORT, buf, NULL, 1);
#ifdef USE_SPI_HW_CHIPSELECT
		spi_hw_cscontrol(SPI_HW_PORT, 1);
#else
		gpio_write(cs_handle, 1);
#endif
		/* Wait for device ready */
		os_task_sleep(100);
		
		/* Read JEDEC ID */
#ifdef USE_SPI_HW_CHIPSELECT
		spi_hw_cscontrol(SPI_HW_PORT, 0);
#else
		gpio_write(cs_handle, 0);
#endif
		buf[0] = SPIF_READ_JEDECID;
		spi_hw_transfer(SPI_HW_PORT, buf, NULL, 1);
		spi_hw_transfer(SPI_HW_PORT, NULL, buf, 3);
#ifdef USE_SPI_HW_CHIPSELECT
		spi_hw_cscontrol(SPI_HW_PORT, 1);
#else
		gpio_write(cs_handle, 1);
#endif
		count = sizeof(spif_vlist) / sizeof (*spif_vlist);
		for (i = 0; i < count; i++) {
			if (spif_vlist[i].id == buf[0])
				break;
		}

		/* print device id details */
		printf("SPI Flash ID: %s[%02X%02X%02X]\n\n", i == count ? "Unknown" : spif_vlist[i].vendor, buf[0], buf[1], buf[2]);
	} while (0);

	spi_hw_free(SPI_HW_PORT);
#ifndef USE_SPI_HW_CHIPSELECT
	if (cs_handle)
		gpio_free(cs_handle);
#endif

	while (1) {
		/* Main task */
		sleep(1);
	}
}
