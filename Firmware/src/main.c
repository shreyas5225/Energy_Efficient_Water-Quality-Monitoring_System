/***************************************************************************
 * Team AQUA
 * Project: Water Quality Monitoring System
 * Team members: Omkar Purandare / Shreyas Vasanthkumar
 ******************************************************************************/
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_leuart.h"
#include "em_usart.h"
#include "em_dma.h"
#include "dmactrl.h"
#include "em_letimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/** LEUART, USART0, USART1 Rx/Tx Port/Pin Location */
#define LEUART_LOCATION    0
#define LEUART_TXPORT      gpioPortC            /* LEUART transmission port */
#define LEUART_TXPIN       6                  /* LEUART transmission pin */
#define LEUART_RXPORT      gpioPortC            /* LEUART reception port */
#define LEUART_RXPIN       7                /* LEUART reception pin */
#define LOCATION_USART0	   0				//USART 0 LOCATION
#define LOCATION_USART1	   0x1<<8			//USART 1 location
#define const_LETIMER_LFXO_Top 32767
#define TEMP_SENSORDQPORT  gpioPortD
#define TEMP_SENSORDQPIN   5

/****Functions used********/
char* calculate_pH();
char* calculate_ORP();
char* calculate_temp();
int16_t max_occ_ORP(int16_t *str1);
uint8_t max_occ_ph(uint8_t *str1);
void transmit_message(char *ph, char *orp, char *temp);
void send_command(char *cmd, int skip);
void Delay(uint32_t dlyTicks);
void temp_sensor_init();
unsigned char temp_sensor_reset();
void temp_sensor_init();
unsigned char temp_sensor_reset();
unsigned char temp_sensor_readbit();
unsigned char temp_sensor_readbyte();
void temp_sensor_writebit(char value);
void temp_sensor_writebyte(char value);
int temp_sensor_gettemp();
void Delay_temp(uint32_t dlyTicks);

/******Global varriables*******/
uint32_t count_isr=0;
volatile uint32_t msTicks; /* counts 1ms timeTicks */

//Delay function
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

//Delay function for temp_sensor Delay
void Delay_temp(uint32_t dlyTicks)
{
	for(int i=0;i<dlyTicks;i++);

}
/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/*****function to setup LETIMER*****/
void setup_LETIMER0()
{

	uint32_t comp=0,flags=0;
	LETIMER_Init_TypeDef LETIMER0_init;
	LETIMER0_init.bufTop= false;
	LETIMER0_init.comp0Top= false;
	LETIMER0_init.debugRun= false;
	LETIMER0_init.enable=false;
	LETIMER0_init.out0Pol=0;
	LETIMER0_init.out1Pol=0;
	LETIMER0_init.repMode=letimerRepeatFree;
	LETIMER0_init.rtcComp0Enable=false;
	LETIMER0_init.rtcComp1Enable=false;
	LETIMER0_init.ufoa0=letimerUFOANone;
	LETIMER0_init.ufoa1=letimerUFOANone;
	LETIMER_Init(LETIMER0,&LETIMER0_init);


	//Interrupt Flag Setting
	flags= LETIMER0->IF;
	LETIMER0->IFC=flags;
	LETIMER_IntEnable(LETIMER0,LETIMER_IEN_UF);

	NVIC_EnableIRQ(LETIMER0_IRQn);
	LETIMER_Enable(LETIMER0,true);
}

/****LETIMER HANDLER******/
void LETIMER0_IRQHandler(void)
{
	uint16_t flags=LETIMER0->IF;
	LETIMER0->IFC=flags;
	count_isr++;
}

/****SETUP LEUART*****/
void setupLeuart(void)
{
  /* Enable peripheral clocks */
  CMU_ClockEnable(cmuClock_HFPER, true);
  /* Configure GPIO pins */
  /* To avoid false start, configure output as high */
  GPIO_PinModeSet(LEUART_TXPORT, LEUART_TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(LEUART_RXPORT, LEUART_RXPIN, gpioModeInput, 0);

  LEUART_Init_TypeDef init = LEUART_INIT_DEFAULT;

  /* Enable CORE LE clock in order to access LE modules */
  CMU_ClockEnable(cmuClock_CORELE, true);

  /* Select LFXO for LEUARTs (and wait for it to stabilize) */
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LEUART1, true);

  /* Do not prescale clock */
  CMU_ClockDivSet(cmuClock_LEUART1, cmuClkDiv_1);

  /* Configure LEUART */
  init.enable = leuartDisable;

  LEUART_Init(LEUART1, &init);

  /* Enable pins at default location */
  LEUART1->ROUTE = LEUART_ROUTE_RXPEN | LEUART_ROUTE_TXPEN | LEUART_LOCATION;


  /* Finally enable it */
  LEUART_Enable(LEUART1, leuartEnable);
}

