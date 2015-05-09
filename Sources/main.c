#include "MPC5604B.h"
#include "IntcInterrupts.h"
#include "typedefs.h"
#include "MPC5604B.h"

#define D0 SIU.GPDO[12].B.PDO
#define D1 SIU.GPDO[13].B.PDO
#define D2 SIU.GPDO[14].B.PDO
#define D3 SIU.GPDO[15].B.PDO
unsigned int Line = 0;
unsigned char SampleFlag = 0;
void delay_us(unsigned long int us)
{
	volatile int i, j;
	
	for (i = 0; i < us; i++)
	{
		for (j = 0; j < 9; j++) {}
	}
}

/*-----------------------------------------------------------------------*/
/* 设置单片机的模式和时钟                                                */
/*-----------------------------------------------------------------------*/
void init_modes_and_clock(void) 
{
    ME.MER.R = 0x0000001D;	/* Enable DRUN, RUN0, SAFE, RESET modes */
	/* 设置sysclk */
    //CGM.FMPLL_CR.R = 0x02400100;	/* 8 MHz xtal: Set PLL0 to 64 MHz */
    CGM.FMPLL_CR.R = 0x01280000;	/* 8 MHz xtal: Set PLL0 to 80 MHz */
    //CGM.FMPLL_CR.R = 0x013C0000;	/* 8 MHz xtal: Set PLL0 to 120 MHz */ 
    ME.RUN[0].R = 0x001F0064;	/* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL     sysclk选择锁相环时钟 */
    ME.RUNPC[0].R = 0x00000010;	/* Peri. Cfg. 1 settings: only run in RUN0 mode      选择RUN0模式 */
  
	/* PCTL[?] 选择需要时钟模块(默认即可，不用设置) */
	//ME.PCTL[32].R = 0x00;	/* MPC56xxB/P/S ADC 0: select ME.RUNPC[0] */
    ME.PCTL[32].B.DBG_F = 0;
	/* Mode Transition to enter RUN0 mode */
    ME.MCTL.R = 0x40005AF0;	/* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x4000A50F;	/* Enter RUN0 Mode & Inverted Key */
    
    while (ME.GS.B.S_MTRANS) {}	/* Wait for mode transition to complete 等待模式转换完成 */
    while(ME.GS.B.S_CURRENTMODE != 4) {} /* Verify RUN0 is the current mode 等待选择RUN0模式 */
  
	/* 开peri0、1、2 */
	/* 外设时钟总线 可用于分频 */
	CGM.SC_DC[0].R = 0x84;	/* LIN */
	CGM.SC_DC[1].R = 0x80;	/* FLEXCAN,DSPI */
    CGM.SC_DC[2].R = 0x80;	/* eMIOS,CTU,ADC */
}


/*-----------------------------------------------------------------------*/
/* 禁用看门狗                                                            */
/* 据徐博说应该首先运行                                                  */
/*-----------------------------------------------------------------------*/
void disable_watchdog(void)
{
	SWT.SR.R = 0x0000c520;	/* rite keys to clear soft lock bit */
	SWT.SR.R = 0x0000d928;
	SWT.CR.R = 0x8000010A;	/* Clear watchdog enable (WEN) */
}


/*-----------------------------------------------------------------------*/
/* 初始化all灯                                                         */
/*-----------------------------------------------------------------------*/
void init_led(void)
{

	//第二版板载LED
	SIU.PCR[12].R = 0x0203;/* PA12  */
	SIU.PCR[13].R = 0x0203;/* PA13  */
	SIU.PCR[14].R = 0x0203;/* PA14  */
	SIU.PCR[15].R = 0x0203;/* PA15  */

	D0 = 1;	/* 1=熄灭 */
	D1 = 1;
	D2 = 1;
	D3 = 1;
	

//	LeftL = 0;	/* 0=熄灭 */
//	RightL = 0;
//	StopL = 0;
//	RunL = 0;
}

/*-----------------------------------------------------------------------*/
/* 使能外部中断                                                          */
/* 总开关                                                                */
/*-----------------------------------------------------------------------*/
void enable_irq(void)
{
  INTC.CPR.B.PRI = 0;	/* Single Core: Lower INTC's current priority */
  asm(" wrteei 1");	/* Enable external interrupts */
}

