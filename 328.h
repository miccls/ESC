// 328.h Innehåller kommentarer och set - up av ATMEGA 328P.

void init_328(void); 

void init_328p(void)
{
  init(); // Lägger denna här för att använda arduinos egna grejer!
  Serial.begin(9600);

  ADMUX  |= (1 << REFS0) | (1 << ADLAR);   // Fixar extern referens åt ADC:en och lagrar resultatet i en byte.
  ADCSRA = 0x87;                     // Slår på ADC-mätningar. 0b10000111

  // För styrning av frikopplingstrissor. Initierar utgångar

  CLEAR_BIT(DDRC, DDC2);
  CLEAR_BIT(DDRC, DDC3);
  CLEAR_BIT(DDRC, DDC4);

  CLEAR_BIT(PORTC, PORTC2);
  CLEAR_BIT(PORTC, PORTC3);
  CLEAR_BIT(PORTC, PORTC4);  // Dessa sex rader är de som initierar pinnarna triggar komparatorinterrupterna.

  CLEAR_BIT(DDRC, DDC1);
  SET_BIT(PORTC, PORTC1); // Detta för att förhindra någon slags power ut ifall denna är hög eller ej. Detta är även en ADC kanal vilket innebär att det finns

  DDRD = 0x3C;
  SET_BIT(DDRB, DDB1);
  SET_BIT(DDRB, DDB2);
  CLEAR_BIT(PORTB,PORTB1);
  CLEAR_BIT(PORTB,PORTB2);
  PORTD = 0;  // Sätter utgångsläget på PWM-utgångarna till låg.
    // Sätter tre pinnar till PWM-Out. OC0B OC1B OC1A

  CLEAR_BIT(SREG, I);  // Stänger av interrupts.

  // Nu använder jag timer 2 till timekeeping. om det visar sig bli problem med att ha en pin på ett olägligt ställe.

  TCCR0A |= (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);    // Sätter typ av PWM för OC0B.
  TCCR0B |= (1 << CS01); // (1 << WGM02);
  TCCR1A = 0x01;   // Fixar frekvens och räkning för PWM. Får dock in timer1-PWM:en till 8-bit.
  TCCR1B = 0x09;
  TCCR0B &= ~((1 << CS00) | (1 << CS02));

  // Interrupter för BEMF.

  PCICR = 0x02;      // 0b00000010
  PCMSK1 = 0x04;      // Sätter första på PCINT10. 0b00000100

  // Timekeeping

  TCCR2A |= (1 << WGM21); // CTC läge
  CLEAR_BIT(TCCR2B, CS22);  // Prescaling 64
  SET_BIT(TCCR2B, CS21);
  CLEAR_BIT(TCCR2B, CS20);
  TIMSK2 |= (1 << OCIE2A);// Sätter igång CTC-jämförelse med OCR2A
  TCNT2 = 0;
  OCR2A = 200;

  SET_BIT(SREG, I); // Sätter igång interrupts.

}
