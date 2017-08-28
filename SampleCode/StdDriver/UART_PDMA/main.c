/****************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Demonstrate UART transmit and receive function with PDMA
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

#define PLL_CLOCK   192000000

#define ENABLE_PDMA_INTERRUPT 1
#define PDMA_TEST_LENGTH 100

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t g_u8Tx_Buffer[PDMA_TEST_LENGTH];
static uint8_t g_u8Rx_Buffer[PDMA_TEST_LENGTH];

volatile uint32_t u32IsTestOver = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void);
void PDMA_IRQHandler(void);
void UART_PDMATest(void);


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Enable HXT clock (external XTAL 12MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(192000000);
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2); // PCLK divider set 2

    /* Enable IP clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk; // UART0 Clock Enable
    CLK->APBCLK0 |= CLK_APBCLK0_UART1CKEN_Msk; // UART1 Clock Enable
    CLK->AHBCLK  |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable

    /* Select IP clock source */
    /* Select UART0 clock source is HXT */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART0SEL_Msk) | (0x0 << CLK_CLKSEL1_UART0SEL_Pos);
    /* Select UART1 clock source is HXT */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART1SEL_Msk) | (0x0 << CLK_CLKSEL1_UART1SEL_Pos);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CycylesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD2MFP_Msk | SYS_GPD_MFPL_PD3MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD2MFP_UART0_RXD | SYS_GPD_MFPL_PD3MFP_UART0_TXD);

    /* Set PA multi-function pins for UART1 TXD, RXD, CTS and RTS */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk |
                       SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk);
    SYS->GPA_MFPL |= (0x8 << SYS_GPA_MFPL_PA0MFP_Pos) | (0x8 << SYS_GPA_MFPL_PA1MFP_Pos) |
                     (0x8 << SYS_GPA_MFPL_PA2MFP_Pos) | (0x8 << SYS_GPA_MFPL_PA3MFP_Pos);


    /* Lock protected registers */
    SYS_LockReg();

}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    UART_Open(UART0, 115200);
}

void UART1_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    UART_Open(UART1, 115200);
}

void PDMA_Init(void)
{
    /* Open PDMA Channel */
    PDMA_Open(1 << 0); // Channel 0 for UART1 TX
    PDMA_Open(1 << 1); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(0, PDMA_UART1_TX, 0, 0);
    PDMA_SetTransferMode(1, PDMA_UART1_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(0, PDMA_WIDTH_8, PDMA_TEST_LENGTH);
    PDMA_SetTransferCnt(1, PDMA_WIDTH_8, PDMA_TEST_LENGTH);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(0, ((uint32_t) (&g_u8Tx_Buffer[0])), PDMA_SAR_INC, UART1_BASE, PDMA_DAR_FIX);
    PDMA_SetTransferAddr(1, UART1_BASE, PDMA_SAR_FIX, ((uint32_t) (&g_u8Rx_Buffer[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(0, PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(1, PDMA_REQ_SINGLE, 0);
    //Set timeout
    //PDMA_SetTimeOut(0, 0, 0x5555);
    //PDMA_SetTimeOut(1, 0, 0x5555);

#ifdef ENABLE_PDMA_INTERRUPT
    PDMA_EnableInt(0, 0);
    PDMA_EnableInt(1, 0);
    NVIC_EnableIRQ(PDMA_IRQn);
    u32IsTestOver = 0;
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int main(void)
{
    /* Init System, IP clock and multi-function I/O */
    SYS_Init();
    /* Init UART0 for printf */
    UART0_Init();

    /* Init UART1 */
    UART1_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\n\nCPU @ %dHz\n", SystemCoreClock);

    UART_PDMATest();

    while(1);
}

/**
 * @brief       DMA IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The DMA default IRQ, declared in startup_M480.s.
 */
void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS();

    if (status & 0x1) { /* abort */
        printf("target abort interrupt !!\n");
        if (PDMA_GET_ABORT_STS() & 0x4)
            u32IsTestOver = 2;
        PDMA_CLR_ABORT_FLAG(PDMA_GET_ABORT_STS());
    } else if (status & 0x2) { /* done */
        if ( (PDMA_GET_TD_STS() & (1 << 0)) && (PDMA_GET_TD_STS() & (1 << 1)) ) {
            u32IsTestOver = 1;
            PDMA_CLR_TD_FLAG(PDMA_GET_TD_STS());
        }
    } else if (status & 0x300) { /* channel 2 timeout */
        printf("timeout interrupt !!\n");
        u32IsTestOver = 3;
        PDMA_CLR_TMOUT_FLAG(0);
        PDMA_CLR_TMOUT_FLAG(1);
    } else
        printf("unknown interrupt !!\n");
}


/*---------------------------------------------------------------------------------------------------------*/
/*  UART PDMA Test                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void UART_PDMATest()
{
    uint32_t i;

    printf("+-----------------------------------------------------------+\n");
    printf("|  UART PDMA Test                                           |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|  Description :                                            |\n");
    printf("|    The sample code will demo UART1 PDMA function.         |\n");
    printf("|    Please connect UART1_TX and UART1_RX pin.              |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("Please press any key to start test. \n\n");

    getchar();

    /*
        Using UAR1 external loop back.
        This code will send data from UART1_TX and receive data from UART1_RX.
    */

    for (i=0; i<PDMA_TEST_LENGTH; i++) {
        g_u8Tx_Buffer[i] = i;
        g_u8Rx_Buffer[i] = 0xff;
    }

    while(1) {
        PDMA_Init();

        UART1->INTEN |= UART_INTEN_TXPDMAEN_Msk | UART_INTEN_RXPDMAEN_Msk;

#ifdef ENABLE_PDMA_INTERRUPT
        while(u32IsTestOver == 0);

        if (u32IsTestOver == 1)
            printf("test done...\n");
        else if (u32IsTestOver == 2)
            printf("target abort...\n");
        else if (u32IsTestOver == 3)
            printf("timeout...\n");
#else
        while( (!(PDMA_GET_TD_STS() & (1 << 0))) || (!(PDMA_GET_TD_STS() & (1 << 1))) );

        PDMA_CLR_TD_FLAG(PDMA_TDSTS_TDIF0_Msk|PDMA_TDSTS_TDIF1_Msk);
#endif

        UART1->INTEN &= ~UART_INTEN_TXPDMAEN_Msk;
        UART1->INTEN &= ~UART_INTEN_RXPDMAEN_Msk;

        for (i=0; i<PDMA_TEST_LENGTH; i++) {
            if(g_u8Rx_Buffer[i] != i) {
                printf("\n Receive Data Compare Error !!");
                while(1);
            }

            g_u8Rx_Buffer[i] = 0xff;
        }

        printf("\nUART PDMA test Pass.\n");
    }

}


