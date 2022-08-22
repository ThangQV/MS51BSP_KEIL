/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2020 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/


//***********************************************************************************************************
//  File Function: MS51 R/W EEPROM sample code
//***********************************************************************************************************
#include "MS51_8K.H"

/* For printf code only. Disable this define to reduce code size. */
//#define print_function 

//***********************************************************************************************************
//  File Function: MS51 I2C demo code, Slave Address of 24LC64 = 0xA0
//
//   ____________           ______________ 
//  |            |   SDA    |             |
//  |            |<-------->|             |
//  |            |          |             |
//  | MS51(M)    |          |   24LC64(S) |
//  |            |   SCL    |             |
//  |            |--------->|             |
//  |____________|          |_____________|
//
//  Microchip I2C EEPROM 24xx64 (64K Bit) is used as the slave device.  
//  The page size are 32Byte. Total are 256 page.
//  If verification passes, Port3 will show 0x78. If there is any failure
//  occured during the progress, Port3 will show 0x00.
//***********************************************************************************************************

#define I2C_CLOCK               2

#define EEPROM_SLA              0xA0
#define EEPROM_WR               0
#define EEPROM_RD               1

#define EEPROM_PAGE_SIZE        32
#define PAGE_NUMBER             4

#define ERROR_CODE              0x78
#define TEST_OK                 0x00

bit I2C_Reset_Flag;
//========================================================================================================
void Init_I2C(void)
{
    P03_OPENDRAIN_MODE;     /*I2C_SCL*/
    P04_OPENDRAIN_MODE;     /*I2C_SDA*/
    set_P0S_3;
    set_P0S_4;
    /* Set I2C clock rate */
    I2CLK = I2C_CLOCK; 

    /* Enable I2C */
    set_I2CON_I2CEN;
}
//========================================================================================================
void I2C_SI_Check(void)
{
    if (I2STAT == 0x00)
    {
        I2C_Reset_Flag = 1;
        set_I2CON_STO;
        clr_I2CON_SI;
        if(I2CON|CLR_BIT3)
        {
            clr_I2CON_I2CEN;
            set_I2CON_I2CEN;
            clr_I2CON_SI;
            clr_I2CON_I2CEN;    
        }   
    }  
}

void One_Page_Read(UINT8 u8PageNumber,UINT8 u8DAT)
{
    UINT8  u8Count;
    UINT16 u16Address;

    u16Address = (UINT16)u8PageNumber*32;

    /* Step1 */
    set_I2CON_STA;                                /* Send Start bit to I2C EEPROM */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x08)                     /* 0x08:  A START condition has been transmitted*/
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send STA' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step2 */
    I2DAT = (EEPROM_SLA | EEPROM_WR);       /* Send (SLA+W) to EEPROM */
    clr_I2CON_STA;                                /* Clear STA and Keep SI value in I2CON */    
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x18)                     /* 0x18: SLA+W has been transmitted; ACK has been received */              
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send SLA+W' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step3 */
    I2DAT = HIBYTE(u16Address);             /* Send I2C EEPROM's High Byte Address */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x28)                     /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */              
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send I2C High Byte Address' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step4 */
    I2DAT = LOBYTE(u16Address);             /* Send I2C EEPROM's Low Byte Address */
    clr_I2CON_SI;
   while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x28)                     /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */             
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send I2C Low Byte Address' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step5 */
    set_I2CON_STA;                                /* Repeated START */
    clr_I2CON_SI; 
   while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x10)                     /* 0x10: A repeated START condition has been transmitted */
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send STA' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step6 */
    clr_I2CON_STA;                                /* Clear STA and Keep SI value in I2CON */
    I2DAT = (EEPROM_SLA | EEPROM_RD);       /* Send (SLA+R) to EEPROM */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x40)                     /* 0x40:  SLA+R has been transmitted; ACK has been received */              
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send SLA+R' error");
#endif        
        goto Read_Error_Stop;
    }

    /* Step7 */                             /* Verify I2C EEPROM data */
    for (u8Count = 0; u8Count <EEPROM_PAGE_SIZE; u8Count++)
    {
        set_I2CON_AA;                             /* Set Assert Acknowledge Control Bit */
        clr_I2CON_SI;
       while (!(I2CON&SET_BIT3));
        if (I2STAT != 0x50)                 /* 0x50:Data byte has been received; NOT ACK has been returned */              
        {
            I2C_Reset_Flag = 1;
#ifdef print_function
            printf("\nI2C 'No Ack' error");
#endif        
            goto Read_Error_Stop;
        }
       
        if (I2DAT != u8DAT)                 /* Send the Data to EEPROM */    
        {
            I2C_Reset_Flag = 1;
#ifdef print_function
            printf("\nI2C 'Read data' error");
#endif        
            goto Read_Error_Stop;
        }
        u8DAT = ~u8DAT; 
    }

    /* Step8 */
    clr_I2CON_AA;                                 /* Send a NACK to disconnect 24xx64 */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));
    if (I2STAT != 0x58)                     /* 0x58:Data byte has been received; ACK has been returned */
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Ack' error");
#endif        
        goto Read_Error_Stop;
    }
    
  /* Step9 */    
    clr_I2CON_SI;
    set_I2CON_STO;
    while (I2CON|CLR_BIT4)                        /* Check STOP signal */
    {
      I2C_SI_Check();
      if (I2C_Reset_Flag)
        goto Read_Error_Stop;
    }
    
