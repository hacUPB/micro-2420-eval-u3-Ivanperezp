[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/tn5SB-Yw)
# Unidad 3
## Documentación del Proyecto
 
Nombre del estudiante:  
ID: 

---
Se presenta el diagrama de estados del proyecto:
![alt text](<Imagen de WhatsApp 2024-10-11 a las 20.48.23_61f28a59.jpg>)

Se implementó un ejemplo del PWM en el SDK, el siguiente video lo presenta:
<video controls src="Video de WhatsApp 2024-10-11 a las 20.48.24_7f45c0d8.mp4" title="Funcionamiento PWM"></video>
En caso de no poder visualizar el video en línea, por favor descargarlo, se encuentra en el repositorio. 

Finalmente, se quiso implementar una prueba con el teclado, sin embargo los resultados no fueron fructíferos. Se escogió el puerto E. 

# Informe sobre el código para el control de un teclado matricial y el control de PWM en un sistema embebido

Este programa está diseñado para controlar la modulación por ancho de pulsos (PWM) utilizando un microcontrolador con el módulo FlexTimer (FTM) y una interfaz de teclado matricial 4x4. El objetivo es ajustar el ciclo de trabajo del PWM a través de la entrada del teclado, permitiendo activar, desactivar y modificar la intensidad de una señal de salida que controla un LED. Este sistema utiliza una máquina de estados finita (FSM) para gestionar la lógica del flujo de entrada y salida del PWM.

## 2. Definiciones clave y configuración
El código comienza con la inclusión de las librerías necesarias para el funcionamiento del microcontrolador, como la consola de depuración, la configuración de pines, el reloj del sistema, y las funciones del módulo FlexTimer (FTM). A continuación, se definen parámetros clave:

- **PWM_FTM_INSTANCE y PWM_FTM_CHANNEL_NUM:** Estas macros configuran el módulo y el canal del temporizador FTM que generará la señal PWM.
- **FTM_INTERRUPT_NUMBER y FTM_INTERRUPT_HANDLER:** Definen la interrupción del FTM y su manejador correspondiente (aunque no se utiliza en este código).
- **FTM_CLOCK_SOURCE:** Establece la fuente de reloj para el FTM utilizando la frecuencia del reloj central del microcontrolador.

Asimismo, se definen los pines de las filas y columnas del teclado matricial y se mapean los caracteres correspondientes a cada tecla.

## 3. Variables y estructuras de datos
El código utiliza varias variables globales, incluyendo:
- **fsmState:** Un enumerador que define los posibles estados de la máquina de estados (IDLE_STATE, PWM_ACTIVE_STATE, DUTY_CYCLE_ENTRY_STATE).
- **pwmDutyCycle:** Almacena el ciclo de trabajo (duty cycle) del PWM, representando el porcentaje de tiempo que la señal estará activa.
- **inputBuffer:** Almacena temporalmente las entradas del teclado cuando se ingresa el valor del ciclo de trabajo.
- **pwmSignalParams:** Estructura que almacena los parámetros necesarios para configurar la señal PWM, como el canal, el nivel de la señal, el ciclo de trabajo y el retardo en el primer flanco.

## 4. Lógica del programa
El programa principal se ejecuta en un bucle infinito, donde continuamente se lee el estado del teclado matricial y se llama a la función `StateManager()` para procesar la lógica de la máquina de estados. El flujo del programa se describe a continuación:

- **IDLE_STATE:** El sistema está inactivo y espera a que el usuario presione la tecla 'A' en el teclado para activar el PWM con un ciclo de trabajo inicial del 50%. Si se detecta la tecla 'A', el PWM se activa llamando a la función `ActivatePWM()`, y el estado de la FSM cambia a **PWM_ACTIVE_STATE**.
  
- **PWM_ACTIVE_STATE:** En este estado, el PWM está activo y el sistema espera nuevas entradas. Si el usuario presiona la tecla 'B', se desactiva el PWM con `DeactivatePWM()` y el sistema vuelve al estado inactivo. Si se presiona una tecla numérica, se almacena el primer dígito en `inputBuffer` y la FSM transita a **DUTY_CYCLE_ENTRY_STATE**.

- **DUTY_CYCLE_ENTRY_STATE:** Aquí, el usuario puede ingresar un segundo dígito para ajustar el ciclo de trabajo del PWM. Si se presiona la tecla 'D', se valida la entrada y se ajusta el ciclo de trabajo con `AdjustDutyCycle()`. Si se presiona 'C', la entrada se cancela y el estado regresa a **PWM_ACTIVE_STATE**.

## 5. Funciones importantes

- **ActivatePWM():** Configura el FTM para generar una señal PWM con un ciclo de trabajo inicial del 50% a una frecuencia de 24 kHz.
  
- **DeactivatePWM():** Desactiva la señal PWM.
  
- **AdjustDutyCycle(uint8_t duty):** Modifica el ciclo de trabajo del PWM actualizando los registros correspondientes del FTM. Esta función también incluye un retardo para garantizar que los cambios en el PWM se apliquen correctamente.

- **ScanKeypad():** Lee el estado del teclado matricial y devuelve el carácter correspondiente a la tecla presionada. Utiliza un ciclo anidado para recorrer cada fila y verificar qué columna está activa.

