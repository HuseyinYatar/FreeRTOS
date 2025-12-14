#include "tasks.h"
#include "FreeRTOS.h"
#include "task.h"
void vTaskFunction( void *pvParameters ) {
    TaskData_t *pTaskData = (TaskData_t *)pvParameters;

    printf("%s[GOREV BASLADI] ID: %d, Oncelik: %d\033[0m\n", 
           pTaskData->color_code, pTaskData->task_id, pTaskData->initial_priority);

    // Ana görev döngüsü, scheduler tarafýndan yönetilir
    while (pTaskData->remaining_time > 0) {
        
        // Görevin çalýþtýðýný gösteren mesajý yazdýr
        printf("%s[GOREV YURUTULUYOR] ID: %d, Kalan: %d sn\033[0m\n", 
               pTaskData->color_code, pTaskData->task_id, pTaskData->remaining_time);
        
        // Kalan süreyi güncellemek ve askýya alýnmak/önceliði düþürülmek üzere
        // kontrolü scheduler'a býrak
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 saniyelik simülasyon gecikmesi

        // Not: Görevin kalan süresini (remaining_time) ve önceliðini düþürme 
        // iþlemi, genellikle vSchedulerTask içinde yapýlmalýdýr, 
        // çünkü vTaskFunction'ýn kendisi kesintiye uðradýðýný bilemez.
    }
    
    // Görev tamamlandýysa veya scheduler tarafýndan silinecekse bu kýsma ulaþýlmaz.
    // Eðer görev kendi kendini silerse:
    // vTaskDelete( NULL ); 
}