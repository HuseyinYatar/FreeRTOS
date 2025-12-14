#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tasks.h" 
void parse_input(const char *filename, TaskData_t **task_list, int *task_count);

int main( int argc, char *argv[] ) {
    if (argc < 2) {
        fprintf(stderr, "Kullaným: %s <giris.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    TaskData_t *initial_task_list = NULL;
    int task_count = 0;

    // 1. Giriþ dosyasýný oku ve görev listesini hazýrla
    parse_input(argv[1], &initial_task_list, &task_count);

    // 2. FreeRTOS çekirdeðini ve Scheduler görevinizi baþlatýn
    // (Yukarýdaki xTaskCreate ve vTaskStartScheduler çaðrýlarý)
    // ...

    // vTaskStartScheduler() baþarýyla dönerse, bu bir hata durumudur (genellikle gömülü sistemlerde).
    // POSIX simülasyonunda buraya ulaþmamalýdýr.
    printf("Zamanlayýcý sonlandý.\n"); 

    // Belleði temizle
    free(initial_task_list);
    
    return 0;
}

void parse_input(const char *filename, TaskData_t **task_list, int *task_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Giriþ dosyasý açýlamadý.");
        exit(EXIT_FAILURE);
    }

    char line[100];
    int count = 0;
    
    // Dosyadaki satýr sayýsýný bulmak (Bellek ayýrmak için)
    while (fgets(line, sizeof(line), file)) {
        count++;
    }
    
    // Görev listesi için bellek ayýr
    *task_list = (TaskData_t *)calloc(count, sizeof(TaskData_t));
    *task_count = count;

    // Dosyayý tekrar baþýna al
    fseek(file, 0, SEEK_SET);

    int i = 0;
    while (fgets(line, sizeof(line), file) && i < count) {
        int arrival, priority, duration;
        
        // sscanf veya strtok kullanarak veriyi ayýr
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &duration) == 3) {
            (*task_list)[i].arrival_time = arrival;
            (*task_list)[i].initial_priority = priority;
            (*task_list)[i].required_time = duration;
            (*task_list)[i].remaining_time = duration;
            (*task_list)[i].task_id = i + 1; // Basit bir ID atama
            // Rastgele renk atamasý burada yapýlabilir.
            i++;
        }
    }
    fclose(file);
}