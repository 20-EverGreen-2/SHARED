/* i2c - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "soc/i2c_reg.h"

#include "sdkconfig.h"

static const char *TAG = "i2c-example";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

#define I2C_SLAVE_SCL_IO CONFIG_I2C_SLAVE_SCL               /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO CONFIG_I2C_SLAVE_SDA               /*!< gpio number for i2c slave data */
#define I2C_SLAVE_NUM I2C_NUMBER(CONFIG_I2C_SLAVE_PORT_NUM) /*!< I2C port number for slave dev */
#define I2C_SLAVE_TX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave tx buffer size */
#define I2C_SLAVE_RX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave rx buffer size */

#define ESP_SLAVE_ADDR CONFIG_I2C_SLAVE_ADDRESS /*!< ESP32 slave address, you can set any 7bit value */
SemaphoreHandle_t print_mux = NULL;

static xQueueHandle i2c_event_queue = NULL;

static intr_handle_t i2c_slave_intr_handle = NULL;


struct QueueMessage{
	int interruptNo;
} xMessage;

/**
 * @brief test function to show buffer
 */
static void disp_buf(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void IRAM_ATTR i2c_interrupt(){
	ets_printf("i2c_interrupt has been triggered\n");
	ets_printf("interupt number %d\n", esp_intr_get_intno(i2c_slave_intr_handle));
	
	struct QueueMessage *message;
	message = (struct QueueMessage*)malloc(sizeof(struct QueueMessage));
	message->interruptNo = esp_intr_get_intno(i2c_slave_intr_handle);
	
	//int ret = i2c_slave_write_buffer(I2C_SLAVE_NUM
	
	if(i2c_isr_free(i2c_slave_intr_handle) == ESP_OK){
		i2c_slave_intr_handle = NULL;
		ets_printf("Free-ed interrupt handler\n");
	}else{
		ets_printf("Failed to free interrupt handler\n");
	}
	
	BaseType_t ret = xQueueSendFromISR(i2c_event_queue, &message, NULL);
	
	if(ret != pdTRUE){
		ets_printf("Could not send event to queue (%d)\n", ret);
	}
}

static void i2c_handle_interrupt(void *arg){
	
	xSemaphoreTake(print_mux, portMAX_DELAY);
	ESP_LOGI(TAG, "Starting i2c_handle_interrupt loop");
	ESP_LOGI(TAG, "Waiting for i2c events in the event queue");
	xSemaphoreGive(print_mux);
	
	while(1){
	
		xSemaphoreTake(print_mux, portMAX_DELAY);
	
		struct QueueMessage *message;
	
		BaseType_t ret = xQueueReceive(i2c_event_queue, &(message), 1000 / portTICK_RATE_MS);
				
		if(ret){
					
			ESP_LOGI(TAG, "Found new I2C event to handle");
			ESP_LOGI(TAG, "Resetting queue");
		
			free(message);
		
			int size;
			
			uint8_t *data = (uint8_t *)malloc(RW_TEST_LENGTH);
			
			// This is the data length
			int data_length = 0;
			
			size = i2c_slave_read_buffer(I2C_SLAVE_NUM, &data, 16, 1000 / portTICK_RATE_MS);
			
			if(size){
				data_length = atoi((char*)data);
				ESP_LOGI(TAG, "Master told me that there are a few bytes comming up");
				printf("%d bytes to be precise", size);
				disp_buf(data, size);
			}else{
				ESP_LOGW(TAG, "i2c_slave_read_buffer returned -1");
			}
						
			size = i2c_slave_read_buffer(I2C_SLAVE_NUM, &data, data_length, 1000 / portTICK_RATE_MS);
			
			if(size != data_length){
				ESP_LOGW(TAG, "I2C expected data length vs read data length does not match");
			}else{
				disp_buf(data, size);
			}
			
			ESP_LOGI(TAG, "Registering interrupt again");
			
			esp_err_t isr_register_ret = i2c_isr_register(I2C_SLAVE_NUM, i2c_interrupt, 0,0,&i2c_slave_intr_handle);
		
			if(isr_register_ret == ESP_OK){
				ESP_LOGI(TAG, "Registered interrupt handler");
			}else{
				ESP_LOGW(TAG, "Failed to register interrupt handler");
			}			
		}else{
			ESP_LOGW(TAG, "Failed to get queued event");
			printf("xQueueReceive() returned %d\n", ret);
		}
		
		xSemaphoreGive(print_mux);
		
		vTaskDelay(portTICK_RATE_MS / 1000);
	}
	
	vSemaphoreDelete(print_mux);	
	vTaskDelete(NULL);
}

/**
 * @brief i2c slave initialization
 */
static esp_err_t i2c_slave_init()
{
	i2c_event_queue = xQueueCreate(5, sizeof(uint32_t *));
	
	if(i2c_event_queue == 0){
		ESP_LOGW(TAG, "Failed to create event queue");
	}
	
	xSemaphoreTake(print_mux, portMAX_DELAY);
    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t conf_slave;
    conf_slave.sda_io_num = I2C_SLAVE_SDA_IO;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = I2C_SLAVE_SCL_IO;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = ESP_SLAVE_ADDR;
    i2c_param_config(i2c_slave_port, &conf_slave);
	esp_err_t isr_register_ret = i2c_isr_register(i2c_slave_port, i2c_interrupt, 0,0,&i2c_slave_intr_handle);
	
	printf("I2C Slave started with addr %x\n", ESP_SLAVE_ADDR);
	
	if(isr_register_ret == ESP_OK){
		ESP_LOGI(TAG, "Registered interrupt handler");
	}else{
		ESP_LOGW(TAG, "Failed to register interrupt handler");
	}
	
	xSemaphoreGive(print_mux);
	
    return i2c_driver_install(i2c_slave_port, conf_slave.mode,
                              I2C_SLAVE_RX_BUF_LEN,
                              I2C_SLAVE_TX_BUF_LEN, 0);
}
void app_main()
{
    print_mux = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(i2c_slave_init());
    //ESP_ERROR_CHECK(i2c_master_init());
    xTaskCreate(i2c_handle_interrupt, "i2c_handle_interrupt_0", 1024 * 2, (void *)0, 10, NULL);
    //xTaskCreate(i2c_test_task, "i2c_test_task_1", 1024 * 2, (void *)1, 10, NULL);
}
