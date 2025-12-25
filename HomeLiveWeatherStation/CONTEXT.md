# Project: HomeWeatherStation (LiveUI)

## 🎯 Общая цель
Создание домашней метеостанции на ESP32 с передачей данных в реальном времени на Android-приложение (Jetpack Compose).

## 🛠 Технический стек
- **Hardware:** ESP32, MQ135 (Gas), BME280 (T/H/P), BH1750 (Lux).
- **Firmware:** C, FreeRTOS, Arduino framework.
- **Mobile:** Android (Kotlin, Jetpack Compose, LiveUI).
- **Protocol:** MQTT (предположительно) или HTTP.

## 📈 Текущий прогресс
- [x] Чтение данных с сенсоров (задача `sensors`).
- [x] Подключение к Wi-Fi (менеджер соединений).
- [ ] Форматирование данных (JSON).
- [ ] Отправка данных на сервер/брокер.
- [ ] Реализация LiveUI в Android.

## 📂 Описание текущих задач (Задачи для AI)
1. **Wi-Fi Manager:** Нужна реализация стабильного подключения с реконнектом. [x]
2. **Data Logic:** Подготовка структуры JSON для передачи.
3. **Android UI:** Создание ViewModel и UI-компонентов для отображения потоковых данных.