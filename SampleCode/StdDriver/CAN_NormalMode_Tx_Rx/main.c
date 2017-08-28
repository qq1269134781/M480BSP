/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Demonstrate CAN bus transmit and receive a message with normal
 *           mode by connecting CAN 0 and CAN1 to the same CAN bus
 *
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
STR_CANMSG_T rrMsg;

void CAN_ShowMsg(STR_CANMSG_T* Msg);

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle CAN interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CAN_MsgInterrupt(CAN_T *tCAN, uint32_t u32IIDR)
{
    if(u32IIDR==1) {
        printf("Msg-0 INT and Callback\n");
        CAN_Receive(tCAN, 0,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==5+1) {
        printf("Msg-5 INT and Callback \n");
        CAN_Receive(tCAN, 5,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==31+1) {
        printf("Msg-31 INT and Callback \n");
        CAN_Receive(tCAN, 31,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
}


/**
  * @brief  CAN0_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN0_IRQHandler(void)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN0->IIDR;

    if(u8IIDRstatus == 0x00008000) {      /* Check Status Interrupt Flag (Error status Int and Status change Int) */
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_RXOK_Msk) {
            CAN0->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_TXOK_Msk) {
            CAN0->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_EWARN_Msk) {
            printf("EWARN INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_BOFF_Msk) {
            printf("BOFF INT\n") ;
        }
    } else if (u8IIDRstatus!=0) {
        printf("=> Interrupt Pointer = %d\n",CAN0->IIDR -1);

        CAN_MsgInterrupt(CAN0, u8IIDRstatus);

        CAN_CLR_INT_PENDING_BIT(CAN0, ((CAN0->IIDR) -1));      /* Clear Interrupt Pending */

    } else if(CAN0->WU_STATUS == 1) {
        printf("Wake up\n");

        CAN0->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}

/**
  * @brief  CAN1_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN1_IRQHandler(void)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN1->IIDR;

    if(u8IIDRstatus == 0x00008000) {      /* Check Status Interrupt Flag (Error status Int and Status change Int) */
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(CAN1->STATUS & CAN_STATUS_RXOK_Msk) {
            CAN1->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(CAN1->STATUS & CAN_STATUS_TXOK_Msk) {
            CAN1->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(CAN1->STATUS & CAN_STATUS_EWARN_Msk) {
            printf("EWARN INT\n") ;
        }

        if(CAN1->STATUS & CAN_STATUS_BOFF_Msk) {
            printf("BOFF INT\n") ;
        }
    } else if (u8IIDRstatus!=0) {
        //printf("=> Interrupt Pointer = %d\n",CAN1->IIDR -1);
        printf("=> Interrupt Pointer = %d\n",u8IIDRstatus -1);

        CAN_MsgInterrupt(CAN1, u8IIDRstatus);

        //CAN_CLR_INT_PENDING_BIT(CAN1, ((CAN1->IIDR) -1));      /* Clear Interrupt Pending */
        CAN_CLR_INT_PENDING_BIT(CAN1, (u8IIDRstatus -1));      /* Clear Interrupt Pending */

    } else if(CAN1->WU_STATUS == 1) {
        printf("Wake up\n");

        CAN1->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}


void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    // TODO: Configure system clock

    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    // HCLK select external 4~24 MHz high speed crystal clock
    //CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLKSEL_Msk;
    //CLK->CLKSEL0 |= CLK_CLKSEL0_HCLKSEL_HXT;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(192000000);
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2); // PCLK divider set 2

    // Enable IP clock
    CLK->APBCLK0 |= CLK_APBCLK0_TMR0CKEN_Msk;
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk; // UART0 Clock Enable

    /* Select IP clock source */
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_UART0SEL_Msk;
    CLK->CLKSEL1 |= (0x0 << CLK_CLKSEL1_UART0SEL_Pos);// Clock source from external 12 MHz or 32 KHz crystal clock

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD2MFP_Msk | SYS_GPD_MFPL_PD3MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD2MFP_UART0_RXD | SYS_GPD_MFPL_PD3MFP_UART0_TXD);

    /* Set PA multi-function pins for CAN0 RXD(PA.4) and TXD(PA.5) */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk)) |
                    (SYS_GPA_MFPL_PA4MFP_CAN0_RXD | SYS_GPA_MFPL_PA5MFP_CAN0_TXD);

    /* Set PE multi-function pins for CAN1 TXD(PE.7) and RXD(PE.6) */
    SYS->GPE_MFPL = (SYS->GPE_MFPL & ~(SYS_GPE_MFPL_PE6MFP_Msk | SYS_GPE_MFPL_PE7MFP_Msk)) |
                    (SYS_GPE_MFPL_PE6MFP_CAN1_RXD | SYS_GPE_MFPL_PE7MFP_CAN1_TXD);

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



