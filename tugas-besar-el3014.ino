enum States
{
    CLOCK,
    CLOCK_RUNNING,
    CLOCK_SET_HOURS,
    CLOCK_SET_MINUTES,
    CLOCK_SET_SECONDS,
    ALARM,
    STOPWATCH,
};

volatile int segEnb = 0;
volatile int hours = 0, minutes = 0, seconds = 0;
volatile int alarmHours = 0, alarmMinutes = 0;
volatile int stopwatchMinutes = 0, stopwatchSeconds = 0;
volatile int d1, d2;
volatile States state = CLOCK;
volatile States substate = CLOCK_RUNNING;
volatile boolean isStopwatchFreeze = true;
volatile boolean clockHoursMinutes = false;
volatile boolean isAlarmEnabled = false;
volatile int b1PressedBefore = LOW, b2PressedBefore = LOW;
volatile int b1PressedNow = LOW, b2PressedNow = LOW;
volatile int b1Pressed = LOW, b2Pressed = LOW;

int num_array[10][8] = {
    {1, 1, 1, 1, 1, 1, 0, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1, 0}, // 2
    {1, 1, 1, 1, 0, 0, 1, 0}, // 3
    {0, 1, 1, 0, 0, 1, 1, 0}, // 4
    {1, 0, 1, 1, 0, 1, 1, 0}, // 5
    {1, 0, 1, 1, 1, 1, 1, 0}, // 6
    {1, 1, 1, 0, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1, 0}, // 8
    {1, 1, 1, 0, 0, 1, 1, 0}, // 9
};

void setup()
{
    // Seven Segments pin a - dp
    for (int i = 5; i < 13; i++)
    {
        pinMode(i, OUTPUT);
    };

    // Seven segment enable
    for (int i = A0; i < A4; i++)
    {
        pinMode(i, OUTPUT);
    }

    // Pushbuttons
    attachInterrupt(digitalPinToInterrupt(2), changeState, RISING);
    pinMode(4, INPUT);
    pinMode(3, INPUT);

    // Buzzer
    pinMode(0, OUTPUT);

    // RGB LED
    pinMode(A4, OUTPUT);
    pinMode(A5, OUTPUT);
    pinMode(1, OUTPUT);

    // Timer Interrupt Settings
    cli();

    // Timer 1 (~1Hz)
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    OCR1A = 15624;
    TIMSK1 |= (1 << OCIE1A);

    // Timer 2 (multiplexer, ~60Hz)
    TCCR2A = 0;
    TCCR2B = 0;
    TCCR2B |= (1 << WGM12);
    TCCR2B |= (1 << CS12) | (1 << CS10);
    OCR2A = 0xff;
    TIMSK2 |= (1 << OCIE2A);

    sei();
}

void loop()
{
    digitalWrite(0, (hours == alarmHours && minutes == alarmMinutes && seconds < 6 && seconds % 2) ? HIGH : LOW);
    b1PressedBefore = b1PressedNow;
    b2PressedBefore = b2PressedNow;
    b1PressedNow = digitalRead(4);
    b2PressedNow = digitalRead(3);
    b1Pressed = b1PressedBefore == LOW && b1PressedNow == HIGH;
    b2Pressed = b2PressedBefore == LOW && b2PressedNow == HIGH;

    switch (state)
    {
    case CLOCK:
        digitalWrite(1, HIGH);
        digitalWrite(A4, LOW);
        digitalWrite(A5, LOW);
        digitalClockBehavior();
        break;

    case ALARM:
        digitalWrite(1, LOW);
        digitalWrite(A4, HIGH);
        digitalWrite(A5, LOW);
        alarmBehavior();
        break;

    case STOPWATCH:
        digitalWrite(1, LOW);
        digitalWrite(A4, LOW);
        digitalWrite(A5, HIGH);
        stopwatchBehavior();
        break;
    }
}

void changeState()
{
    switch (state)
    {
    case CLOCK:
        state = ALARM;
        substate = CLOCK_RUNNING;
        break;
    case ALARM:
        state = STOPWATCH;
        break;
    case STOPWATCH:
        state = CLOCK;
        break;
    }
}

