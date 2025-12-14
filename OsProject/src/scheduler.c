#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h" // TickType_t burada tanýmlanýr
#include "task.h"     // vTaskDelay, vTaskSuspend vb. için
#include "queue.h"    // Kuyruk API'leri için
#include "portmacro.h" // pdMS_TO_TICKS makrosu için ek önlem

#include "scheduler.h" // Kendi baþlýk dosyalarýnýz
#include "tasks.h"     

// **********************************************
// 1. KUYRUK TANIMLARI VE YARDIMCI YAPILAR
// **********************************************

// Görev kuyruklarý (FreeRTOS Queues). TaskData_t* iþaretçisi tutacaklar.
static QueueHandle_t xRTQueue = NULL;           // Öncelik 0 (Gerçek Zamanlý - FCFS)
static QueueHandle_t xQ1 = NULL;                // Öncelik 1 (En yüksek kullanýcý)
static QueueHandle_t xQ2 = NULL;                // Öncelik 2 (Orta kullanýcý)
static QueueHandle_t xQ3 = NULL;                // Öncelik 3 (En düþük kullanýcý - Round Robin)

// Simülasyon zamaný (1 Tick = 1 Saniye Kuantumu)
static int current_time = 0; 

// O anda çalýþan görev (Scheduler tarafýndan yönetilen)
static TaskData_t *pCurrentTask = NULL; 

// Toplam görev sayýsýný tutmak için bir global deðiþken veya fonksiyon (main.c'den) varsayýlýyor.
// const int TOTAL_TASK_COUNT = [main.c'den gelen görev sayýsý]; 

/**
 * @brief Görevin önceliðine göre uygun kuyruða eklenmesi.
 * @param pTask Kuyruða eklenecek görev verisi iþaretçisi.
 */
static void vAddToAppropriateQueue(TaskData_t *pTask) {
    QueueHandle_t target_queue = NULL;
    
    // Önceliðe göre hedef kuyruðu belirle
    switch (pTask->initial_priority) { // initial_priority = mevcut öncelik
        case 0: target_queue = xRTQueue; break;
        case 1: target_queue = xQ1; break;
        case 2: target_queue = xQ2; break;
        case 3: target_queue = xQ3; break;
        default:
            fprintf(stderr, "HATA: Geçersiz görev önceliði: %d\n", pTask->initial_priority);
            return;
    }

    // Kuyruða ekle (Öðenin iþaretçisini gönderiyoruz)
    if (xQueueSend(target_queue, &pTask, 0) != pdPASS) {
        fprintf(stderr, "HATA: Görev ID %d kuyruða eklenemedi (Kuyruk dolu).\n", pTask->task_id);
    } else {
        printf("[\033[34mKUYRUK\033[0m @%d] Görev ID %d, Öncelik %d kuyruðuna eklendi.\n", 
               current_time, pTask->task_id, pTask->initial_priority);
    }
}


/**
 * @brief Tüm kuyruklarýn ve baþlangýç listesinin boþ olup olmadýðýný kontrol eder.
 * @param initial_task_list Baþlangýç giriþ listesi (tamamlananlar NULL'lanabilir).
 * @return Tüm iþler bittiyse pdTRUE, aksi halde pdFALSE.
 */
static BaseType_t xIsSimulationComplete(TaskData_t *initial_task_list) {
    // 1. Kuyruklarýn boþ olup olmadýðýný kontrol et
    if (uxQueueMessagesWaiting(xRTQueue) > 0 ||
        uxQueueMessagesWaiting(xQ1) > 0 ||
        uxQueueMessagesWaiting(xQ2) > 0 ||
        uxQueueMessagesWaiting(xQ3) > 0) {
        return pdFALSE;
    }
    
    // 2. Gelen görevler listesinde iþlenmemiþ görev kalýp kalmadýðýný kontrol et
    // (initial_task_list'in sonunu kontrol etme mantýðý burada uygulanmalýdýr. 
    // Basitlik için, tüm görevlerin FreeRTOS görevleri olarak oluþturulduðu varsayýlýr.)

    return pdTRUE; // Eðer tüm kuyruklar boþsa ve yeni görev gelmeyecekse
}


// **********************************************
// 2. ANA ZAMANLAYICI GÖREVÝ (vSchedulerTask)
// **********************************************