/**
  * @brief      Init CAN driver
  */

void CAN_Init(CAN_T  *tCAN)
{
    if(tCAN == CAN0) {
        // Enable IP clock
        CLK->APBCLK0 |= CLK_APBCLK0_CAN0CKEN_Msk;

    } else if(tCAN == CAN1) {
        // Enable IP clock
        CLK->APBCLK0 |= CLK_APBCLK0_CAN1CKEN_Msk;

    }
}

/**
  * @brief      Disable CAN
  * @details    Reset and clear all CAN control and disable CAN IP
  */

void CAN_STOP(CAN_T  *tCAN)
{
    if(tCAN == CAN0) {
        /* Disable CAN0 Clock and Reset it */
        SYS->IPRST1 |= SYS_IPRST1_CAN0RST_Msk;
        SYS->IPRST1 &= ~SYS_IPRST1_CAN0RST_Msk;
        CLK->APBCLK0 &= ~CLK_APBCLK0_CAN0CKEN_Msk;
    } else if(tCAN == CAN1) {
        /* Disable CAN0 Clock and Reset it */
        SYS->IPRST1 |= SYS_IPRST1_CAN1RST_Msk;
        SYS->IPRST1 &= ~SYS_IPRST1_CAN1RST_Msk;
        CLK->APBCLK0 &= ~CLK_APBCLK0_CAN1CKEN_Msk;
    }
}

/*----------------------------------------------------------------------------*/
/*  Some description about how to create test environment                     */
/*----------------------------------------------------------------------------*/
void Note_Configure()
{
    printf("\n\n");
    printf("+------------------------------------------------------------------------+\n");
    printf("|  About CAN sample code configure                                       |\n");
    printf("+------------------------------------------------------------------------+\n");
    printf("|   The sample code provide a simple sample code for you study CAN       |\n");
    printf("|   Before execute it, please check description as below                 |\n");
    printf("|                                                                        |\n");
    printf("|   1.CAN_TX and CAN_RX should be connected to your CAN transceiver      |\n");
    printf("|   2.Using two module board and connect to the same CAN BUS             |\n");
    printf("|   3.Check the terminal resistor of bus is connected                    |\n");
    printf("|   4.Using UART0 as print message port                                  |\n");
    printf("|                                                                        |\n");
    printf("|  |--------|       |-----------| CANBUS  |-----------|       |--------| |\n");
    printf("|  |        |------>|           |<------->|           |<------|        | |\n");
    printf("|  |        |CAN_TX |   CAN     |  CAN_H  |   CAN     |CAN_TX |        | |\n");
    printf("|  |  M480  |       |Transceiver|         |Transceiver|       |  M480  | |\n");
    printf("|  |        |<------|           |<------->|           |------>|        | |\n");
    printf("|  |        |CAN_RX |           |  CAN_L  |           |CAN_RX |        | |\n");
    printf("|  |--------|       |-----------|         |-----------|       |--------| |\n");
    printf("|   |                                                           |        |\n");
    printf("|   |                                                           |        |\n");
    printf("|   V                                                           V        |\n");
    printf("| UART0                                                         UART0    |\n");
    printf("|(print message)                                          (print message)|\n");
    printf("+------------------------------------------------------------------------+\n");
}

