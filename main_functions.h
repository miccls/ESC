// Innehåller de stora funktionerna som löper kontinuerligt i main().

#include <defines.h>

void regulator(void);
void statemachine(void); 
void ADCupdate(void);
void esc_start(void);


/* regulator()

   Sköter PWM-regleringen för att hålla utströmmen under strömgräns
   och kompensera för att batterispänningen sjunker under hög last.

*/

void regulator(void)        // Funderar på att i framtiden ändra denna till ett bestämt varvtal.
{
  pulse_width_reference = ADCREADINGS.ACCELERATOR;    // Fixade en macro som ersätter map() här i arduinos mjukvara.
  if (!STATUS.regen)
  {
    if (ADCREADINGS.BAT <= BATREFERENCE)
    {
      SCALEFACTOR = (BATREFERENCE / ADCREADINGS.BAT);
    }
    else
    {
      SCALEFACTOR = 1;
    }
    // Regulator som jobbar med kvadraten på felet.
    if (REALREADINGS.SHUNT > IMAX)
    {
      errorterm += (REALREADINGS.SHUNT - IMAX) / ERRORCOEFF;
      pulse_width -= errorterm + SQUARED((REALREADINGS.SHUNT - IMAX)) / REGCOEFF;
    }
    else
    {
      if (pulse_width < pulse_width_reference * SCALEFACTOR)
      {
        errorterm =+ (pulse_width_reference * SCALEFACTOR - pulse_width) / ERRORCOEFF;
        if ((ADCREADINGS.SHUNT - IMAX < 
        	pulse_width_reference * SCALEFACTOR - pulse_width) && ADCREADINGS.SHUNT > 30)
        {
          pulse_width += errorterm + SQUARED((ADCREADINGS.SHUNT - IMAX)) / REGCOEFF;
        }
        else 
        {
          pulse_width += errorterm + 
          	SQUARED(pulse_width - pulse_width_reference * SCALEFACTOR) / REGCOEFF;
        }
      }
      else if (pulse_width > pulse_width_reference * SCALEFACTOR)
      {
        errorterm -= (pulse_width_reference * SCALEFACTOR - pulse_width) / ERRORCOEFF;
        pulse_width -= errorterm + 
        	SQUARED(pulse_width - pulse_width_reference * SCALEFACTOR) / REGCOEFF;
      }
    }
  }
  else if (STATUS.regen)  // Här lägger vi in en variant som reglerar bromsningen, den kommer vara ruskigt lik det ovan.
  {
    if (REALREADINGS.SHUNT > IREGMAX)
    {
      pulse_width += SQUARED((ADCREADINGS.SHUNT - IREGMAX)) / REGCOEFF;    // Tanken här är att reglera på ström först, sedan på skillnad i effekti och inducerad spänning
    }
    else if (REALREADINGS.SHUNT - IREGMAX < V_BEMF - V_EFF)       // VI har ett problem, här tas ingen hänsyn till åkarens input. Den måste behandlas!
    { // Tanken är att V_EFF kanske kan låtas bildas från pulse_width_reference.
      pulse_width -= SQUARED((ADCREADINGS.SHUNT - IREGMAX)) / REGCOEFF;
    }
    else
    {
      pulse_width -= SQUARED(V_BEMF - V_EFF) / (REGCOEFF << 1);
    }
  }

  // Clamp

if (pulse_width > maxpwm)
{
  pulse_width = maxpwm;
}
else if (pulse_width < 0)
{
  pulse_width = 0;
}

  if (ABS(REALREADINGS.SHUNT - IMAX) < 0.5 or 
  	ABS(pulse_width_reference * SCALEFACTOR - pulse_width) < 0.5)   // Nolla feltermen
  {
    errorterm = 0;
  }

  SET_PWM((int)pulse_width);
}

/////////////////////////////////////////////////////