void digitalClockBehavior()
{
    switch (substate)
    {
    case CLOCK_RUNNING:
        if (clockHoursMinutes)
        {
            d1 = minutes;
            d2 = seconds;
        }
        else
        {
            d1 = hours;
            d2 = minutes;
        }
        if (b1Pressed)
            clockHoursMinutes = !clockHoursMinutes;
        if (b2Pressed)
        {
            d1 = 0;
            d2 = 0;
            substate = CLOCK_SET_HOURS;
        }
        break;

    case CLOCK_SET_HOURS:
        d1 = 1;
        d2 = hours;
        if (b1Pressed)
            hours = hours < 23 ? hours + 1 : 0;
        if (b2Pressed)
            substate = CLOCK_SET_MINUTES;
        break;

    case CLOCK_SET_MINUTES:
        d1 = 2;
        d2 = minutes;
        if (b1Pressed)
            minutes = minutes < 59 ? minutes + 1 : 0;
        if (b2Pressed)
            substate = CLOCK_SET_SECONDS;
        break;

    case CLOCK_SET_SECONDS:
        d1 = 3;
        d2 = seconds;

        if (b1Pressed)
            seconds = seconds < 58 ? seconds + 1 : 0;
        if (b2Pressed)
            substate = CLOCK_RUNNING;
        break;
    }
}

void alarmBehavior()
{
    d1 = alarmHours;
    d2 = alarmMinutes;

    if (b1Pressed)
        alarmHours = alarmHours < 23 ? alarmHours + 1 : 0;
    if (b2Pressed)
        alarmMinutes = alarmMinutes < 59 ? alarmMinutes + 1 : 0;
}

void stopwatchBehavior()
{
    d1 = stopwatchMinutes;
    d2 = stopwatchSeconds;
    if (b2Pressed)
        isStopwatchFreeze = !isStopwatchFreeze;
    if (b1Pressed)
    {
        stopwatchMinutes = 0;
        stopwatchSeconds = 0;
        isStopwatchFreeze = true;
    }
}

// ISR COUNTER
ISR(TIMER1_COMPA_vect)
{
    if (substate == CLOCK_RUNNING)
    {
        seconds++;
        if (seconds >= 60)
        {
            seconds = 0;
            minutes++;
            if (minutes >= 60)
            {
                minutes = 0;
                hours++;
                if (hours >= 24)
                    hours = 0;
            }
        }
    }

    if (!isStopwatchFreeze)
    {
        stopwatchSeconds++;
        if (stopwatchSeconds >= 60)
        {
            stopwatchSeconds = 0;
            stopwatchMinutes++;
            if (stopwatchMinutes >= 100)
            {
                stopwatchSeconds = 0;
                stopwatchMinutes = 0;
            }
        }
    }
}

// ISR MULTIPLEXING
ISR(TIMER2_COMPA_vect)
{
    for (int i = 0; i < 4; i++)
    {
        if (i == segEnb)
        {
            PORTC ^= (1 << i);
        }
        else
        {
            PORTC &= ~(1 << i);
        }
    }

    int numToDisplay = 0;
    switch (segEnb)
    {
    case 0: // digit puluhan jam
        numToDisplay = d1 / 10;
        break;
    case 1: // digit satuan jam
        numToDisplay = d1 % 10;
        break;
    case 2: // digit puluhan detik
        numToDisplay = d2 / 10;
        break;
    default: // digit puluhan detik
        numToDisplay = d2 % 10;
        break;
    }

    for (int i = 0; i < 10; i++)
    {
        if (i < 3)
        {
            if (num_array[numToDisplay][i])
            {
                PORTD |= (1 << (i + 5));
            }
            else
            {
                PORTD &= ~(1 << (i + 5));
            }
        }
        else
        {
            if (num_array[numToDisplay][i])
            {
                PORTB |= (1 << (i - 3));
            }
            else
            {
                PORTB &= ~(1 << (i - 3));
            }
        }
    }

    if (segEnb != 3)
    {
        segEnb++;
    }
    else
    {
        segEnb = 0;
    }
}