#include "xparameters.h"
#include "xgpio.h"
#include "xscutimer.h"
#include "xscugic.h"

XGpio dip, push, leds, ssd;
int gameState = 0;
int type = -1, num;
int action = 0;
int dip_check_prev;

XScuTimer_Config *ConfigPtr;
XScuTimer Timer;
XScuTimer *TimerInstancePtr = &Timer;

XScuGic IntcInstance;
XScuGic *IntcInstancePtr = &IntcInstance;
XScuGic_Config *IntcConfig;

int Init_Peripherals();
int Init_GIC();
int Configure_GIC();
void EnableInts();
void ExceptionInit();
void DisableIntr();

static void Push_Intr_Handler(void *CallBackRef)
{
	XGpio *push_ptr = (XGpio *) CallBackRef;
	int button = XGpio_DiscreteRead(&push, 1);
	XGpio_InterruptDisable(push_ptr, 0xF);
	if (button != 0) {

		xil_printf("Singular Button Press!\r\n");
		switch(gameState) {
			case 0:
				gameState = 1;
				srand(XScuTimer_GetCounterValue(TimerInstancePtr));
				xil_printf("The game begins...\r\n");
				break;
			case 1:
				action = 1;
				if ( type != 0 || (type == 0 && button != 1<<num)) {
					xil_printf("Incorrect!\r\n");
					gameState = 2;
				} else {
					xil_printf("Correct!\r\n");
					XScuTimer_Stop(TimerInstancePtr);
					XScuTimer_ClearInterruptStatus(TimerInstancePtr);
					XGpio_DiscreteWrite(&leds, 1, 0b0000);
					XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
					sleep(1);
				}
		}

	}

	(void)XGpio_InterruptClear(&push, 0xF);
	XGpio_InterruptEnable(&push, 0xF);
}

static void Dip_Intr_Handler(void *CallBackRef)
{
	XGpio *dip_ptr = (XGpio *) CallBackRef;
	XGpio_InterruptDisable(dip_ptr, 0xF);
	int dip_check = XGpio_DiscreteRead(&dip, 1);
	if (dip_check_prev != dip_check) {
		usleep(10000);
		if (XGpio_DiscreteRead(&dip, 1) == dip_check) {
			xil_printf("DIP Switch Flip!\r\n");
			if (gameState == 1) {
				action = 1;
				if ( type != 1 || ( type == 1 && abs(dip_check - dip_check_prev) != 1<<num ) ) {
					xil_printf("Incorrect!\r\n");
					gameState = 2;
				} else {
					xil_printf("Correct!\r\n");
					XScuTimer_Stop(TimerInstancePtr);
					XScuTimer_ClearInterruptStatus(TimerInstancePtr);
					XGpio_DiscreteWrite(&leds, 1, 0b0000);
					XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
					sleep(1);
				}
			}
			dip_check_prev = XGpio_DiscreteRead(&dip, 1);
		}
	}

    (void)XGpio_InterruptClear(dip_ptr, 0xF);
    XGpio_InterruptEnable(dip_ptr, 0xF);
}

void startState() {
	XScuTimer_LoadTimer(TimerInstancePtr, 0xffffffff);
	XScuTimer_EnableAutoReload(&Timer);
	XScuTimer_Start (&Timer);

	XGpio_DiscreteWrite(&leds, 1, 0b0000);
	XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
	xil_printf("Welcome to Reactor Meltdown!\r\n");
	xil_printf("Press any button!\r\n");
	while (!gameState) {
		XGpio_DiscreteWrite(&leds, 1, 0b1111);
		XGpio_DiscreteWrite(&ssd, 1, 0b11111111);
		sleep(1);
		XGpio_DiscreteWrite(&leds, 1, 0b0000);
		XGpio_DiscreteWrite(&ssd, 1, 0b01111111);
		sleep(1);
	}
	XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
	XScuTimer_Stop(TimerInstancePtr);
	XScuTimer_ClearInterruptStatus(TimerInstancePtr);
}

