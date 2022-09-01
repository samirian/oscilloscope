#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv.h"

#define F_CPU 12000000L
#include <util/delay.h>

#define USB_DATA_OUT 2
#define ECHO 3
#define USB_DATA_IN 4
#define SAMPLE_COUNT 50

static uchar replyBuf[SAMPLE_COUNT + 1];
static uchar dataReceived = 0, dataLength = 0; // for USB_DATA_IN
static volatile uint8_t counter = 0;
static volatile uint8_t timer = 0;
static volatile uint8_t sample[SAMPLE_COUNT];

void ADC_init(){
	ADMUX = 0b01100000;    // PA0 -> ADC0, ADLAR=1 (8-bit)
	ADCSRA |= ((1<<ADEN) | (1<<ADSC) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0)); // ADC prescaler at 4

}

uint8_t getADCSample(void){
	uint8_t ADCvalue = 0;
	ADCSRA |= (1 << ADSC);
	while(!(ADCSRA & (1<<ADIF))); // waiting for ADIF, conversion complete
	ADCvalue = ADCH;
	return ADCvalue;
}

ISR(TIMER1_COMPA_vect){
	if(counter < SAMPLE_COUNT){
		sample[counter] = getADCSample();
		counter++;
	}
}

void init_timer1(uint8_t prescaler, uint16_t ocr1a){
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1<<WGM12);
	switch(prescaler){
		case 0:
			break;
		case 1:
			TCCR1B |= (1<<CS10);//CTC no prescaler
			break;
		case 2:
			TCCR1B |= (1<<CS11);//CTC no prescaler
			break;
		case 3:
			TCCR1B |= (1<<CS11) | (1<<CS10);//CTC no prescaler
			break;
		case 4:
			TCCR1B |= (1<<CS12);//CTC no prescaler
			break;
		case 5:
			TCCR1B |= (1<<CS12) | (1<<CS10);//CTC no prescaler
			break;
	}
	uint8_t serg = SREG;
	cli();
	TCNT1 = 0;
	OCR1A = ocr1a;
	SREG = serg;
	TIMSK |= (1<<OCIE1A);
	sei();
}

// this gets called when custom control message is received
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *)data; // cast data to correct type
	uint8_t sreg;
	switch(rq->bRequest) { // custom command is in the bRequest field
	    case USB_DATA_OUT: // send data to PC
	    	if (counter == SAMPLE_COUNT){
				replyBuf[0] = SAMPLE_COUNT;
				uchar i;
		    	for(i = 0 ; i < SAMPLE_COUNT ;i++){
			    	replyBuf[i + 1] = sample[i];
			    }
	        	usbMsgPtr = replyBuf;
			    counter = 0;
			    return SAMPLE_COUNT + 1;
			}
			replyBuf[0] = 0;
	        usbMsgPtr = replyBuf;
	    	return 1;
	    case ECHO:
			sreg = SREG;
			cli();
			replyBuf[0] = (uint8_t)((OCR1A & 0xFF00) >> 8);
			replyBuf[1] = (uint8_t)(OCR1A & 0x00FF);
			replyBuf[2] = 'D';
			SREG = sreg;
        	usbMsgPtr = replyBuf;
	    	return 3;
	    case USB_DATA_IN: // receive data from PC
			dataLength  = (uchar)rq->wLength.word;
	        dataReceived = 0;
			
			if(dataLength  > sizeof(replyBuf)) // limit to buffer size
				dataLength  = sizeof(replyBuf);
			return USB_NO_MSG; // usbFunctionWrite will be called now
    }
    return 0; // should not get here
}
// This gets called when data is sent from PC to the device
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
	uchar i;
	for(i = 0; dataReceived < dataLength && i < len; i++, dataReceived++){
		replyBuf[dataReceived] = data[i];
	}
	init_timer1(replyBuf[0], (replyBuf[1] + ( (replyBuf[2]) << 8) )  );
    return (dataReceived == dataLength); // 1 if we received it all, 0 if not
}

int main() {
	uchar i;
	DDRC = 0x00;
    wdt_enable(WDTO_1S); // enable 1s watchdog timer
	ADC_init();
    init_timer1(1, 0xffff);
    usbInit();
	
    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }

    sei(); // Enable interrupts after re-enumeration
    usbDeviceConnect();
    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }
    return 0;
}