/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "virtual_com.h"
#include "imrxt_ba_monitor.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

dev devInfo = {
	.familyID = 0x00005254, // 'R' 'T'
	.extID = 0x31303532,    // '1' '0' '5' '2'
	.pageSize = FLASH_PAGE_SIZE,
	.totalPage = 0x7DE00, //
	.sectorSize = SECTOR_SIZE,
	.bootloaderAddr = BOOTLOADER_START_ADDRESS,
	.appAddr = APP_START_ADDRESS,
	.userAddr = 0x20021FC0,
	.status = 0x00
};	
 
extern void JumpToApp(uint32_t address);

void writeStatus(uint32_t magic)
{
	  status_t status;
		status = flexspi_nor_flash_erase_sector(BOOTLOADER_FLEXSPI, BOOTLOADER_DATA_AREA * SECTOR_SIZE);
    if (status != kStatus_Success)
    {
       //PRINTF("Erase sector failure !\r\n");
       return ;
    }
		
		devInfo.status = magic;
		
		status = flexspi_nor_flash_buffer_program(BOOTLOADER_FLEXSPI, BOOTLOADER_DATA_AREA * SECTOR_SIZE,
                                            (void *)&devInfo, 40);
    if (status != kStatus_Success)
    {
        //PRINTF("Page program failure !\r\n");
        return ;
    }
}

void call_application(uint32_t address)
{
	__disable_irq();
	
	uint32_t vector = address + 0x2000; 

	
	//write_vtor
	unsigned long *pVTOR = (unsigned long*)0xE000ED08;
  *pVTOR = vector;
	
	/* Rebase the Stack Pointer */
  __set_MSP(*(uint32_t *) vector);
	
	//SystemInit();
	
	__enable_irq();
	
	 /* Load the Reset Handler address of the application */
  uint32_t  app_start_address = vector + 0x4b5;  // jump to reset handle address

  /* Jump to application Reset Handler in the application */
  JumpToApp(app_start_address);
	
}

void check_application()
{
	
	 if(BOOT_STATUS_MAGIC == BOOT_STATUS_DATA)
	 {
		 writeStatus(0x00);
		 //usb_echo("BOOTLOADER MODE\r\n");
		 return;
	 }else{
		 writeStatus(BOOT_STATUS_MAGIC);
		 //usb_echo("APP MODE\r\n");
		 call_application(APP_START_ADDRESS);
	 }
	 
	 

	 /*writeStatus(BOOT_STATUS_MAGIC);
	 PRINTF("IS Mw !\r\n");
	 for(uint32_t i = 0; i < 4096; i++)
		 for(uint32_t j = 0; j < 102400; j++);
	 
	PRINTF("IS Masd !\r\n");
	writeStatus(0x00);
	 
	call_application(APP_START_ADDRESS);*/
	
}

int main(void)
{
		BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    SCB_DisableDCache();
	  flexspi_nor_flash_init(BOOTLOADER_FLEXSPI);
	 // flexspi_nor_enable_quad_mode(BOOTLOADER_FLEXSPI);
	 
	  check_application();

	  vcom_cdc_init();
	   
	  imrxt_ba_monitor_init(IMRXT_BA_INTERFACE_USBCDC);
  
    while (1)
    {
			imrxt_ba_monitor_run();
    }
}
