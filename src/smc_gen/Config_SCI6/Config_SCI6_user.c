/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products.
* No other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
* OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY
* LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR
* ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability 
* of this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : Config_SCI6_user.c
* Version      : 1.8.0
* Device(s)    : R5F571MFCxFP
* Description  : This file implements device driver for Config_SCI6.
* Creation Date: 2020-02-13
***********************************************************************************************************************/

/***********************************************************************************************************************
Pragma directive
***********************************************************************************************************************/
/* Start user code for pragma. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_macrodriver.h"
#include "Config_SCI6.h"
/* Start user code for include. Do not edit comment generated here */
#include "I2C_MPU9250.h"
/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
extern volatile uint8_t   g_sci6_iic_transmit_receive_flag;  /* SCI6 transmit receive flag for I2C */
extern volatile uint8_t   g_sci6_iic_cycle_flag;             /* SCI6 start stop flag for I2C */
extern volatile uint8_t   g_sci6_slave_address;              /* SCI6 target slave address */
extern volatile uint8_t * gp_sci6_tx_address;                /* SCI6 send buffer address */
extern volatile uint16_t  g_sci6_tx_count;                   /* SCI6 send data number */
extern volatile uint8_t * gp_sci6_rx_address;                /* SCI6 receive buffer address */
extern volatile uint16_t  g_sci6_rx_count;                   /* SCI6 receive data number */
extern volatile uint16_t  g_sci6_rx_length;                  /* SCI6 receive data length */
/* Start user code for global. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
* Function Name: R_Config_SCI6_Create_UserInit
* Description  : This function adds user code after initializing the SCI6 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_SCI6_Create_UserInit(void)
{
    /* Start user code for user init. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI6_transmit_interrupt
* Description  : This function is TXI6 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

#if FAST_INTERRUPT_VECTOR == VECT_SCI6_TXI6
#pragma interrupt r_Config_SCI6_transmit_interrupt(vect=VECT(SCI6,TXI6),fint)
#else
#pragma interrupt r_Config_SCI6_transmit_interrupt(vect=VECT(SCI6,TXI6))
#endif
static void r_Config_SCI6_transmit_interrupt(void)
{
    if (0U == SCI6.SISR.BIT.IICACKR)
    {
        if (_80_SCI_IIC_TRANSMISSION == g_sci6_iic_transmit_receive_flag)
        {
            if (g_sci6_tx_count > 0U)
            {
                SCI6.TDR = *gp_sci6_tx_address;
                gp_sci6_tx_address++;
                g_sci6_tx_count--;
            }
            else
            {
                /* Generate stop condition */
                g_sci6_iic_cycle_flag = _00_SCI_IIC_STOP_CYCLE;
                R_Config_SCI6_IIC_StopCondition();
            }
        }
        else if (_00_SCI_IIC_RECEPTION == g_sci6_iic_transmit_receive_flag)
        {
            SCI6.SIMR2.BIT.IICACKT = 0U;
            SCI6.SCR.BIT.RIE = 1U;
            if (g_sci6_rx_length == (g_sci6_rx_count + 1))
            {
                SCI6.SIMR2.BIT.IICACKT = 1U;
            }

            /* Write dummy */
            SCI6.TDR = 0xFFU;
        }
    }
    else
    {
        /* Generate stop condition */
        g_sci6_iic_cycle_flag = _00_SCI_IIC_STOP_CYCLE;
        R_Config_SCI6_IIC_StopCondition();
    }
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI6_transmitend_interrupt
* Description  : This function is TEI6 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void r_Config_SCI6_transmitend_interrupt(void)
{
    if (_80_SCI_IIC_START_CYCLE == g_sci6_iic_cycle_flag)
    {
        SCI6.SIMR3.BIT.IICSTIF = 0U;
        SCI6.SIMR3.BIT.IICSCLS = 0U;
        SCI6.SIMR3.BIT.IICSDAS = 0U;
        SCI6.TDR = g_sci6_slave_address;
    }
    else if (_00_SCI_IIC_STOP_CYCLE == g_sci6_iic_cycle_flag)
    {
        SCI6.SIMR3.BIT.IICSTIF = 0U;
        SCI6.SIMR3.BYTE |= (_30_SCI_SSDA_HIGH_IMPEDANCE | _C0_SCI_SSCL_HIGH_IMPEDANCE);
        if (_80_SCI_IIC_TRANSMISSION == g_sci6_iic_transmit_receive_flag)
        {
            r_Config_SCI6_callback_transmitend();
        }
        if (_00_SCI_IIC_RECEPTION == g_sci6_iic_transmit_receive_flag)
        {
            r_Config_SCI6_callback_receiveend();
        }
    }
    else
    {
        /* Do nothing */
    }
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI6_receive_interrupt
* Description  : This function is RXI6 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

#if FAST_INTERRUPT_VECTOR == VECT_SCI6_RXI6
#pragma interrupt r_Config_SCI6_receive_interrupt(vect=VECT(SCI6,RXI6),fint)
#else
#pragma interrupt r_Config_SCI6_receive_interrupt(vect=VECT(SCI6,RXI6))
#endif
static void r_Config_SCI6_receive_interrupt(void)
{
    if (g_sci6_rx_length > g_sci6_rx_count)
    {
        *gp_sci6_rx_address = SCI6.RDR;
        gp_sci6_rx_address++;
        g_sci6_rx_count++;
    }
    else
    {
        /* Generate stop condition */
        g_sci6_iic_cycle_flag = _00_SCI_IIC_STOP_CYCLE;
        R_Config_SCI6_IIC_StopCondition();
    }
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI6_callback_transmitend
* Description  : This function is a callback function when SCI6 finishes transmission
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_Config_SCI6_callback_transmitend(void)
{
    /* Start user code for r_Config_SCI6_callback_transmitend. Do not edit comment generated here */
    busIMU = BUS_IMU_FREE;
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI6_callback_receiveend
* Description  : This function is a callback function when SCI6 finishes reception
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_Config_SCI6_callback_receiveend(void)
{
    /* Start user code for r_Config_SCI6_callback_receiveend. Do not edit comment generated here */
    busIMU = BUS_IMU_FREE;
    /* End user code. Do not edit comment generated here */
}

/* Start user code for adding. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */   