void vSchedulerTask(void *pvParameters) {
    TaskData_t *initial_task_list = (TaskData_t *)pvParameters;
    
    // Kuyruklarý oluþturma (Hata kontrolü atlanmýþtýr, varsayýlýyor)
    const UBaseType_t max_queue_size = 20; 
    xRTQueue = xQueueCreate(max_queue_size, sizeof(TaskData_t *));
    xQ1 = xQueueCreate(max_queue_size, sizeof(TaskData_t *));
    xQ2 = xQueueCreate(max_queue_size, sizeof(TaskData_t *));
    xQ3 = xQueueCreate(max_queue_size, sizeof(TaskData_t *));
    
    // Simülasyon sonlanana kadar devam eden döngü
    while (1) {
        printf("\n\033[44m--- ZAMAN ADIMI: %d ---\033[0m\n", current_time);

        // 1. Yeni Gelen Görevleri Kontrol Etme ve Oluþturma
        // Varsayýlan TaskData_t listesinde sonu bulmak için basit bir döngü
        for (int i = 0; initial_task_list[i].required_time > 0 || initial_task_list[i].arrival_time > 0; i++) {
            
            TaskData_t *pIncomingTask = &initial_task_list[i];

            // Görev henüz oluþturulmadýysa (handle == NULL) ve varýþ zamaný geldiyse
            if (pIncomingTask->handle == NULL && pIncomingTask->arrival_time <= current_time) {
                
                // Görev için dinamik bellek ayýrma (TaskData_t yapýsýný kopyala)
                TaskData_t *pNewTask = (TaskData_t *)pvPortMalloc(sizeof(TaskData_t));
                if (pNewTask == NULL) { /* Hata yönetimi */ continue; }
                *pNewTask = *pIncomingTask; 
                
                // FreeRTOS Görevini Oluþtur
                // Öncelik, pNewTask->initial_priority'den alýnýr (0, 1, 2 veya 3)
                if (xTaskCreate(vTaskFunction, "SimTask", configMINIMAL_STACK_SIZE,
                                (void *)pNewTask, pNewTask->initial_priority, 
                                &(pNewTask->handle)) == pdPASS) 
                {
                    printf("[\033[32mYENÝ GÖREV\033[0m @%d] ID %d oluþturuldu. Öncelik: %d.\n", 
                           current_time, pNewTask->task_id, pNewTask->initial_priority);
                    
                    // Görev oluþturulur oluþturulmaz askýya alýnýp kuyruða alýnmalý
                    vTaskSuspend(pNewTask->handle); 
                    vAddToAppropriateQueue(pNewTask);
                    
                } else {
                    fprintf(stderr, "HATA: Görev oluþturulamadý (ID %d).\n", pNewTask->task_id);
                    pvPortFree(pNewTask);
                }
            }
        }
        
        // **********************************************
        // 2. ZAMANLAMA KARARI VE YÜRÜTME (ÞEKÝL 3 MANTIK AKIÞI)
        // **********************************************
        
        // --- 2.1. RT Kuyruk Kontrolü (Öncelik 0 - FCFS) ---
        TaskData_t *pNextRTTask = NULL;
        if (uxQueueMessagesWaiting(xRTQueue) > 0) {
            
            // Eðer o anda çalýþan bir kullanýcý görevi varsa (RT deðilse), onu kes.
            if (pCurrentTask != NULL && pCurrentTask->initial_priority != 0) {
                printf("[\033[31mPREEMPT\033[0m @%d] Kullanýcý Görevi ID %d kesildi.\n", current_time, pCurrentTask->task_id);
                vTaskSuspend(pCurrentTask->handle);
                // Kesilen görevi, kuantum sonrasý mantýkta olduðu gibi kuyruða geri göndermeye gerek yok, 
                // çünkü bu görev kuantumunu henüz bitirmedi. Sadece askýya alýnýr.
                // Bu, simülasyonu kolaylaþtýrmak için bir basitleþtirmedir.
                pCurrentTask = NULL; 
            }
            
            // RT Görevini Kuyruktan Çýkar
            if (xQueueReceive(xRTQueue, &pNextRTTask, 0) == pdPASS) {
                pCurrentTask = pNextRTTask;
                printf("[\033[36mRT KURALI\033[0m @%d] RT Görevi ID %d yürütülüyor (Süre: %d sn).\n", 
                       current_time, pCurrentTask->task_id, pCurrentTask->remaining_time);
                
                // RT görevi kesintisiz çalýþýr. Tüm süresi boyunca bekle.
                vTaskResume(pCurrentTask->handle);
                TickType_t xExecutionTicks = pdMS_TO_TICKS(pCurrentTask->remaining_time * 1000);
                vTaskDelay(xExecutionTicks);
                
                // Görev tamamlandýktan sonra
                current_time += pCurrentTask->remaining_time; 
                pCurrentTask->remaining_time = 0; 
                
                printf("[\033[35mSON\033[0m @%d] RT Görevi ID %d tamamlandý.\n", current_time, pCurrentTask->task_id);
                vTaskDelete(pCurrentTask->handle);
                pvPortFree(pCurrentTask);
                pCurrentTask = NULL;
                
                continue; // Döngüyü yeniden baþlat (yeni gelen RT görevlerini kontrol etmek için)
            }
        }
        
        // --- 2.2. Geri Beslemeli Kuyruk Kontrolü (Öncelik 1, 2, 3) ---
        
        TaskData_t *pNextUserTask = NULL;
        BaseType_t xTaskFound = pdFALSE;

        // En yüksek öncelikli boþ olmayan kuyruktan görevi çek
        if (xQueueReceive(xQ1, &pNextUserTask, 0) == pdPASS) xTaskFound = pdTRUE;
        else if (xQueueReceive(xQ2, &pNextUserTask, 0) == pdPASS) xTaskFound = pdTRUE;
        else if (xQueueReceive(xQ3, &pNextUserTask, 0) == pdPASS) xTaskFound = pdTRUE; 
        
        if (xTaskFound == pdTRUE) {
            pCurrentTask = pNextUserTask;
            printf("[\033[36mKUANTUM\033[0m @%d] Görev ID %d (Öncelik %d) 1 sn kuantum için çalýþýyor.\n", 
                   current_time, pCurrentTask->task_id, pCurrentTask->initial_priority);

            // Görevi çalýþtýrmaya baþla
            vTaskResume(pCurrentTask->handle);
            
            // 1 Saniyelik Kuantum Süresince Bekle
            vTaskDelay(pdMS_TO_TICKS(1000)); 

            // Görevi askýya al (Kuantum doldu)
            vTaskSuspend(pCurrentTask->handle); 
            
            // Kalan süreyi güncelle
            pCurrentTask->remaining_time--;

            // Kuantum Sonrasý Kontrol ve Öncelik Düþürme
            if (pCurrentTask->remaining_time <= 0) {
                // Görev Tamamlandý
                printf("[\033[35mSON\033[0m @%d] Kullanýcý Görevi ID %d tamamlandý.\n", current_time + 1, pCurrentTask->task_id);
                vTaskDelete(pCurrentTask->handle);
                pvPortFree(pCurrentTask);
            } else {
                // Görev Askýya Alýndý ve Önceliði Düþürülecek
                int new_priority = pCurrentTask->initial_priority + 1;
                
                // Round Robin (Öncelik 3'ten sonrasý tekrar 3'e gider)
                if (new_priority > 3) new_priority = 3; 

                // Öncelik ayarlandý ve ilgili kuyruða geri gönderildi
                vTaskPrioritySet(pCurrentTask->handle, new_priority);
                pCurrentTask->initial_priority = new_priority;
                vAddToAppropriateQueue(pCurrentTask); 
            }
            pCurrentTask = NULL; // Karar tamamlandý
        } 
        else {
            // --- 2.3. Tüm Kuyruklar Boþ ---
            printf("[\033[33mBOÞTA\033[0m @%d] Tüm kuyruklar boþ. Kontrol ediliyor...\n", current_time);

            // Simülasyon sonu kontrolü
            if (xIsSimulationComplete(initial_task_list) == pdTRUE) {
                printf("\n\033[42m*** Tüm görevler tamamlandý. Zamanlayýcý sonlanýyor. ***\033[0m\n");
                // FreeRTOS'u kapatma mekanizmasý (POSIX portunda vTaskEndScheduler() kullanýlabilir)
                vTaskEndScheduler();
                break;
            }
            
            // Yeni görev gelene kadar bekle (1 saniye)
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Zaman adýmýný ilerlet (RT görevleri yukarýda kendi süresi kadar ilerletir)
        current_time++; 
    }
    
    // Simülasyon sonlandýðýnda bu görev silinir.
    vTaskDelete(NULL); 
}