void gameLoop() {
    XGpio_DiscreteWrite(&leds, 1, 0b0000);
    int count = 0;
    while (gameState == 1) {
        type = (rand() + 1) % 2;
        num = rand() % 4;

        switch(count) {
        case 0:
            if (type == 0) {    //Push Buttons
                XGpio_DiscreteWrite(&leds, 1, 0b0011);
                XGpio_DiscreteWrite(&ssd, 1, 0b00111111);
            } else {    //Switch
                XGpio_DiscreteWrite(&leds, 1, 0b1100);
                XGpio_DiscreteWrite(&ssd, 1, 0b00111111);
            }
            break;
        case 1:
            if (type == 0) {    //Push Buttons
                XGpio_DiscreteWrite(&leds, 1, 0b0011);
                XGpio_DiscreteWrite(&ssd, 1, 0b00000110);
            } else {    //Switch
                XGpio_DiscreteWrite(&leds, 1, 0b1100);
                XGpio_DiscreteWrite(&ssd, 1, 0b00000110);
            }
            break;
        case 2:
            if (type == 0) {    //Push Buttons
                XGpio_DiscreteWrite(&leds, 1, 0b0011);
                XGpio_DiscreteWrite(&ssd, 1, 0b01011011);
            } else {    //Switch
                XGpio_DiscreteWrite(&leds, 1, 0b1100);
                XGpio_DiscreteWrite(&ssd, 1, 0b01011011);
            }
            break;
        case 3:
            XGpio_DiscreteWrite(&ssd, 1, 0b01001111);
            xil_printf("YOU WIN!\r\n");
            //XGpio_DiscreteWrite(&ssd, 1, 0b00111111);
            gameState = 2;
            sleep(5);
            exit(0);
        }

        sleep(1);
        XGpio_DiscreteWrite(&leds,1,0b0000);
        XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
        sleep(1);



        switch(num) {
        case 0:
            ++count;
            XGpio_DiscreteWrite(&leds, 1, 0b0001);
            XGpio_DiscreteWrite(&ssd, 1, 0b00000110);
            if (count <= 3)
                xil_printf("score: %d\r\n", count);
            break;
        case 1:
            ++count;
            XGpio_DiscreteWrite(&leds, 1, 0b0010);
            XGpio_DiscreteWrite(&ssd, 1, 0b01011011);
            if (count <= 3)
                xil_printf("score: %d\r\n", count);
            break;
        case 2:
            ++count;
            XGpio_DiscreteWrite(&leds, 1, 0b0100);
            XGpio_DiscreteWrite(&ssd, 1, 0b01001111);
            if (count <= 3)
                xil_printf("score: %d\r\n", count);
            break;
        case 3:
            XGpio_DiscreteWrite(&leds, 1, 0b1000);
            XGpio_DiscreteWrite(&ssd, 1, 0b01100110);

            ++count;
            if (count <= 3)
                xil_printf("score: %d\r\n", count);
            break;
        }
        //0  0  0  0  0  0  0  0
        //     M  TL BL B  BR TR T

        sleep(1);
        XGpio_DiscreteWrite(&leds, 1, 0b0000);
        XGpio_DiscreteWrite(&ssd, 1, 0b00000000);

        XScuTimer_LoadTimer(TimerInstancePtr, 333500000 * 3); //timing
        XScuTimer_Start(TimerInstancePtr);

        while (!XScuTimer_IsExpired(TimerInstancePtr) && !action) {
            //xil_printf("%d\r\n", XScuTimer_GetCounterValue(TimerInstancePtr));
        }
        if (action != 1 && gameState != 2) {
            xil_printf("Ran out of time.\r\n");
            gameState = 2;
        } else {
            action = 0;
        }
    }
}

int main (void)
{
    //0  0  0  0  0  0  0  0
    //	 M  TL BL B  BR TR T

   xil_printf("-- Start of the Program --\r\n");

   Init_Peripherals();
   Init_GIC();
   Configure_GIC();
   EnableInts();
   ExceptionInit();

   startState();
   sleep(1);
   gameLoop();

   DisableIntr();
   XGpio_DiscreteWrite(&ssd, 1, 0b00000000);
   XGpio_DiscreteWrite(&leds, 1, 0b0000);
   xil_printf("GAME OVER!\n");

   while (1) {
	   XGpio_DiscreteWrite(&ssd, 1, 0b01101111);
	   usleep(1);
	   XGpio_DiscreteWrite(&ssd, 1, 0b11101111);
	   usleep(1);
   }
   return 0;
}


