#include "panic_wrapper.h"
#include "lib.h"
#include <Arduino.h>
#include "esp_debug_helpers.h"


extern "C"
{
    extern esp_err_t __real_esp_backtrace_print_from_frame(int depth, const esp_backtrace_frame_t *frame, bool panic);
}

extern "C" esp_err_t IRAM_ATTR __wrap_esp_backtrace_print_from_frame(int depth, const esp_backtrace_frame_t *frame, bool panic)
{
    // Check arguments
    if (depth <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize stk_frame with first frame of stack
    esp_backtrace_frame_t stk_frame;
    memcpy(&stk_frame, frame, sizeof(esp_backtrace_frame_t));

    // Check if first frame is valid
    bool corrupted = !(esp_stack_ptr_is_sane(stk_frame.sp) &&
                       (esp_ptr_executable((void *)esp_cpu_process_stack_pc(stk_frame.pc)) ||
                        /* Ignore the first corrupted PC in case of InstrFetchProhibited */
                        (stk_frame.exc_frame && ((XtExcFrame *)stk_frame.exc_frame)->exccause == EXCCAUSE_INSTR_PROHIBITED)));

    // depth >= STACK_DEPTH のとき、全部保存されない
    uint32_t i = STACK_DEPTH;
    while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted)
    {
        if (!esp_backtrace_get_next_frame(&stk_frame))
        { // Get previous stack frame
            corrupted = true;
        }
        s_exception_info.pc[STACK_DEPTH - i] = esp_cpu_process_stack_pc(stk_frame.pc);
        s_exception_info.sp[STACK_DEPTH - i] = stk_frame.sp;
    }

    return __real_esp_backtrace_print_from_frame(depth, frame, panic);
}

/*
esp_err_t IRAM_ATTR esp_backtrace_print_from_frame(int depth, const esp_backtrace_frame_t* frame, bool panic)
{
    //Check arguments
    if (depth <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    //Initialize stk_frame with first frame of stack
    esp_backtrace_frame_t stk_frame;
    memcpy(&stk_frame, frame, sizeof(esp_backtrace_frame_t));

    print_str("\r\n\r\nBacktrace:", panic);
    print_entry(esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp, panic);

    //Check if first frame is valid
    bool corrupted = !(esp_stack_ptr_is_sane(stk_frame.sp) &&
                       (esp_ptr_executable((void *)esp_cpu_process_stack_pc(stk_frame.pc)) ||
                        // Ignore the first corrupted PC in case of InstrFetchProhibited
                        (stk_frame.exc_frame && ((XtExcFrame *)stk_frame.exc_frame)->exccause == EXCCAUSE_INSTR_PROHIBITED)));

    uint32_t i = (depth <= 0) ? INT32_MAX : depth;
    while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted) {
        if (!esp_backtrace_get_next_frame(&stk_frame)) {    //Get previous stack frame
            corrupted = true;
        }
        print_entry(esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp, panic);
    }

    //Print backtrace termination marker
    esp_err_t ret = ESP_OK;
    if (corrupted) {
        print_str(" |<-CORRUPTED", panic);
        ret =  ESP_FAIL;
    } else if (stk_frame.next_pc != 0) {    //Backtrace continues
        print_str(" |<-CONTINUES", panic);
    }
    print_str("\r\n\r\n", panic);
    return ret;
}
*/