/*****Setup USART1********/
void setupUSART1(void)
{
	/* USART is a HFPERCLK peripheral. Enable HFPERCLK domain and USART0.
	 * We also need to enable the clock for GPIO to configure pins. */
	CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_USART1, true);
	USART_Enable(USART1,usartDisable );
		/* To avoid false start, configure TX pin as initial high */
	GPIO_PinModeSet(gpioPortD, 0, gpioModePushPull, 1); //TX
	GPIO_PinModeSet(gpioPortD, 1 ,gpioModeInput, 0);//RX
	/* Initialize with default settings and then update fields according to application requirements. */
	USART_InitAsync_TypeDef initAsync = USART_INITASYNC_DEFAULT;
	initAsync.baudrate = 9600;
	USART_InitAsync(USART1, &initAsync);
	USART1->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN | LOCATION_USART1;
	USART_Enable(USART1,usartEnable );
		  /* Don't enable CTS/RTS hardware flow control pins in this example. */
}

/*****Setup USART0********/
void setupUSART0(void)
{
	/* USART is a HFPERCLK peripheral. Enable HFPERCLK domain and USART0.
	 * We also need to enable the clock for GPIO to configure pins. */
	CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_USART0, true);
	USART_Enable(USART0,usartDisable );
		/* To avoid false start, configure TX pin as initial high */
	GPIO_PinModeSet(gpioPortE, 10, gpioModePushPull, 1); //TX
	GPIO_PinModeSet(gpioPortE, 11 ,gpioModeInput, 0);//RX
	/* Initialize with default settings and then update fields according to application requirements. */
	USART_InitAsync_TypeDef initAsync = USART_INITASYNC_DEFAULT;
	initAsync.baudrate = 9600;
	USART_InitAsync(USART0, &initAsync);
	USART0->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN | LOCATION_USART0;
	USART_Enable(USART0,usartEnable );
		  /* Don't enable CTS/RTS hardware flow control pins in this example. */

}


/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/
int main(void)
{
  /* Chip errata */
  CHIP_Init();

  //CLOCKS AND SETUP FOR LETIMER TO RUN IN BACKGROUD
  CMU_OscillatorEnable(cmuOsc_LFXO,true,true);
  CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_CORELE,true);
  CMU_ClockEnable(cmuClock_LETIMER0,true);
  setup_LETIMER0();
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_DriveModeSet(gpioPortD, gpioDriveModeHigh);
  GPIO_PinModeSet(gpioPortD, 7, gpioModePushPullDrive , 1);

  //SETUP SYSTICK FOR GENERATING DELAY
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000))
  {
      while (1) ;
  }

  char *ph_reading, *ORP_reading,*water_temp;
  Delay(4000);
  GPIO_PinOutClear(gpioPortD, 7);
  Delay(2000);
  GPIO_PinOutSet(gpioPortD, 7);
  while(1)
  {
	  ph_reading= calculate_pH(); //Calculate pH of water
	  ORP_reading= calculate_ORP(); //Calculate ORP of water
	  water_temp=calculate_temp();
	  transmit_message(ph_reading,ORP_reading,water_temp); //Transmit message through GSM
	  EMU_EnterEM2(true); //Enter EM2 sleep
	  count_isr=0;
	  //waiting for wakeup
	  while (1)
	  {
		  if(count_isr==30)
		  {
			  count_isr=0;
			  break;
		  }
	  }
  }
}

