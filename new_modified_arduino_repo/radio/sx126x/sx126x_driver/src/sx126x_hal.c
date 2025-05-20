#include "sx126x_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "radio_board.h"



sx126x_hal_status_t sx126x_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    radio_context_t* sx126x_context = (const radio_context_t* ) context;

    sx126x_hal_wait_on_busy(sx126x_context);
    HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&(sx126x_context->spi), command,command_length, 10000);
    HAL_SPI_Receive(&(sx126x_context->spi), data,data_length, 10000);
    HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;

}


sx126x_hal_status_t sx126x_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    radio_context_t* sx126x_context = (const radio_context_t* ) context;
    sx126x_hal_wait_on_busy(sx126x_context);
    
    HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&(sx126x_context->spi), command,command_length, 10000);
    HAL_SPI_Transmit(&(sx126x_context->spi),data,data_length, 10000);
    HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;


}


sx126x_hal_status_t sx126x_hal_wait_on_busy( const void* radio )
{
    const radio_context_t* sx126x_context = (const radio_context_t* ) radio;

    GPIO_PinState gpio_state;

    do
    {
        gpio_state = HAL_GPIO_ReadPin(sx126x_context->busy.GPIO_PORT,sx126x_context->busy.pin);
    } while (gpio_state == GPIO_PIN_SET);
    
    return SX126X_HAL_STATUS_OK;

}

sx126x_hal_status_t sx126x_hal_reset( const void* context )
{

    radio_context_t* sx126x_context = (const radio_context_t* ) context;
    
    HAL_GPIO_WritePin(RESET_PIN_PORT, RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RESET_PIN_PORT, RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(20);

    return SX126X_HAL_STATUS_OK;

}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context )
{
    radio_context_t* sx126x_context = ( radio_context_t* ) context;

    if( ( radio_board_get_operating_mode( ) == RADIO_BOARD_OP_MODE_SLEEP ) ||
        ( radio_board_get_operating_mode( ) == RADIO_BOARD_OP_MODE_RX_DC ) )
    {
        // Wakeup radio
        HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_RESET);

        

        // Radio is awake in STDBY_RC mode
        HAL_GPIO_WritePin(sx126x_context->nss.GPIO_PORT, sx126x_context->nss.pin, GPIO_PIN_SET);

    }

    // Wait on busy pin for 100 ms
    HAL_Delay(100);
    return sx126x_hal_wait_on_busy( sx126x_context );
}