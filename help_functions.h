// Functions used by the functions in main_functions.


void modectrl(void);
int check_pin(int PINREG, int PIN);
int read_adc(int ADCPIN);




/* modectrl()

   Ställer max pulsbredd beroende på vilket läge som är valt med
   potentiometern.

   Input: none, Output: none.
*/

void modectrl(void)
{
  if (ADCREADINGS.MODEPOT >= 0 && ADCREADINGS.MODEPOT <= POTLOW)
  {
    maxduty = 255;
  }
  else if (ADCREADINGS.MODEPOT > POTLOW && ADCREADINGS.MODEPOT < POTHIGH)
  {
    maxduty = 255;
  }
  else if (ADCREADINGS.MODEPOT >= POTHIGH)
  {
    maxduty = 255;
  }
}

/////////////////////////////////////////////////////

/* check_pin()
  Kollar om ett ben är logisk hög eller logisk låg.
  Den mäter värdet PINCHECKNUMBER gånger och kollar om den var hög mer 
  än hälften av dessa gånger. I så fall matar den ut att benet är logisk hög
*/

int check_pin(int PINREG, int PIN)      // FIXA!
{
    for (int i = 1; i <= PINCHECKNUMBER; i++)    // Kolla ifall bromshandtaget är intryckt
    {
      BITCHECKCOUNTER += CHECK_BIT(PINREG, PIN);
    }
    if (BITCHECKCOUNTER > PINCHECKNUMBER >> 1)   // Kolla ifall bromshandtaget är intryckt
    {
      return True;
    }
    else
    {
      return False;
    }
    BITCHECKCOUNTER = 0;
} 

/////////////////////////////////////////////////////

/* read_adc()

    Funktion som läser av ADC-värdet på den kanal man anger.
    Tar ett antal värden och bildar ett medelvärde, detta ökar säkerheten hos mätvärdena.

   Input: int, Output: int.
*/

    int read_adc(int ADCPIN)
    {
      switch (ADCPIN)
      {
        case MODEPOT:
      ADMUX |= (1 << MUX2) | (1 << MUX0);     // ADC5 Ska vara MODEPOT
      ADMUX &= ~((1 << MUX3) | (1 << MUX1));
      break;
      case ACCELERATORPOT:
      ADMUX |= (1 << MUX2) | (1 << MUX1);    // ADC6 SKA VARA ACCELERATORPOT
      ADMUX &= ~((1 << MUX3) | (1 << MUX0));
      break;
      case VBAT:
      ADMUX |= (1 << MUX2) | (1 << MUX0) | (1 << MUX1);  // ADC7 Ska vara VBAT
      ADMUX &= ~(1 << MUX3);
      break;
      case SHUNT:
      ADMUX &= ~((1 << MUX3) | (1 << MUX0) | (1 << MUX2) | (1 << MUX1));   // ADC0 Ska vara SHUNT
      break;

    }
    ADCsum = 0;
    for (int i = 1; i <= 8; i++)
    {
      SET_BIT(ADCSRA, ADSC);
      while (CHECK_BIT(ADCSRA, ADSC)); // Vänta på att avläsningen är färdig.
      ADCsum += ADCH;
    }
    return (int(ADCsum >> 3));
  }

  /////////////////////////////////////////////////////