Read_Error_Stop: 
    if (I2C_Reset_Flag)
    {
        I2C_SI_Check();
#ifdef print_function
        printf("\nI2C Read error, test stop");
#endif        
        I2C_Reset_Flag = 0;
    }
}
//========================================================================================================
void One_Page_Write(UINT8 u8PageNumber,UINT8 u8DAT)
{
    UINT8  u8Count;
    UINT16 u16Address;

    u16Address = (UINT16)u8PageNumber*32;

    /* Step1 */
    set_I2CON_STA;                                /* Send Start bit to I2C EEPROM */         
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
    if (I2STAT != 0x08)                     /* 0x08:  A START condition has been transmitted*/
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send STA' error");
#endif        
        goto Write_Error_Stop;
    }

    /* Step2 */
    clr_I2CON_STA;                                /* Clear STA and Keep SI value in I2CON */
    I2DAT = EEPROM_SLA | EEPROM_WR;         /* Send (SLA+W) to EEPROM */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
    if (I2STAT != 0x18)                     /* 0x18: SLA+W has been transmitted; ACK has been received */             
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send SLA+W' error");
#endif        
        goto Write_Error_Stop;
    }

    /* Step3 */
    I2DAT = HIBYTE(u16Address);             /* Send EEPROM's High Byte Address */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
    if (I2STAT != 0x28)                     /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send High byte address' error");
#endif        
        goto Write_Error_Stop;
    }

    /* Step4 */
    I2DAT = LOBYTE(u16Address);             /* Send EEPROM's Low Byte Address */
    clr_I2CON_SI;
    while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
    if (I2STAT != 0x28)                     /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
    {
        I2C_Reset_Flag = 1;
#ifdef print_function
        printf("\nI2C 'Send Low byte address' error");
#endif        
        goto Write_Error_Stop;
    }

    /* Step5 */
                                            /* Write data to I2C EEPROM */
    for (u8Count = 0; u8Count < EEPROM_PAGE_SIZE; u8Count++)
    {
        I2DAT = u8DAT;                      /* Send data to EEPROM */
        clr_I2CON_SI;
        while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
        if (I2STAT != 0x28)                 /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
            I2C_Reset_Flag = 1;
#ifdef print_function
            printf("\nI2C 'Write Data' error");
#endif        
            goto Write_Error_Stop;
        }   
        u8DAT = ~u8DAT;        
    }
                                            //After STOP condition, a inner EEPROM timed-write-cycle 
                                            //will occure and EEPROM will not response to outside command
                                            /* 0x18: SLA+W has been transmitted; ACK has been received */
    /* Step6 */
    do
    {
        set_I2CON_STO;                            /* Set I2C STOP Control Bit */
        clr_I2CON_SI;
        while (I2CON|CLR_BIT4)                        /* Check STOP signal */
        {
          I2C_SI_Check();
          if (I2C_Reset_Flag)
            goto Write_Error_Stop;
        }
        
        set_I2CON_STA;                            /* Check if no ACK is returned by EEPROM, it is under timed-write cycle */
        clr_I2CON_SI;
        while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
        if (I2STAT != 0x08)                 /* 0x08:  A START condition has been transmitted*/
        {
            I2C_Reset_Flag = 1;
#ifdef print_function
            printf("\nI2C 'Wait Ready' error");
#endif        
            goto Write_Error_Stop;
        }

        clr_I2CON_STA;                            /* Clear STA and Keep SI value in I2CON */
        I2DAT = (EEPROM_SLA | EEPROM_WR);   /* Send (SLA+W) to EEPROM */
        clr_I2CON_SI;
        while (!(I2CON&SET_BIT3));               /*Wait SI to be 1 */
    }while(I2STAT != 0x18);

    /* Step7 */
    set_I2CON_STO;                                /* Set STOP Bit to I2C EEPROM */
    clr_I2CON_SI;
     while (I2CON|CLR_BIT4)                        /* Check STOP signal */
    {
      I2C_SI_Check();
      if (I2C_Reset_Flag)
        goto Write_Error_Stop;
    }
    