int Init_Peripherals()
{

   int xResult = XGpio_Initialize(&dip, XPAR_SWITCHES_DEVICE_ID);
   if (xResult != XST_SUCCESS){
		xil_printf("Dip switch init failed\r\n");
		return XST_FAILURE;
   }
   XGpio_SetDataDirection(&dip, 1, 0xffffffff);

   xResult = XGpio_Initialize(&push, XPAR_BUTTONS_DEVICE_ID);
   if (xResult != XST_SUCCESS){
	   xil_printf("Push buttons init failed\r\n");
	   return XST_FAILURE;
   }
   XGpio_SetDataDirection(&push, 1, 0xffffffff);

   xResult = XGpio_Initialize(&leds, XPAR_LEDS_DEVICE_ID);
   if (xResult != XST_SUCCESS){
	   xil_printf("LEDS init failed\r\n");
	   return XST_FAILURE;
   }
   XGpio_SetDataDirection(&leds, 1, 0xffffffff);

   xResult = XGpio_Initialize(&ssd, XPAR_SSD_DEVICE_ID);
   if (xResult != XST_SUCCESS){
	   xil_printf("SSD init failed\r\n");
	   return XST_FAILURE;
   }
   XGpio_SetDataDirection(&ssd, 1, 0xffffffff);

   ConfigPtr = XScuTimer_LookupConfig (XPAR_PS7_SCUTIMER_0_DEVICE_ID);
   xResult = XScuTimer_CfgInitialize(&Timer, ConfigPtr, ConfigPtr->BaseAddr);
   if (xResult != XST_SUCCESS){
	  xil_printf("Timer init() failed\r\n");
	  return XST_FAILURE;
   }

   dip_check_prev = XGpio_DiscreteRead(&dip, 1);

   return XST_SUCCESS;
}

int Init_GIC()
{
	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
	if(!IntcConfig)
	{
	   xil_printf("Looking for GIC failed!\r\n");
	   return XST_FAILURE;
	}

	int xResult = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	if(xResult != XST_SUCCESS)
	{
	   xil_printf("Init GIC failed!\r\n");
	   return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int Configure_GIC()
{
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, XPAR_FABRIC_SWITCHES_IP2INTC_IRPT_INTR, 0xA0, 0x3);
	int xResult = XScuGic_Connect(IntcInstancePtr,
							XPAR_FABRIC_SWITCHES_IP2INTC_IRPT_INTR,
							(Xil_ExceptionHandler)Dip_Intr_Handler,
							(void *)&dip);
	if(xResult != XST_SUCCESS)
	{
	   xil_printf("Connect dip switch interrupt failed!\r\n");
	   return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, XPAR_FABRIC_BUTTONS_IP2INTC_IRPT_INTR, 0x8, 0x3);
	xResult = XScuGic_Connect(IntcInstancePtr,
							XPAR_FABRIC_BUTTONS_IP2INTC_IRPT_INTR,
							(Xil_ExceptionHandler)Push_Intr_Handler,
							(void *)&push);
	if(xResult != XST_SUCCESS)
	{
	   xil_printf("Connect push button interrupt failed!\r\n");
	   return XST_FAILURE;
	}

	return XST_SUCCESS;
}

void EnableInts()
{
   XScuGic_Enable(IntcInstancePtr, XPAR_FABRIC_SWITCHES_IP2INTC_IRPT_INTR);
   XScuGic_Enable(IntcInstancePtr, XPAR_FABRIC_BUTTONS_IP2INTC_IRPT_INTR);

   XScuGic_CPUWriteReg(IntcInstancePtr, 0x0, 0xf);

	XGpio_InterruptEnable(&dip, 0xF);
    XGpio_InterruptGlobalEnable(&dip);

	XGpio_InterruptEnable(&push, 0xF);
    XGpio_InterruptGlobalEnable(&push);
}

void ExceptionInit()
{
	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
							(Xil_ExceptionHandler)XScuGic_InterruptHandler,
							IntcInstancePtr);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			(void *)IntcInstancePtr);

	Xil_ExceptionEnableMask(XIL_EXCEPTION_ALL);
}

void DisableIntr()
{

	XGpio_InterruptDisable(&dip, 0x3);
	XGpio_InterruptDisable(&push, 0x3);

	XScuGic_Disable(IntcInstancePtr, XPAR_FABRIC_SWITCHES_IP2INTC_IRPT_INTR);
	XScuGic_Disconnect(IntcInstancePtr, XPAR_FABRIC_SWITCHES_IP2INTC_IRPT_INTR);
	XScuGic_Disable(IntcInstancePtr, XPAR_FABRIC_BUTTONS_IP2INTC_IRPT_INTR);
	XScuGic_Disconnect(IntcInstancePtr, XPAR_FABRIC_BUTTONS_IP2INTC_IRPT_INTR);
}
