#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#include <rom/ets_sys.h>

#define PRI_PREFIX "MC "

void *heap_caps_malloc_default(size_t size);
void *heap_caps_realloc_default(void *p, size_t size);

void *__real_heap_caps_malloc(size_t size, uint32_t caps);
void __real_heap_caps_free(void *p);
void *__real_heap_caps_realloc(void *p, size_t size, uint32_t caps);

static portMUX_TYPE mem_check_mux = portMUX_INITIALIZER_UNLOCKED;

extern uint32_t g_ticks_per_us_pro;
uint32_t mem_check_timestamp()
{
    return xthal_get_ccount() / (g_ticks_per_us_pro * 1000);
}

#define TEST_STACK(N) {                                                 \
        const void *addr = __builtin_return_address(N + 1);             \
        if (!esp_ptr_executable(addr))                                  \
        {                                                               \
            break;                                                      \
        }                                                               \
        ets_printf(" %x", (unsigned int)addr);                      \
    }

static IRAM_ATTR __attribute__((noinline)) void mem_check_print_callstack(void)
{
    do
    {
        TEST_STACK(0)
        TEST_STACK(1)
        TEST_STACK(2)
        TEST_STACK(3)
        TEST_STACK(4)
        TEST_STACK(5)
        TEST_STACK(6)
        TEST_STACK(7)
        TEST_STACK(8)
        TEST_STACK(9)
    } while(0);
}

IRAM_ATTR __attribute__((noinline)) void *__wrap_malloc(size_t size)
{
    void *p = heap_caps_malloc_default(size);
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x + %x %x", mem_check_timestamp(), (unsigned int)p, size);
    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    return p;
}

IRAM_ATTR __attribute__((noinline)) void __wrap_free(void *p)
{
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x - %x", mem_check_timestamp(), (unsigned int)p);
//    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    __real_heap_caps_free(p);
}

IRAM_ATTR __attribute__((noinline)) void *__wrap_realloc(void *p, size_t size)
{
    void *p_new = heap_caps_realloc_default(p, size);
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x * %x %x %x", mem_check_timestamp(), (unsigned int)p, (unsigned int)p_new, size);
    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    return p_new;
}

IRAM_ATTR __attribute__((noinline)) void *__wrap_calloc(size_t nmemb, size_t size)
{
    size = size * nmemb;
    void *p = heap_caps_malloc_default(size);
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x + %x %x", mem_check_timestamp(), (unsigned int)p, size);
    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    if (p != NULL)
    {
        memset(p, 0, size);
    }
    return p;
}

IRAM_ATTR __attribute__((noinline)) void *__wrap_heap_caps_malloc(size_t size, uint32_t caps)
{
    void *p = __real_heap_caps_malloc(size, caps);
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x + %x %x", mem_check_timestamp(), (unsigned int)p, size);
    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    return p;
}

IRAM_ATTR __attribute__((noinline)) void __wrap_heap_caps_free(void *p)
{
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x - %x", mem_check_timestamp(), (unsigned int)p);
//    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    __real_heap_caps_free(p);
}

IRAM_ATTR __attribute__((noinline)) void *__wrap_heap_caps_realloc(void *p, size_t size, uint32_t caps)
{
    void *p_new = __real_heap_caps_realloc(p, size, caps);
    taskENTER_CRITICAL(&mem_check_mux);
    ets_printf(PRI_PREFIX "%x * %x %x %x", mem_check_timestamp(), (unsigned int)p, (unsigned int)p_new, size);
    mem_check_print_callstack();
    ets_printf("\n");
    taskEXIT_CRITICAL(&mem_check_mux);
    return p_new;
}