/*  statemachine()

    Håller koll så att allt utförs på den tid det ska.
    Den snabba räknaren exekveras varje 10 ms och den
    långsamma körs varje sekund.
*/

void statemachine(void)
{
  if (timer2cnt >= fastcompare)
  {
    STATUS.fastflag = True;
    slow_timer_cnt++;
    medium_timer_cnt++;
    timer2cnt = 0;
    phasetime++;
  }

  if (medium_timer_cnt >= mediumcompare)
  {                                                                           // Här fixade jag så att den inte dividerar med 0, det gjorde den innan, KATASTROF.
    //(OldPhasetime != 0) ? RPM = RPMSCALEFACTOR / OldPhasetime : RPM = 0;      // Varvtalet på motorn tror jag. Måste prova så att det stämmer. Tanken är (60/tiden)
    //V_BEMF = RPM / KV;                                                // Här provar jag att beräkna BEMFen över motorns lindor, vi får se om det fungerar.
    //V_EFF = SCALE(REALREADINGS.BAT, pulse_width_reference, BYTE);     // Tar produken här mellan D och batterispänningen för att se om den effektiva spänningen som kommer ut är
    //(V_BEMF > V_EFF) ? STATUS.regen = True : STATUS.regen = False;    // Kollar ifall man kan börja återladda genom regenerativ bromsning eller om vi vill öka hastigheten.
    STATUS.breakON = check_pin(PINC,PINC1);

  }

  if (ADCREADINGS.SHUNT > IEMERGENCY) 
  {
    SET_PWM(0);
    timer2cnt = 0;
    while (True)
    {
      phasetime = True;
      delay(10000); // 1s.
      if (phasetime)
      {
        goto motor_stopped;
      }
    }
  }

  if (slow_timer_cnt >= slowcompare) // Kod som kör en gång per sekund.
  {
    modectrl();            // Ställer in läget beroende värdet hos ADCREADINGS.MODEPOT
    slow_timer_cnt = 0;
    STATUS.potsampleflag = True;    
  }

  if (phasetime > 100)          // Kolla ifall motorn stått stilla länge.
  {
    motor_stopped:
      phasetime = 101;
      ALL_OFF();
  }

  
  if (REALREADINGS.SHUNT < NO_LOAD && REALREADINGS.BAT < VMIN && 0)
  {
    ALL_OFF();
    while (True)
    {}
  }

}

/////////////////////////////////////////////////////

/* ADCupdate()

    Uppdaterar all ADCvärden som används i koden.
    Input: none, Output: none.

*/

void ADCupdate(void)
{

  ADCREADINGS.BAT = read_adc(VBAT);
  ADCREADINGS.ACCELERATOR = read_adc(ACCELERATORPOT);
  ADCREADINGS.SHUNT = read_adc(SHUNT);

  // Omvandling för shuntspänningen till ampere och för VBAT till volt. Jag har tänkt lägga till en check att när I_out är 0 så kollar man batterispänningen för att ge en procentsats på laddningen.

  REALREADINGS.SHUNT = ADCREADINGS.SHUNT; // Här behöver man inte göra något eftersom ADC är 0 - 255 och REAL är 0 - 250. I princip samma.
  REALREADINGS.BAT = (ADCREADINGS.BAT * 50) >> 8;
  // Behöver inte läsas lika ofta

  if (STATUS.potsampleflag)
  {
    ADCREADINGS.MODEPOT = read_adc(MODEPOT);
    STATUS.potsampleflag = False;
  }
}

void esc_start(void)
{
  STATUS.allflags = False; // Sätter alla flags till noll
  STATUS.fastflag = True;  // Ser till att vi får värden från ADC:en direkt
  all_off();
  bldc_step = AB;
  set_bemf(A, RISING);

  // Senare, när vi är längre i projektet kan vi här lägga in 
  // en check som mäter batterispänningen olastat så att vi initialt kan få en idé om 
  // scooterns laddningsstatus.

}
