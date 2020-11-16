/* Kod till huvuddator på BLDC-fartreglage, denna kod är skriven för en Atmel ATMega328p.

Skriven av: Martin Svärdsjö.
Alla konstanta termer är i caps, exempelvis IMAX. Alla variabler är 
små bokstäver, exempelvis pulse_width. 
Alla funktioner med stora bokstäver är så kallade macros och de med små är vanliga funktioner.
Alla variabel deklarationer och alla definitioner av macros, konstanter och macros finns i headern defines.h
Alla funktioner som inte ingår explicit i main() finns i auxfunctions.h, resten finns här.

// UPPTAGNA PINS: PD9, PB1, PB2, PC5, ADC6, PB6, PB7, PC2, PC3, PC4, ADC7, PD4, PD2, PD3, PC0, PC1,

*/
#include <avr/io.h>   //Detta är så alla registerkommandon fungerar.
#include <defines.h>  // Definierar variabler
#include <help_functions.h> // Innehåller hjälpfunktioner som används i följande moduler.
#include <bldc.h> // Innehåller de funktioner som sköter kommuteringen av motorn.
#include <328.h> // Innehåller en funktion, init_328p(), som fixar alla register på ATMEGA 328P för drift av fartreglaget.
#include <main_functions.h> // Innehåller regulator, statemachine och ADC - läsaren. De viktiga funktioner som körs hela tiden.

/* main()

   Här samlar jag kodens alla funktioner. Godis för att hålla koll på allt.
   Vi går in i alla funktioner men det är inte säkert att de exekveras, det finns
   ett if-statement i varje som kolla ifall det är dags.

   Input: none, Output: none.
*/

//////////////////////////////////////////////////////

int main()
{
  init_328p();   // Initierar alla räknare, mätningar, interrupts och timers i processorn.
  esc_start();   // Allt som ska göras när fartreglaget startas läggs i denna funktion.
  while (True)
  {
    statemachine(); // Uppdaterar alla flaggor. Bestämmer vad som ska hända och när det ska hända.
    //motor_start();  // Kickar igång motorn ifall den stannat. Gör denna till kommentar ifall man vill dra igång manuellt.
    if (STATUS.fastflag)
    {
      ADCupdate();    // Uppdaterar mätvärden som används i koden.
      regulator();    // Reglerar pulsbredden med avseende på en del olika parametrar.
      STATUS.fastflag = False;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////

/* ISR()

  Här är de två interruptkoder som är aktiva i processorn. ISR(TIMER2_COMPA_vect) är en
  time-keeping interrupt som går på processorns klocka.
  Den andra, ISR(PCINT1_vect), är en funktion som processorn flaggar för en extern interrupt.
  Den körs när komparatorerna som är kopplade till motorn för att känna BEMF:en går höga i den
  ordning de ska. Varje gång den kör så byter den processorben att känna på.

   Input: Interrupt flag, Output: none.
*/

ISR(TIMER2_COMPA_vect) // Glömt hastigheten, får kolla upp detta.
{
  timer2cnt++;   // För att räkna varvtal, får se om det funkar, dvs att det inte går för fort.
}

ISR (PCINT1_vect)
{
  if((PINC & PCMSK1) != pin_state)   // Om det inte är den önskade pinchange:en så hoppar vi över tills den kommer.
  {
    return;
  }
  for(byte i = 0; i < DEBOUNCE_COUNT; i++)
  {
    if(bldc_step & 1)
    {
      if(PINC & PCMSK1) i -= 1;    // PCMSK1 är registret som lagrar vilka pinnar som har interrupt. Detta är en AND mellan den och logiska läget på pinnen som är aktiverad. 00001000 & 01011010 = 00001000.
    }
    else
    {
      if(!(PINC & PCMSK1))  i -= 1;
    }
  }
  phasetime = 0;
  bldc_move(); // Byter läge
  bldc_step++; // Ökar step för nästa gång det ska bytas
  bldc_step %= RESTART;  // Sätter step till 1 ifall det blir 7 i föregående line.
}