/*----------------------------------------------------------------------------*/
/*  Test Function                                                             */
/*----------------------------------------------------------------------------*/
void CAN_ShowMsg(STR_CANMSG_T* Msg)
{
    uint8_t i;
    printf("Read ID=%8X, Type=%s, DLC=%d,Data=",Msg->Id,Msg->IdType?"EXT":"STD",Msg->DLC);
    for(i=0; i<Msg->DLC; i++)
        printf("%02X,",Msg->Data[i]);
    printf("\n\n");
}

/*----------------------------------------------------------------------------*/
/*  Send Tx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_Tx(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    uint32_t i;

    /* Send a 11-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = 0x7FF;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 7;
    tMsg.Data[1]  = 0xFF;

    if(CAN_Transmit(tCAN, MSG(0),&tMsg) == FALSE) { // Configure Msg RAM and send the Msg in the RAM
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(0).Send STD_ID:0x7FF, Data[07,FF]done\n");

    /* Send a 29-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x12345;
    tMsg.DLC      = 3;
    tMsg.Data[0]  = 1;
    tMsg.Data[1]  = 0x23;
    tMsg.Data[2]  = 0x45;

    if(CAN_Transmit(tCAN, MSG(1),&tMsg) == FALSE) {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(1).Send EXT:0x12345 ,Data[01,23,45]done\n");

    /* Send a data message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x7FF01;
    tMsg.DLC      = 4;
    tMsg.Data[0]  = 0xA1;
    tMsg.Data[1]  = 0xB2;
    tMsg.Data[2]  = 0xC3;
    tMsg.Data[3]  = 0xD4;

    if(CAN_Transmit(tCAN, MSG(3),&tMsg) == FALSE) {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(3).Send EXT:0x7FF01 ,Data[A1,B2,C3,D4]done\n");

    for(i=0; i < 10000; i++);

    printf("Trasmit Done!\nCheck the receive host received data\n\n");

}

/*----------------------------------------------------------------------------*/
/*  Receive Rx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_SetRxMsg(CAN_T *tCAN)
{
    if(CAN_SetRxMsg(tCAN, MSG(0),CAN_STD_ID, 0x7FF) == FALSE) {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(5),CAN_EXT_ID, 0x12345) == FALSE) {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(31),CAN_EXT_ID, 0x7FF01) == FALSE) {
        printf("Set Rx Msg Object failed\n");
        return;
    }

}

void Test_NormalMode_WaitRxMsg(CAN_T *tCAN)
{
    /*Choose one mode to test*/
#if 1
    /* Polling Mode */
    while(1) {
        while(tCAN->IIDR ==0);            /* Wait IDR is changed */
        CAN_Receive(tCAN, tCAN->IIDR -1, &rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
#else
    /* INT Mode */
    CAN_EnableInt(tCAN, CAN_CON_IE_Msk);
    NVIC_SetPriority(CAN1_IRQn, (1<<__NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ(CAN1_IRQn);

    printf("Enter any key to exit\n");
    getchar();
#endif
}

int main()
{
    SYS_Init();
    UART0_Init();

    /* Select CAN Multi-Function */
    CAN_Init(CAN0);
    CAN_Init(CAN1);

    Note_Configure();

    CAN_Open(CAN0,  500000, CAN_NORMAL_MODE);
    CAN_Open(CAN1,  500000, CAN_NORMAL_MODE);

    printf("\n");
    printf("+------------------------------------------------------------------ +\n");
    printf("|  Nuvoton CAN BUS DRIVER DEMO                                      |\n");
    printf("+-------------------------------------------------------------------+\n");
    printf("|  Transmit/Receive a message by normal mode                        |\n");
    printf("+-------------------------------------------------------------------+\n");

    printf("Press any key to continue ...\n\n");
    getchar();

    Test_NormalMode_SetRxMsg(CAN1);

    Test_NormalMode_Tx(CAN0);

    Test_NormalMode_WaitRxMsg(CAN1);

    while(1) ;

}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
