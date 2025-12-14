#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS.h" 

#include "task.h"
typedef struct {
    int task_id;               // Görev Kimliði (Giriþ dosyasýndan)
    int initial_priority;      // Baþlangýç önceliði (0, 1, 2, 3)
    int required_time;         // Gereken toplam görev süresi (saniye)
    int remaining_time;        // Kalan görev süresi (dinamik olarak deðiþecek)
    int arrival_time;          // Varýþ zamaný (Ne zaman çalýþmaya hazýr hale geldiði)
    char color_code[10];       // Terminal çýktýsý için rastgele renk kodu
    TaskHandle_t handle;       // FreeRTOS görev tanýtýcýsý
} TaskData_t;

#endif // TASKS_H