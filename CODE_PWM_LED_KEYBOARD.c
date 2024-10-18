#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_ftm.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definiciones
 ******************************************************************************/
#define PWM_FTM_INSTANCE FTM3
#define PWM_FTM_CHANNEL_NUM  kFTM_Chnl_4

/* Configuración de la interrupción */
#define FTM_INTERRUPT_NUMBER   FTM3_IRQn
#define FTM_INTERRUPT_HANDLER  FTM3_IRQHandler

/* Flags de interrupciones */
#define FTM_CHANNEL_INTERRUPT_ENABLE kFTM_Chnl4InterruptEnable
#define FTM_CHANNEL_INTERRUPT_FLAG   kFTM_Chnl4Flag

/* Fuente de reloj para el FTM */
#define FTM_CLOCK_SOURCE CLOCK_GetFreq(kCLOCK_CoreSysClk)

#define NUM_ROWS 4U
#define NUM_COLS 4U

#define IS_DIGIT_KEY(k) ((k) >= '0' && (k) <= '9')

/*******************************************************************************
 * Prototipos
 ******************************************************************************/
void SimpleDelay(void);
void ActivatePWM(void);
void DeactivatePWM(void);
void AdjustDutyCycle(uint8_t percentage);
void StateManager(void);
char ScanKeypad(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile bool ftmInterruptTriggered = false;
volatile bool increaseBrightnessFlag = true;
volatile uint8_t activeDutyCycle = 10U;

ftm_config_t pwmConfig;
ftm_chnl_pwm_signal_param_t pwmSignalParams;
ftm_pwm_level_select_t pwmOutputLevel = kFTM_LowTrue;

typedef enum {
    IDLE_STATE = 0,
    PWM_ACTIVE_STATE,
    DUTY_CYCLE_ENTRY_STATE
} FSM_State_t;

FSM_State_t fsmState;

char inputBuffer[3] = {0}; /* Para almacenar dos dígitos y el carácter nulo */
uint8_t inputIndex;
volatile uint8_t pwmDutyCycle = 0;

char currentKeyPress;
const char keypadMapping[NUM_ROWS][NUM_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

const uint8_t rowPins[NUM_ROWS] = {12U, 13U, 14U, 15U};
const uint8_t columnPins[NUM_COLS] = {11U, 12U, 13U, 14U};

/*******************************************************************************
 * Código
 ******************************************************************************/

int main(void)
{
    /* Inicialización de los pines, reloj y consola de depuración */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    fsmState = IDLE_STATE;
    inputIndex = 0;

    while(1) {
        currentKeyPress = ScanKeypad();
        StateManager(); /* Gestiona la lógica de la máquina de estados */
    }

    return 0;
}

/* Activa el PWM con la configuración actual */
void ActivatePWM(void) {
    /* Configura el PWM a 24kHz */
    pwmSignalParams.chnlNumber = PWM_FTM_CHANNEL_NUM;
    pwmSignalParams.level = pwmOutputLevel;
    pwmSignalParams.dutyCyclePercent = pwmDutyCycle;
    pwmSignalParams.firstEdgeDelayPercent = 0U;
    pwmSignalParams.enableDeadtime = false;

    FTM_GetDefaultConfig(&pwmConfig);
    FTM_Init(PWM_FTM_INSTANCE, &pwmConfig);

    FTM_SetupPwm(PWM_FTM_INSTANCE, &pwmSignalParams, 1U, kFTM_CenterAlignedPwm, 24000U, FTM_CLOCK_SOURCE);

    FTM_StartTimer(PWM_FTM_INSTANCE, kFTM_SystemClock);
}

/* Detiene el PWM */
void DeactivatePWM(void) {
    FTM_UpdateChnlEdgeLevelSelect(PWM_FTM_INSTANCE, PWM_FTM_CHANNEL_NUM, 0U);
}

/* Ajusta el ciclo de trabajo del PWM */
void AdjustDutyCycle(uint8_t duty) {
    FTM_UpdateChnlEdgeLevelSelect(PWM_FTM_INSTANCE, PWM_FTM_CHANNEL_NUM, 0U);
    FTM_UpdatePwmDutycycle(PWM_FTM_INSTANCE, PWM_FTM_CHANNEL_NUM, kFTM_CenterAlignedPwm, duty);
    FTM_SetSoftwareTrigger(PWM_FTM_INSTANCE, true);
    FTM_UpdateChnlEdgeLevelSelect(PWM_FTM_INSTANCE, PWM_FTM_CHANNEL_NUM, pwmOutputLevel);
    SimpleDelay();
}

/* Retardo simple */
void SimpleDelay(void)
{
    volatile uint32_t delayCount = 0U;
    for (delayCount = 0U; delayCount < 8000U; ++delayCount)
    {
        __asm("NOP");
    }
}

/* Máquina de estados Mealy */
void StateManager(void) {
    switch(fsmState) {
        case IDLE_STATE:
            if (currentKeyPress == 'A') {
                ActivatePWM();

                pwmDutyCycle = 50;
                AdjustDutyCycle(pwmDutyCycle);
                fsmState = PWM_ACTIVE_STATE;
            }
            break;

        case PWM_ACTIVE_STATE:
            if (currentKeyPress == 'B') {
                DeactivatePWM();
                pwmDutyCycle = 0;
                fsmState = IDLE_STATE;
            } else if (IS_DIGIT_KEY(currentKeyPress)) {
                inputBuffer[0] = currentKeyPress;
                inputIndex = 1;
                fsmState = DUTY_CYCLE_ENTRY_STATE;
            }
            break;

        case DUTY_CYCLE_ENTRY_STATE:
            if (IS_DIGIT_KEY(currentKeyPress) && inputIndex < 2) {
                inputBuffer[inputIndex++] = currentKeyPress;
            } else if (currentKeyPress == 'D') {
                inputBuffer[inputIndex] = '\0';
                pwmDutyCycle = atoi(inputBuffer);
                if (pwmDutyCycle >= 0 && pwmDutyCycle <= 99) {
                    AdjustDutyCycle(pwmDutyCycle);
                }
                inputIndex = 0;
                fsmState = PWM_ACTIVE_STATE;
            } else if (currentKeyPress == 'C') {
                inputIndex = 0;
                fsmState = PWM_ACTIVE_STATE;
            }
            break;

        default:
            fsmState = IDLE_STATE;
            break;
    }

    /* Actualiza el ciclo de trabajo basado en la entrada y el estado actual (Mealy) */
    if (fsmState == PWM_ACTIVE_STATE && IS_DIGIT_KEY(currentKeyPress)) {
        AdjustDutyCycle(pwmDutyCycle); /* Dependencia directa de la entrada */
    }
}

/* Escanea el teclado matricial */
char ScanKeypad(void) {
    for (uint8_t row = 0; row < NUM_ROWS; row++)
    {
        for (uint8_t i = 0; i < NUM_ROWS; i++)
        {
            GPIO_PinWrite(GPIOB, rowPins[i], 1U);
        }

        GPIO_PinWrite(GPIOB, rowPins[row], 0U);
        SDK_DelayAtLeastUs(5U, CLOCK_GetFreq(kCLOCK_CoreSysClk));

        for (uint8_t col = 0; col < NUM_COLS; col++)
        {
            if (!GPIO_PinRead(GPIOA, columnPins[col]))
            {
                SDK_DelayAtLeastUs(20U, CLOCK_GetFreq(kCLOCK_CoreSysClk));

                if (!GPIO_PinRead(GPIOA, columnPins[col]))
                {
                    while (!GPIO_PinRead(GPIOA, columnPins[col]))
                    {
                        SDK_DelayAtLeastUs(5U, CLOCK_GetFreq(kCLOCK_CoreSysClk));
                    }

                    return keypadMapping[row][col];
                }
            }
        }
    }

    return '\0'; /* No se presionó ninguna tecla */
}
