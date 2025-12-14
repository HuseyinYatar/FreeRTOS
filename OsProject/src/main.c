#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FreeRTOS Kütüphaneleri
#include "FreeRTOS.h"
#include "task.h"

// Kendi baþlýk dosyalarýnýz
#include "tasks.h"     // TaskData_t yapýsýný ve vTaskFunction prototipini içerir
#include "scheduler.h" // vSchedulerTask prototipini içerir

// Global Deðiþkenler (Eðer scheduler.c'den eriþilmesi gerekiyorsa)
// Global deðiþken kullanýmýndan kaçýnmak için bunlarý SchedulerTask'a parametre olarak geçireceðiz.
// TaskData_t *initial_task_list = NULL;
// int task_count = 0;


/**
 * @brief Giriþ dosyasýný okur ve görev verilerini ayrýþtýrýr.
 * * @param filename Giriþ dosyasýnýn adý (giris.txt).
 * @param task_list Ayrýþtýrýlan görev verilerini tutacak dinamik TaskData_t listesi.
 * @param task_count Bulunan görev sayýsýný döndürür.
 */
void parse_input(const char *filename, TaskData_t **task_list, int *task_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("HATA: Giriþ dosyasý açýlamadý.");
        exit(EXIT_FAILURE);
    }

    char line[100];
    int count = 0;
    
    // 1. Satýr sayýsýný sayarak bellek için gereken boyutu bul
    while (fgets(line, sizeof(line), file)) {
        count++;
    }
    
    // Görev listesi için bellek ayýr (Calloc ile sýfýrlanmýþ bellek)
    *task_list = (TaskData_t *)calloc(count, sizeof(TaskData_t));
    *task_count = count;

    // Dosyayý tekrar baþýna al
    fseek(file, 0, SEEK_SET);

    int i = 0;
    while (fgets(line, sizeof(line), file) && i < count) {
        int arrival, priority, duration;
        
        // Veriyi "virgül" ile ayrýþtýr
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &duration) == 3) {
            (*task_list)[i].arrival_time = arrival;
            (*task_list)[i].initial_priority = priority;
            (*task_list)[i].required_time = duration;
            (*task_list)[i].remaining_time = duration; // Baþlangýçta kalan süre = gereken süre
            (*task_list)[i].task_id = i + 1;           // 1'den baþlayan ID atama
            // TODO: tasks.c içinde bir rastgele renk atama fonksiyonu çaðýrýlabilir.

            printf("Giriþ Okundu: ID %d | Varýþ: %d | Öncelik: %d | Süre: %d sn\n", 
                   (*task_list)[i].task_id, arrival, priority, duration);
            i++;
        } else {
            fprintf(stderr, "UYARI: Dosya formatý hatasý (Satýr %d): %s", i + 1, line);
        }
    }
    fclose(file);
}

int main( int argc, char *argv[] ) {
    
    // 1. Komut satýrý argümaný kontrolü
    if (argc < 2) {
        fprintf(stderr, "Kullaným: ./freertos_sim <giris.txt>\n");
        return EXIT_FAILURE;
    }

    TaskData_t *initial_task_list = NULL;
    int task_count = 0;

    // 2. Giriþ dosyasýný oku ve görev listesini hazýrla
    parse_input(argv[1], &initial_task_list, &task_count);

    if (task_count == 0) {
        printf("Giriþ dosyasýnda iþlenecek görev bulunamadý. Çýkýlýyor.\n");
        return EXIT_SUCCESS;
    }

    // 3. Özel Zamanlayýcý Görevini Oluþturma
    // Bu görev, tüm giriþ listesini parametre olarak alacak ve kendi kuyruk mantýðýný yönetecek.
    TaskHandle_t xSchedulerHandle = NULL;
    
    // configMAX_PRIORITIES - 1, FreeRTOS'taki en yüksek kullanýcý önceliði olmalýdýr.
    // Bu, scheduler'ýn diðer tüm kullanýcý görevlerinden önce çalýþmasýný saðlar.
    xTaskCreate(
        vSchedulerTask,                 // Scheduler mantýðýný içeren fonksiyon
        "SchedulerTask",                // Görev Adý
        configMINIMAL_STACK_SIZE * 4,   // Yeterli stack boyutu (dosya okuma, kuyruk yönetimi için)
        (void *)initial_task_list,      // Hazýrlanan görev listesini parametre olarak ver
        configMAX_PRIORITIES - 1,       // En yüksek kullanýcý önceliði (RTOS görevlerinden düþük)
        &xSchedulerHandle               // Handle, gerekirse harici kontrol için
    );

    printf("\nFreeRTOS Görev Simülasyonu Baþlatýlýyor...\n");
    printf("----------------------------------------\n");

    // 4. FreeRTOS çekirdeðini baþlat
    // Bu fonksiyon, baþarýlý olursa geri dönmez, sonsuz döngüye girer ve 
    // zamanlayýcý görevini (SchedulerTask) çalýþtýrmaya baþlar.
    vTaskStartScheduler();
    
    // Buraya ulaþýlmasý, genellikle bellek yetersizliði gibi kritik bir hatadýr.
    printf("HATA: FreeRTOS Zamanlayýcýsý Sonlandý! (Yetersiz bellek veya kritik hata)\n"); 

    // Bellek temizleme (Eðer scheduler baþlatýlamazsa)
    if (initial_task_list != NULL) {
        free(initial_task_list);
    }
    
    return 0;
}