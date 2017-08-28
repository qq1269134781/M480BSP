/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    This sample shows how to setup the secure key and how to perform secure key comparison.
 *
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>

#include "NuMicro.h"


#define KPMAX_VAL      3               /* KPMAX setting on setup security key (1~15) */
#define KEMAX_VAL      7               /* KEMAX setting on setup security key (1~31) */

uint32_t  good_key[3] = { 0xe29c0f71, 0x8af051ce, 0xae1f8392 };      /* Assumed correct key in this demo program. */
uint32_t  bad_key[3] =  { 0x73422111, 0xac45663a, 0xf46ac321 };      /* Assumed wrong key in this demo program. */


void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT,CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);

    /* Set both PCLK0 and PCLK1 as HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2;

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART module clock source as HXT and UART module clock divider as 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART0 multi-function pins, RXD(PD.2) and TXD(PD.3) */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD2MFP_Msk | SYS_GPD_MFPL_PD3MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD2MFP_UART0_RXD | SYS_GPD_MFPL_PD3MFP_UART0_TXD);

    /* Lock protected registers */
    SYS_LockReg();
}

void UART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
}

void dump_key_status()
{
    printf("KPKEYSTS: 0x%x\n    ", FMC->KPKEYSTS);    /* FMC_KEYSTS register value */
    printf("%s ", (FMC->KPKEYSTS & FMC_KPKEYSTS_KEYLOCK_Msk) ? "[LOCK]" : "[UNLOCK]");  /* KEYLOCK(FMC_KEY_STS[1] */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_KEYBUSY_Msk)
        printf("[BUSY] ");                            /* KEYBUSY(FMC_KEYSTS[0]) */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_KEYMATCH_Msk)
        printf("[KEYMATCH] ");                        /* KEYMATCH(FMC_KEYSTS[2]) */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_FORBID_Msk)
        printf("[FORBID] ");                          /* FORBID(FMC_KEYSTS[3]) */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_KEYFLAG_Msk)
        printf("[KEY LOCK] ");                        /* KEYFLAG(FMC_KEYSTS[4]) */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_CFGFLAG_Msk)
        printf("[CONFIG LOCK] ");                     /* CFGFLAG(FMC_KEYSTS[5]) */
    if (FMC->KPKEYSTS & FMC_KPKEYSTS_SPFLAG_Msk)
        printf("[SPROM LOCK] \n");                    /* SPFLAG(FMC_KEYSTS[6]) */

    printf("KPCNT: 0x%x, KPMAX: 0x%x\n", (FMC->KPCNT & FMC_KPCNT_KPCNT_Msk) >> FMC_KPCNT_KPCNT_Pos,    /* KPCNT(FMC_KPCNT[3:0])  */
           (FMC->KPCNT & FMC_KPCNT_KPMAX_Msk) >> FMC_KPCNT_KPMAX_Pos);   /* KPMAX(FMC_KPCNT[11:8]) */
    printf("KPKEYCNT: 0x%x, KEMAX: 0x%x\n", (FMC->KPKEYCNT & FMC_KPKEYCNT_KPKECNT_Msk) >> FMC_KPKEYCNT_KPKECNT_Pos,    /* KPKEYCNT(FMC_KPKEYCNT[5:0])  */
           (FMC->KPKEYCNT & FMC_KPKEYCNT_KPKEMAX_Msk) >> FMC_KPKEYCNT_KPKEMAX_Pos);   /* KEMAX(FMC_KECNT[13:8]) */
    printf("\n\nPress any key to continue...\n");     /* Wait user press any key on UART0 debug console */
    getchar();                                        /* block on getting any one character from UART0 */
}

int main()
{
    SYS_Init();                        /* Init System, IP clock and multi-function I/O */

    UART0_Init();                      /* Initialize UART0 */

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("+-----------------------------------------+\n");
    printf("|   M480 FMC Secure Key Sample Demo       |\n");
    printf("+-----------------------------------------+\n");

    SYS_UnlockReg();                   /* Unlock protected registers */

    FMC_Open();                        /* Enable FMC ISP function */

    /* Setup a new key */
    if (FMC_SKey_Setup(good_key, KPMAX_VAL, KEMAX_VAL, 0, 0) < 0) {
        printf("Failed to setup key!\n");   /* error message */
        while (1);                     /* Failed to setup security key. Program aborted. */
    }

    printf("The security key status after key setup:\n");
    dump_key_status();                 /* Dump FMC security key status. */

    FMC_SKey_Compare(bad_key);         /* Enter a wrong key for key comparison. */
    printf("The security key status after enter a wrong key:\n");
    dump_key_status();                 /* Dump FMC security key status. */

    FMC_SKey_Compare(bad_key);         /* Enter a wrong key for key comparison. */
    printf("The security key status after enter a wrong key second time:\n");
    dump_key_status();                 /* Dump FMC security key status. */

    FMC_SKey_Compare(good_key);        /* Enter the right key for key comparison. */
    printf("The security key status after enter a good key.\n");
    dump_key_status();                 /* Dump FMC security key status. */

    printf("Erase KPROM key.\n");
    FMC_Erase(FMC_KPROM_BASE);

    printf("Test done.\n");
    while (1);
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