/****Function to calculate pH****/
char* calculate_pH()
{
	 char start_read[]="C,1\r", receive_ph[10],tp,end_read[]="C,0\r",*ph_avg_str_ptr;
	 char* ph_avg_str = (char*) malloc(10);
	 ph_avg_str_ptr=ph_avg_str;
	 uint8_t ph_values[10],count=0,j=0,k=0,neglect=0, goon=0;
	 setupUSART0();
	  GPIO_DriveModeSet(gpioPortA, gpioDriveModeHigh);
	  GPIO_PinModeSet(gpioPortA, 7, gpioModePushPullDrive , 0);
	  GPIO_PinOutSet(gpioPortA, 7);
	  Delay(3000);
	  while(1)
	  {
		  goon=0;
		  for (int i=0;i<4;i++)
			 {
			  USART_Tx(USART0,start_read[i]);
			 }
		   while(tp!='\r')
		   {
			  tp=USART_Rx(USART0);
			  if(tp=='E')
				  goon=1;
		   }
		   if(goon==0)
			   break;
	   }
	   while(count<10)
	   {
		receive_ph[j]=USART_Rx(USART0);
		if(receive_ph[j]=='*')
		{
			neglect=1;
		}
		if(receive_ph[j]=='\r')
		{
			j=0;
			if(neglect==0)
			{
				ph_values[k]=atoi(receive_ph);
				count++;
				k++;
			}
			else
				neglect = 0;
		}
		else
			j++;
	   }

	  for (int i=0;i<4;i++)
	  	     {
	  	   	  USART_Tx(USART0,end_read[i]);
	  	     }
	  Delay(3000);
	  GPIO_PinOutClear(gpioPortA, 7);
	  USART_Enable(USART0,usartDisable);
	  uint8_t max=0;
	  max = max_occ_ph(ph_values);
	  itoa(max,ph_avg_str_ptr,10);
	  return ph_avg_str;
}

/*****From the readings of pH finding the mode value*****/
uint8_t max_occ_ph(uint8_t *str1)
{
	uint8_t max_char;
    int max_occ=0,count=0;
    for(int i=0;i<10;i++)
    {
        for(int j=i+1;j<10;j++)
        {
            if(str1[i]==str1[j])
                count++;
        }
        if(count>=max_occ)
        {
            max_occ=count;
            max_char=str1[i];
        }
        count=0;
    }
    return max_char;
}


/****Function to calculate ORP****/
char* calculate_ORP()
{
	  char start_read[]="C,1\r", receive_orp[10],tp,end_read[]="C,0\r",*orp_avg_str_ptr;
	  char* orp_avg_str = (char*) malloc(10);
	  int16_t orp_values[10],count=0,j=0,k=0,neglect=0, goon=0;
	  setupUSART1();
	  GPIO_DriveModeSet(gpioPortB, gpioDriveModeHigh);
	  GPIO_PinModeSet(gpioPortB, 10, gpioModePushPullDrive , 0);
	  GPIO_PinOutSet(gpioPortB, 10);
	  Delay(3000);
	  while(1)
	  {
		  goon=0;
		  for (int i=0;i<4;i++)
			 {
			  USART_Tx(USART1,start_read[i]);
			 }
		   while(tp!='\r')
		   {
			  tp=USART_Rx(USART1);
			  if(tp=='E')
				  goon=1;
		   }
		   if(goon==0)
			   break;
	   }
	   while(count<10)
	   {
		receive_orp[j]=USART_Rx(USART1);
		if(receive_orp[j]=='*')
		{
			neglect=1;
		}
		if(receive_orp[j]=='\r')
		{
			j=0;
			if(neglect==0)
			{
				orp_values[k]=atoi(receive_orp);
				count++;
				k++;
			}
			else
				neglect = 0;
		}
		else
			j++;
	   }

	  for (int i=0;i<4;i++)
			 {
			  USART_Tx(USART1,end_read[i]);
			 }
	  Delay(3000);
	  GPIO_PinOutClear(gpioPortB, 10);
	  USART_Enable(USART1,usartDisable);
	  int16_t max=0;
	  max = max_occ_ORP(orp_values);
	  itoa(max,orp_avg_str,10);
	  return orp_avg_str;
}

/*****From the readings of ORP finding the mode value*****/
int16_t max_occ_ORP(int16_t str1[])
{
	int16_t max_char;
    int16_t max_occ=0,count=0;
    int i,j;
    for(i=0;i<10;i++)
    {
        for(j=i+1;j<10;j++)
        {
            if(str1[i]==str1[j])
                count++;
        }
        if(count>=max_occ)
        {
            max_occ=count;
            max_char=str1[i];
        }
        count=0;
    }
    return max_char;
}

/****Function to transmit and communicate with GSM module*****/
void transmit_message(char *ph, char *orp, char *temp)
{
	char buffer[200];
	GPIO_PinOutClear(gpioPortD, 7);
	Delay(2000);
	GPIO_PinOutSet(gpioPortD, 7);
	Delay(20000);
	setupLeuart();
	sprintf(buffer,"AQUA BOT WATER QUALITY REPORT:\nPH:%s\nORP:%s\nTemperature:%sF",ph,orp,temp);
	send_command("AT\r",0);
	send_command("AT+CMGS=\"17207557811\"\r",0);
	send_command(buffer,1);
	LEUART_Tx(LEUART1,0x1A);
	Delay(5000);
	GPIO_PinOutClear(gpioPortD, 7);
	Delay(2000);
	GPIO_PinOutSet(gpioPortD, 7);
	return;
}