void writereset(void)
{
	SIU.PCR[12].R = 0x0203;	// PA12 as WRST
	SIU.GPDO[12].B.PDO=1; 
	delay_us(150);
	SIU.GPDO[12].B.PDO=0; 
	
}
void rck_high(void)
{
	SIU.PCR[9].R = 0x0203;	// PA9 as RCK
	SIU.GPDO[9].B.PDO=1; 
}
void rck_low(void)
{
	SIU.PCR[9].R = 0x0203;	// PA9 as RCK
	SIU.GPDO[9].B.PDO=0; 
}
void readreset(void)
{
	SIU.PCR[8].R = 0x0203;	// PA8 as RRST
	SIU.GPDO[8].B.PDO=0;    //0 as RRST Enable
	delay_us(150);
	rck_high();
	delay_us(150);
	rck_low();
	delay_us(150);
	SIU.GPDO[8].B.PDO=1; 
}
void oe_enable(void)
{
	SIU.PCR[11].R = 0x0203;	// PA11 as OE
	SIU.GPDO[11].B.PDO=0;   // OE enable
}
void we_enable(void)
{
	SIU.PCR[10].R = 0x0203;	// PA10 as WE
	SIU.GPDO[10].B.PDO=1;   //WE enable
}
void we_disable(void)
{
	SIU.PCR[10].R = 0x0203;	// PA10 as WE
	SIU.GPDO[10].B.PDO=0;   //WE disable
}
void init_fifoinit(void)
{
	//SIU.PCR[10].R = 0x0203;	// PA10 as WE
	//SIU.GPDO[10].B.PDO=0;   //WE disable
	we_disable();
	SIU.PCR[11].R = 0x0203;	// PA11 as OE
	SIU.GPDO[11].B.PDO=1;   // OE disable
	writereset();
	readreset();
}
void FieldInputCapture(void)
{
	EMIOS_0.CH[3].CSR.B.FLAG = 1;//清场中断
	EMIOS_0.CH[7].CSR.B.FLAG = 1;//清行中断
	SampleFlag=1;
	we_enable();
	Line=0;	
}
void RowInputCapture(void)
{
	EMIOS_0.CH[7].CSR.B.FLAG = 1;//清行中断
	Line++;
	if ( SampleFlag == 0 ) 
	{ 
		return;
	} 
	if(Line==240)
	{
		writereset();
		we_disable();
		
	}
}

/*----------------------------------------------------------------------*/
/*视频信号场行中断初始                                             	 */
/*----------------------------------------------------------------------*/
void initEMIOS_0Image(void) 
{ 
	//PA3场中断捕捉上升沿及下降沿
	EMIOS_0.CH[3].CCR.B.MODE = 0x02; // Mode is SAIC, continuous 
	EMIOS_0.CH[3].CCR.B.BSL = 0x01; /* Use counter bus B (default) */
	EMIOS_0.CH[3].CCR.B.EDSEL = 1;  //Both edges
//	EMIOS_0.CH[3].CCR.B.EDPOL=1; //Edge Select falling edge
//	EMIOS_0.CH[3].CCR.B.FEN=1;  //interupt enbale
	SIU.PCR[3].R = 0x0102;  // Initialize pad for eMIOS channel Initialize pad for input 
	INTC_InstallINTCInterruptHandler(FieldInputCapture,142,1);  
	
	//PA7行中断捕捉上升沿
	EMIOS_0.CH[7].CCR.B.MODE = 0x02; // Mode is SAIC, continuous 
	EMIOS_0.CH[7].CCR.B.BSL = 0x01; /* Use counter bus B (default) */
	EMIOS_0.CH[7].CCR.B.EDSEL = 0;
	EMIOS_0.CH[7].CCR.B.EDPOL=1; //Edge Select rising edge
//	EMIOS_0.CH[7].CCR.B.FEN=1;  //interupt enbale
	SIU.PCR[7].R = 0x0102;  // Initialize pad for eMIOS channel Initialize pad for input 
	INTC_InstallINTCInterruptHandler(RowInputCapture,144,3); 
	
	//C10口二值化入口
	SIU.PCR[42].R = 0x0102;  // C9口二值化入口
}

void main(void) {
  //volatile int i = 0;

  	//int flag = 1;
  	int i,j;
  	disable_watchdog();
  	init_modes_and_clock();
  	enable_irq();
  	initEMIOS_0Image();
  	init_fifoinit();
  	//init_all_and_POST();
  	//g_f_enable_speed_control=1;
  	//write_camera_data_to_TF();
  	//set_speed_target(10);
  	//SetupBKL();
  //	SetupCCD();	
  	//g_f_enable_speed_control=0;
  	//g_f_enable_supersonic=1;
  //	EMIOS_0.CH[3].CCR.B.FEN=1;//开场中断
  //	LCD_write_english_string(96,0,"T");


  	/* Loop forever */
  	for(;;)
  	{
  		//PGPDI
  		oe_enable();
  		readreset();
  		for(i=0;i<240;i++)
  		{
  			for(j=0;j<640;j++)
  			{
  				rck_high();
  				//delay_us(150);
  				rck_low();
  				//delay_us(150);
  			}
  		}
  	}
  	

  /* Loop forever */
  
  }




