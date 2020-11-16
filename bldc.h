/* bldc.h
Header avsedd för att innehålla alla funktioner som använd för själva kommuteringen av motor.
*/

#include <defines.h>


void bldc_move();
void set_bemf(int phase, int type);
void motor_start(void);
void beep(int time, int freq);
void AH_BL(void);
void AH_CL(void);
void BH_CL(void);
void BH_AL(void);
void CH_AL(void);
void CH_BL(void);

// Nedan följer alla funktionerna i headern separerade med följande rad: 

////////////////////////////////////////////////////////////////

void bldc_move() // Styr var den reglerade pulsbredden styrs ut till gatedrivarna som i sin tur styr effektFET:arna.
{
  switch(bldc_step)   // OCR0B är A, OCR1B är C, OCR1A är B.
  {
    case 0:
    AH_BL();
    set_bemf(C, FALLING);
    break;
    case 1:
    AH_CL();
    set_bemf(B, RISING);
    break;
    case 2:
    BH_CL();
    set_bemf(A, FALLING);
    break;
    case 3:
    BH_AL();
    set_bemf(C, RISING);
    break;
    case 4:
    CH_AL();
    set_bemf(B, FALLING);
    break;
    case 5:
    CH_BL();
    set_bemf(A, RISING);
  }
}

////////////////////////////////////////////////////////////////

void AH_BL() // TCCR1A är för OCR1n, TCCR0A är för OCR0n.
{ 
  TCCR1A = ~((1 << COM1A1) | (1 << COM1B1) | (1 << COM1A0) | (1 << COM1B0)); 
  TCCR0A |= (1 << COM0B1) | (1 << COM0B0); 
  TRANSPORTREG = 0b00010000;
}

////////////////////////////////////////////////////////////////

void AH_CL()
{
  TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1) | (1 << COM1A0) | (1 << COM1B0));
  TCCR0A |= (1 << COM0B1) | (1 << COM0B0); 
  TRANSPORTREG = 0b00001000; // 0x08
}

////////////////////////////////////////////////////////////////

void BH_CL()
{
  TCCR1A |= (1 << COM1A0) | (1 << COM1A1);
  TCCR1A &= ~((1 << COM1B1) | (1 << COM1B0));
  TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0)); 
  TRANSPORTREG = 0b00000100; // 0x04
}

////////////////////////////////////////////////////////////////

void BH_AL()
{
  TCCR1A |= (1 << COM1A0) | (1 << COM1A1);
  TCCR1A &= ~((1 << COM1B1) | (1 << COM1B0));
  TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0)); 
  TRANSPORTREG = 0b00010000;
}

////////////////////////////////////////////////////////////////

void CH_AL()
{
  TCCR1A &= ~((1 << COM1A0) | (1 << COM1A1));
  TCCR1A |= (1 << COM1B1) | (1 << COM1B0);
  TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0)); 
  TRANSPORTREG = 0b00001000; // 0x08
}

////////////////////////////////////////////////////////////////

void CH_BL()
{
  TCCR1A &= ~((1 << COM1A0) | (1 << COM1A1));
  TCCR1A |= (1 << COM1B1) | (1 << COM1B0);
  TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0));
  TRANSPORTREG = 0b00000100; // 0x04
}

////////////////////////////////////////////////////////////////

void ALL_OFF()
{
  TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1) | (1 << COM1A0) | (1 << COM1B0));
  TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0)); 
  TRANSPORTREG = 0b00011100; // 0x1C
}

////////////////////////////////////////////////////////////////

void set_bemf(int phase, int type)
{
  switch (phase)
  {
    case A:
      if (type == RISING)
      {
        PCMSK1 = 0x04;    // Interrupt på pin 0b00000100 vilket blir C2. 
      pin_state = 0x04; // Önskat värde på pinnen för att något ska hända lagras i pinchange och är i Fallingfallet 0.
      }
     else
     {
      PCMSK1 = 0x04;    // Interrupt på pin 0b00000100 vilket blir C2.
      pin_state = 0;
     }
    break;
    case B:
      if (type == RISING)
      {
      PCMSK1 = 0x08;    // Interrupt på pin 0b00001000 vilket blir C3.
      pin_state = 0x08;
      }
      else
      {
        PCMSK1 = 0x08;    // Interrupt på pin 0b00001000 vilket blir C3.
      pin_state = 0;
      }
    break;
    case C:
      if (type == RISING)
      {
        PCMSK1 = 0x10;    // Interrupt på pin 0b00010000 vilket blir C4.
      pin_state = 0x10;
      }
      else
      {
        PCMSK1 = 0x10;    // Interrupt på pin 0b00010000 vilket blir C4.
      pin_state = 0;
      }
      break;
}
}

////////////////////////////////////////////////////////////////

void motor_start(void)   
{
  if (STATUS.motorON || !(pulse_width_reference > 20))   // Kör igenom startsekvens. Lägg in att när motorn stannar så stängs alla externa interrups av så att denna kan gå utan att något händer.
  {
    return;
  }
    SET_PWM((int)PWM_START);
  // CLEAR_BIT(SREG,I); Om det skulle visa sig att den timade interrupten stör. Antingen detta eller min sleep_micros.
    PCICR = 0;
  for (unsigned long i = 2000; i > 500; i -= 20)   // Detta är superfirstdraft. Funkar för den stora. (unsigned long i = 2000; i > 500; i -= 20) 
  { 
    bldc_move();
    bldc_step++;
    bldc_step %= 6; 
   delayMicroseconds(i);      //sleep_micros(i); Funktionen finns och är redo att användas, men jag börjar med arduinos befintliga.
  }

STATUS.motorON = True;
phasetime = 0;
  // SET_BIT(SREG_I);
SET_PWM((int)pulse_width);
  PCICR = 0x02;  //0b00000010
}

////////////////////////////////////////////////////////////////

void beep(int time, int freq) // Ändra denna så att den använder delayMicroseconds().
{
CLEAR_BIT(TCCR2B,CS20);   // Prescaling 8, räknefrekvensen blir 2 MHz.
SET_BIT(TCCR2B,CS21);
CLEAR_BIT(TCCR2B,CS22);
ALL_OFF();
OCR2A = 2;     // Säger åt funktionen att köra interruptfunktionen varannan tick.

period_in_micros = 1000000/freq;       // Perioden i mikrosekunder.
period_number = (time / period_in_micros) >> 1;   // Eftersom den kör två perioder per for - loop.
timer2cnt = 0;
for (int i = 0; i < period_number; ++i)
{
  SET_BIT(PORTB,PORTB2);
  while (timer2cnt < period_in_micros >> 3)
  {
  }
  CLEAR_BIT(PORTB,PORTB2);
  timer2cnt = 0;
  phasetime = 0;
  while (timer2cnt < period_in_micros)
  {
  }
  SET_BIT(PORTB,PORTB1);
  while (timer2cnt < period_in_micros >> 3)
  {
  }
  CLEAR_BIT(PORTB,PORTB1);
  timer2cnt = 0;
  phasetime = 0; 
  while (timer2cnt < period_in_micros)
  {
  }
}

SET_BIT(TCCR2B,CS20);   // Prescaling 8, räknefrekvensen blir 2 MHz.
CLEAR_BIT(TCCR2B,CS21);
SET_BIT(TCCR2B,CS22);
OCR2A = 12;     // Säger åt funktionen att köra interruptfunktionen varannan tick.

}

