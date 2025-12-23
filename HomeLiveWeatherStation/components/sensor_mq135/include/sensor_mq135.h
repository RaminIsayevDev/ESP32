#ifndef SENSOR_MQ135_H
#define SENSOR_MQ135_H

// --- НАСТРОЙКИ MQ135 ---
#define MQ135_ADC_UNIT      ADC_UNIT_1
#define MQ135_ADC_CHANNEL   ADC_CHANNEL_6   
#define MQ135_ATTEN         ADC_ATTEN_DB_12
#define RL_VALUE            1.0   // kOhm

extern float R0;                // Откалибруйте это значение при 20C/33%RH!

// Параметры для CO2
#define PARA_A  110.47
#define PARA_B  -2.862

// Функция чтения датчика MQ135
float sensor_mq135_read();

// --- ДЕЛИТЕЛЬ НАПРЯЖЕНИЯ ---
#define R_DIVIDER_GND       2.2   // kOhm (резистор к земле)
#define R_DIVIDER_SENSOR    3.3   // kOhm (резистор от датчика)
#define V_SENSOR_MAX        5.0   // V (максимальное напряжение на выходе датчика)

#endif