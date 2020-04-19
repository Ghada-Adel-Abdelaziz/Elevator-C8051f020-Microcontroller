 #include <c8051f020.h> 
//------------------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F02x
//------------------------------------------------------------------------------------
sfr16 DP       = 0x82;                    // data pointer
sfr16 TMR3RL   = 0x92;                    // Timer3 reload value
sfr16 TMR3     = 0x94;                    // Timer3 counter
sfr16 ADC0     = 0xbe;                    // ADC0 data
sfr16 ADC0GT   = 0xc4;                    // ADC0 greater than window
sfr16 ADC0LT   = 0xc6;                    // ADC0 less than window
sfr16 RCAP2    = 0xca;                    // Timer2 capture/reload
sfr16 T2       = 0xcc;                    // Timer2
sfr16 RCAP4    = 0xe4;                    // Timer4 capture/reload
sfr16 T4       = 0xf4;                    // Timer4
sfr16 DAC0     = 0xd2;                    // DAC0 data
sfr16 DAC1     = 0xd5;                    // DAC1 data
//------------------------------------------------------------------------------------
int f=0;
int ir1F=0; //IR sensor 1 flag
int angle=0;
int ir2F=0; //IR sensor 2 flag
int chk1=0; //IR sensor 1 additional check flag variable to detect high to low edge
int chk2=0; //IR sensor 2 additional check flag variable to detect high to low edge
int persons=0; //number of persons inside the elevator
int doorInt=0; //variable to count 5 seconds for door opening
int moving=0; //moving=0 means elevator is not moving
int floor=0; //current floor number
int direction=1; //direction=1 means elevator is going up
int upRequests[5] = {0,0,0,0,0}; //array to save the going-up requests from different floors
int downRequests[5] = {0,0,0,0,0}; //array to save the going-down requests from different floors
int floorRequests[5] = {0,0,0,0,0}; //array to save the floor requests inside the elevator
sbit buzzer = P0^3;
sbit LED = P2^4; //LED=0 means door is closed
sbit ir1 = P2^5;
sbit ir2 = P2^6;
sbit servo=P2^7;
//Buttons variables
sbit door = P0^2;
sbit floor0 = P0^1;
sbit floor0Up = P2^0;
sbit floor1Up = P2^1;
sbit floor2Up = P2^2;
sbit floor3Up = P2^3;
sbit floor1Down = P0^4;
sbit floor2Down = P0^5;
sbit floor3Down = P0^6;
sbit floor4Down = P0^7;
//-------------------------------------------------------------------------------------

void delay(void)  // Function for creating a very short delay
{
    unsigned i;  
    for(i=0;i<200;i++);
}

void msdelay(unsigned int time)  // Function for creating delay in milliseconds.
{
    unsigned i,j ;
    for(i=0;i<time;i++)    
    for(j=0;j<1250;j++);
}

void Timer3_Init(void)
{
    TMR3CN = 0x00;                      // Stop Timer3; Clear TF3;
                                        // use SYSCLK/12 as timebase
    TMR3RL = 0x00;                  // Init reload values
    TMR3 = 0xffff;                   // set to reload immediately
    EIE2 |= 0x01;                     // enable Timer3 interrupts
    TMR3CN |= 0x04;                     // start Timer3
}

void Timer3_ISR (void) interrupt 14
{
   TMR3CN &= ~(0x80);                     // clear TF3
   doorInt = doorInt+1;
   if(doorInt==145)
   {
       LED = 0;
       moving = 1;
   }
}

void servo_delay(int times)
{
    int m;
    for(m=0;m<times;m++)
    {
        TH0=0xFF;
        TL0=0xD2;
        TR0=1;
        while(TF0==0);
        TF0=0;
        TR0=0;
    }
}

void openDoor(void) reentrant
{   
    if(door==0) openDoor();
    LED = 1;
	doorInt = 0;
    Timer3_Init();
}

void countPersons(void)
{
	if(ir1==0)
	{
		openDoor();
		if(ir2F==1)
		{
			chk1=1;
			delay();
		}
		else	ir1F = 1;
	}
	else
	{
		if(chk1)
		{
			persons = persons-1;
			chk1=0;
			ir2F=0;
		}
	}
	
	if(ir2==0)
	{
		if(ir1F==1)
		{
			chk2=1;
			delay();
		}
		else	ir2F = 1;
	}
	else
	{
		if(chk2)
		{
			persons = persons+1;
			chk2=0;
			ir1F=0;
		}
	}
	
	if(persons>4)
	{
		buzzer = 1;
		openDoor();
	}
	else	buzzer = 0;
}

void saveRequests(void)
{
    if(!floor0Up)   upRequests[0]=1;
    if(!floor1Up)   upRequests[1]=1;
    if(!floor2Up)   upRequests[2]=1;
    if(!floor3Up)   upRequests[3]=1;
    if(!floor1Down)   downRequests[1]=1;
    if(!floor2Down)   downRequests[2]=1;
    if(!floor3Down)   downRequests[3]=1;
    if(!floor4Down)   downRequests[0]=1;
    if(!(P5&0x01))  floorRequests[1]=1;
    if(!(P5&0x02))  floorRequests[2]=1;
    if(!(P5&0x04))  floorRequests[3]=1;
    if(!(P5&0x08))  floorRequests[4]=1;
    if(!(floor0))  floorRequests[0]=1;
}

int nextStop(void)
{
    int stop=-1;
    int i=2;
    if(direction)
    {
        stop=4;
        i = floor+1;
        while(i<4)
        {
            if(floorRequests[i] || upRequests[i])
            {
                stop=i;
                break;
            }
            else{
                floorRequests[4]=1;
                i++;
            }
        }
    }
    else
    {
        stop=0;
        i=floor-1;
        while(i>0)
        {
            if(floorRequests[i] || downRequests[i])
            {
                stop=i;
                break;
            }
            else
            {
                floorRequests[0]=1;
                i--;
            }
        }
    }
    return stop;
}

void displayFloor(void)
{
    if(floor==0)    P1 = 0xBF;
    else if(floor==1)   P1 = 0x86;
    else if(floor==2)   P1 = 0xDB;
    else if(floor==3)   P1 = 0xD7;
    else if(floor==4)   P1 = 0xE6;
}

void main(void)
{
    //configure clock
    OSCXCN =0x67; 
    while (!(OSCXCN & 0x80)); // Wait till XTLVLD ( bit no 7 ) pin is set (crystal is stable)
    OSCICN = 0x88;
    
    // disable watchdog timer
    WDTCN = 0xde;
    WDTCN = 0xad;
    
    XBR2 = 0x40; //enable pins output by crossbar
    EA = 1; //set global interrupt bit
    TMOD=0x01; // Selecting Timer 0, Mode 1

    P2MDOUT |= 0x90;
    P0MDOUT |= 0x08;
    P1MDOUT |= 0x7F;
	servo=0;
	buzzer=0;
	LED=0;
    P1 = 0xBF; //7-segment =0
    while(1)
    {
        displayFloor();
        saveRequests();
        if(LED==0 && moving==0 && (upRequests[floor] || downRequests[floor] || floorRequests[floor]))
        {
            floorRequests[floor]=0;
            if(direction)   upRequests[floor]=0;
            else    downRequests[floor]=0;
            openDoor();
        }
		if(LED==1)	countPersons();
        msdelay(200);
        if(moving==1)
        {
            int stop = nextStop();
            msdelay(10);
            servo=1;
		    servo_delay(25+stop*18);
		    servo=0;
            moving=0;
            floor=stop;
            if(floor==4)    direction=0;
            else if(floor==0)    direction=1;
        }
    }
}