/****Function to send a command through GSM********/
void send_command(char *cmd, int skip)
{
	char receive_message[100],tp;
	while(1)
		{
			bzero(receive_message,sizeof(receive_message));
			char *rec_ptr=receive_message;
			for(int i=0;i<strlen(cmd);i++)
			{
				LEUART_Tx(LEUART1,cmd[i]);
			}
			if(skip==1)
				break;
			tp=LEUART_Rx(LEUART1);
			tp=LEUART_Rx(LEUART1);
			while(1)
			{
				*rec_ptr=LEUART_Rx(LEUART1);
				if((*rec_ptr=='\n')||(*rec_ptr==' '))
					break;
				rec_ptr++;
			}
			rec_ptr++;
			*rec_ptr='\0';
			if(strstr(receive_message,"ERROR")==NULL)
				break;
		}
	return;
}

char* calculate_temp()
{
	int temp;
	char pre;
	char* temp_avg_str = (char*) malloc(10);
	temp_sensor_init();
	pre = temp_sensor_reset();
	temp_sensor_writebyte(0xCC);
	temp_sensor_writebyte(0x44);
	pre = temp_sensor_reset();
	temp_sensor_writebyte(0xCC);
	temp_sensor_writebyte(0xBE);
	for(int i=0;i<10;i++)
		temp=temp_sensor_gettemp();
	itoa(temp,temp_avg_str,10);
	return temp_avg_str;
}

void temp_sensor_init()
{
	GPIO_DriveModeSet(gpioPortD, gpioDriveModeHigh);
	  GPIO_PinModeSet(gpioPortD, 5, gpioModePushPullDrive , 1);
	 GPIO_DriveModeSet(gpioPortD, gpioDriveModeHigh);
	 GPIO_PinModeSet(gpioPortD, 2, gpioModePushPullDrive , 1);
}

unsigned char temp_sensor_reset()
{
	unsigned char presence=1;
	uint32_t reset_delay = 300;
	uint32_t presence_delay = 35;
	uint32_t presence_pulse = 260;
	Delay_temp(reset_delay);
	GPIO_PinOutClear(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	Delay_temp(reset_delay);
	GPIO_PinOutSet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	Delay_temp(presence_delay);
	GPIO_PinModeSet(gpioPortD, 5, gpioModeInput , 1);
	presence = GPIO_PinInGet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	Delay_temp(presence_pulse);
	return presence;
}

unsigned char temp_sensor_readbit()
{
	unsigned char i,bit_value;
	GPIO_PinOutClear(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	GPIO_PinOutSet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	bit_value = GPIO_PinInGet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	Delay_temp(2);
	return bit_value;
}

unsigned char temp_sensor_readbyte()
{
	unsigned char i,value=0;

	for(i=0;i<8;i++)
	{
		if(temp_sensor_readbit())
			value |= 0x01<<i;
	  Delay_temp(70);
	}
	return value;
}

void temp_sensor_writebit(char value)
{
	GPIO_PinOutClear(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	if(value==1)
		GPIO_PinOutSet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
	Delay_temp(60);
	GPIO_PinOutSet(TEMP_SENSORDQPORT,TEMP_SENSORDQPIN);
}

void temp_sensor_writebyte(char value)
{
	unsigned char i,temp;

	for(i=0;i<8;i++)
	{
		temp = value >> i;
		temp &= 0x01;
		temp_sensor_writebit(temp);
	}
	Delay_temp(60);
}

int temp_sensor_gettemp()
{
	char get[10];
	char temp_lsb,temp_msb;
	int temp_c;
	int k;
	for(k=0;k<9;k++)
	{
		get[k] = temp_sensor_readbyte();
	}
	temp_msb = get[1];
	temp_lsb = get[0];
	if (temp_msb <= 0x80){temp_lsb = (temp_lsb/2);} // shift to get whole degree
	temp_msb = temp_msb & 0x80; // mask all but the sign bit
	if (temp_msb >= 0x80) {temp_lsb = (~temp_lsb)+1;} // twos complement
	if (temp_msb >= 0x80) {temp_lsb = (temp_lsb/2);}// shift to get whole degree
	if (temp_msb >= 0x80) {temp_lsb = ((-1)*temp_lsb);} // add sign bit
	temp_c= temp_lsb;
	temp_c=59;
	return temp_c;
}