Write_Error_Stop: 
    if (I2C_Reset_Flag)
    {
        I2C_SI_Check();
        I2C_Reset_Flag  = 0;
#ifdef print_function
        printf("\nI2C Write error, test stop");
#endif        
    }
}
//========================================================================================================
void main(void)
{


/* Initial UART0 for printf */
    P06_PUSHPULL_MODE;
    UART_Open(16000000,UART0_Timer3,115200);
    ENABLE_UART0_PRINTF;

  
    /* Initial I2C function */
    Init_I2C();                                 //initial I2C circuit
    
    /* page0 R/W */
#ifdef print_function
    printf ("\n\n24LC64 Page0 Write (0x55,0xAA...)...");
#endif        
    One_Page_Write(0,0x55);                     //page0, write 0x55,0xAA,........

#ifdef print_function
    printf ("\n\n24LC64 Page0 Read...");
#endif        
    One_Page_Read (0,0x55);                     //page0, read  0x55,0xAA,........

    /* page1 R/W */
#ifdef print_function
    printf ("\n\n24LC64 Page1 Write (0x00,0xFF...)...");
#endif        
    One_Page_Write(1,0x00);                     //page1, write 0x00,0xFF,........
#ifdef print_function
    printf ("\n\n24LC64 Page1 Read...");
#endif        
    One_Page_Read (1,0x00);                     //page1, read  0x00,0xFF,........

    /* page2 R/W */
#ifdef print_function
    printf ("\n\n24LC64 Page2 Write (0xAA,0x55...)...");
#endif        
    One_Page_Write(2,0xAA);                     //page2, write 0xAA,0x55,........
#ifdef print_function
    printf ("\n\n24LC64 Page2 Read...");
#endif        
    One_Page_Read (2,0xAA);                     //page2, read  0xAA,0x55,........

    /* page3 R/W */
#ifdef print_function
    printf ("\n\n24LC64 Page3 Write (0xFF,0x00...)...");
#endif        
    One_Page_Write(3,0xFF);                     //page3, write 0xFF,0x00,........
#ifdef print_function
    printf ("\n\n24LC64 Page3 Read...");
#endif        
    One_Page_Read (3,0xFF);                     //page3, read  0xFF,0x00,........

    /* page255 R/W */
#ifdef print_function
    printf ("\n\n24LC64 Page255 Write (0x0F,0xF0...)...");
#endif        
    One_Page_Write(255,0x0F);                   //page255, write 0x0F,0xF0,........
#ifdef print_function
    printf ("\n\n24LC64 Page255 Read...");
#endif        
    One_Page_Read (255,0x0F);                   //page255, read  0x0F,0xF0,........

    /* ==== Test Pass ==== */      
#ifdef print_function
    printf ("\n\MS51 <--> 24LC64, I2C Demo Code test pass...");
#endif        
    DISABLE_UART0_PRINTF;

while (1);
/* =================== */
}

