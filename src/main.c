#include "stdint.h"
#include "stdio.h"
#include "ti/real.h"
#include "ti/screen.h"
#include <math.h>
#include <string.h>
#include <tice.h>

// <fileioc.h>
#define ti_X ("\x58\0\0")
#define ti_L1 ("\x5D\x0\0")

real_t stack[101];
char buffer[50];
uint8_t idx;
bool decimal;
bool negative;
bool constantsmode = false;
bool scimode = false;
bool radians = false;
real_t decimalfactor;

real_t ln10, pi, e;
real_t rZero, rOne, rTen;

real_t x;
real_t L1[100];

void init_constants() {
    rZero = os_Int24ToReal(0);
    rOne = os_Int24ToReal(1);
    rTen = os_Int24ToReal(10);

    ln10 = os_RealLog(&rTen);

    real_t negOne = os_Int24ToReal(-1);
    pi = os_RealAcosRad(&negOne);

    e = os_RealExp(&rOne);
}

void real_to_str(real_t r, char *buffer) { os_RealToStr(buffer, &r, 0, 1, -1); }

bool is_zero(real_t r) { return os_RealCompare(&r, &rZero) == 0; }

void draw_line() {
    strcpy(buffer, "                    ");
    real_to_str(stack[idx], buffer);
    for (int i = 0; i < 15; i++) {
        os_SetCursorPos(9, i);
        os_PutStrFull(&buffer[i]);
    }
    os_SetCursorPos(9, 0);
    os_PutStrFull(buffer);
}

void drawdecimal_line() {
    os_SetCursorPos(9, 0);
    os_PutStrFull(buffer);
    os_PutStrFull(".");
}

void draw_stack_clear(uint8_t row, bool clear) {
    if (row >= 9) {
        os_SetCursorPos(8, 0);
        os_PutStrFull("...            ");
        real_t len = os_Int24ToReal((int24_t)idx); // Turn the len into a string
        os_RealToStr(buffer, &len, 0, 1, -1);
        os_SetCursorPos(8, 4);
        os_PutStrFull(buffer);
    } else {
        real_to_str(stack[row], buffer);
        if (clear) {
            os_SetCursorPos(row, 0);
            os_PutStrFull("               ");
        }
        os_SetCursorPos(row, 0);
        os_PutStrFull(buffer);
    }
}

void draw_stack(uint8_t row) { draw_stack_clear(row, false); }

void draw_full_stack() {
    for (uint8_t row = 0; row < idx && row <= 9; row++)
        draw_stack_clear(row, true);
}

void delete_stack(uint8_t row) {
    if (row < 9) {
        os_SetCursorPos(row, 0);
        os_PutStrFull("               ");
    }
}

void hint(char *str) {
    os_SetCursorPos(0, 25);
    os_PutStrFull(str);
}

void redraw_screen() {
    os_ClrHome();
    draw_full_stack();
    draw_line();
}

void draw_lines(const char *keys[]) {
    os_ClrHome();

    uint24_t font_height = os_FontGetHeight() + 3;
    uint24_t spacing = font_height * 2; // So starts past bar

    for (int i = 0; i < 11; i++) {
        os_FontDrawText(keys[i], 0, spacing);
        spacing += font_height;
    }

    while (os_GetCSC() == 0)
        ;

    redraw_screen();
}

void constants_mode(bool enabled) {
    if (enabled) {
        constantsmode = true;
        hint("^");
    } else {
        constantsmode = false;
        hint(" ");
    }
}

void new_entry() {
    decimal = false;
    negative = false;
    stack[idx] = rZero;
    hint(" ");
    draw_line();
    // The new line doesn't overwrite the second char so manually fix that
    os_SetCursorPos(9, 1);
    os_PutStrFull(" ");
}

void new_problem() {
    idx = 0;
    os_ClrHome();
    new_entry();
}

#define BINARY_OP(os_func)                                                             \
    do {                                                                               \
        if (!is_zero(stack[idx])) {                                                    \
            if (idx >= 1) {                                                            \
                stack[idx - 1] = os_func(&stack[idx - 1], &stack[idx]);                \
                draw_stack_clear(idx - 1, true);                                       \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        } else {                                                                       \
            if (idx >= 2) {                                                            \
                stack[idx - 2] = os_func(&stack[idx - 2], &stack[idx - 1]);            \
                draw_stack_clear(idx - 2, true);                                       \
                delete_stack(idx - 1);                                                 \
                idx--;                                                                 \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        }                                                                              \
    } while (false)

#define UNARY_OP(os_func)                                                              \
    do {                                                                               \
        if (!is_zero(stack[idx])) {                                                    \
            stack[idx] = os_func(&stack[idx]);                                         \
            draw_line();                                                               \
        } else {                                                                       \
            if (idx >= 1) {                                                            \
                stack[idx - 1] = os_func(&stack[idx - 1]);                             \
                draw_stack_clear(idx - 1, true);                                       \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        }                                                                              \
    } while (false)

real_t square(const real_t *a) { return os_RealMul(a, a); }

real_t tenPow(const real_t *a) { return os_RealPow(&rTen, a); }

real_t scientificNotation(const real_t *a, const real_t *b) {
    real_t p = os_RealPow(&rTen, b);
    return os_RealMul(a, &p);
}

real_t add(const real_t *a, const real_t *b) { return os_RealAdd(a, b); }

real_t subtract(const real_t *a, const real_t *b) { return os_RealSub(a, b); }

real_t multiply(const real_t *a, const real_t *b) { return os_RealMul(a, b); }

real_t divide(const real_t *a, const real_t *b) { return os_RealDiv(a, b); }

real_t recip(const real_t *a) { return os_RealInv(a); }

real_t log10Real(const real_t *a) {
    real_t ln = os_RealLog(a);
    return os_RealDiv(&ln, &ln10);
}

real_t cust_sin(const real_t *a) {
    if (radians)
        return os_RealSinRad(a);
    real_t rad = os_RealDegToRad(a);
    return os_RealSinRad(&rad);
}

real_t cust_cos(const real_t *a) {
    if (radians)
        return os_RealCosRad(a);
    real_t rad = os_RealDegToRad(a);
    return os_RealCosRad(&rad);
}

real_t cust_tan(const real_t *a) {
    if (radians)
        return os_RealTanRad(a);
    real_t rad = os_RealDegToRad(a);
    return os_RealTanRad(&rad);
}

real_t cust_asin(const real_t *a) {
    real_t val = os_RealAsinRad(a);
    if (radians)
        return val;
    return os_RealRadToDeg(&val);
}

real_t cust_acos(const real_t *a) {
    real_t val = os_RealAcosRad(a);
    if (radians)
        return val;
    return os_RealRadToDeg(&val);
}

real_t cust_atan(const real_t *a) {
    real_t val = os_RealAtanRad(a);
    if (radians)
        return val;
    return os_RealRadToDeg(&val);
}

int main() {
    uint8_t key;

    init_constants();
    new_problem();

    while (!((key = os_GetCSC()) == sk_Graph || (constantsmode && key == sk_Mode))) {
        if (constantsmode) {
            switch (key) {
            case sk_Power:
                stack[idx] = pi;
                constants_mode(false);
                draw_line();
                break;
            case sk_Div:
                stack[idx] = e;
                constants_mode(false);
                draw_line();
                break;
            case sk_Store:
                x = stack[idx];
                constants_mode(false);
                draw_line();
                break;
            case sk_Log:
                UNARY_OP(tenPow);
                constants_mode(false);
                break;
            case sk_Ln:
                UNARY_OP(os_RealExp);
                constants_mode(false);
                break;
            case sk_Sin:
                UNARY_OP(cust_asin);
                constants_mode(false);
                break;
            case sk_Cos:
                UNARY_OP(cust_acos);
                constants_mode(false);
                break;
            case sk_Tan:
                UNARY_OP(cust_atan);
                constants_mode(false);
                break;
            case sk_Square:
                UNARY_OP(os_RealSqrt);
                constants_mode(false);
                break;
            case sk_Comma:
                BINARY_OP(scientificNotation);
                constants_mode(false);
                break;
            case sk_Apps:
                radians = !radians;
                constants_mode(false);
                hint(radians ? "r" : "d");
                break;
            case sk_2nd:
                constants_mode(false);
                break;
            case sk_Del:
                constants_mode(false);
                new_problem();
                break;
            }
        } else {
            if (key == sk_0 || key == sk_1 || key == sk_2 || key == sk_3 ||
                key == sk_4 || key == sk_5 || key == sk_6 || key == sk_7 ||
                key == sk_8 || key == sk_9) {
                int toAddInt = 0;
                if (key == sk_1)
                    toAddInt = 1;
                if (key == sk_2)
                    toAddInt = 2;
                if (key == sk_3)
                    toAddInt = 3;
                if (key == sk_4)
                    toAddInt = 4;
                if (key == sk_5)
                    toAddInt = 5;
                if (key == sk_6)
                    toAddInt = 6;
                if (key == sk_7)
                    toAddInt = 7;
                if (key == sk_8)
                    toAddInt = 8;
                if (key == sk_9)
                    toAddInt = 9;

                real_t rAdd = os_Int24ToReal(toAddInt);

                if (!decimal) {
                    stack[idx] = os_RealMul(&stack[idx], &rTen);
                    if (!negative)
                        stack[idx] = os_RealAdd(&stack[idx], &rAdd);
                    else
                        stack[idx] = os_RealSub(&stack[idx], &rAdd);
                    draw_line();
                } else {
                    rAdd = os_RealMul(&rAdd, &decimalfactor);
                    if (!negative)
                        stack[idx] = os_RealAdd(&stack[idx], &rAdd);
                    else
                        stack[idx] = os_RealSub(&stack[idx], &rAdd);
                    decimalfactor = os_RealDiv(&decimalfactor, &rTen);

                    draw_line();
                }
            } else if (key == sk_Chs) {
                stack[idx] = os_RealNeg(&stack[idx]);
                negative = !negative;
                draw_line();
            } else if (key == sk_DecPnt) {
                if (!decimal) {
                    decimal = true;
                    decimalfactor = os_RealDiv(&rOne, &rTen);
                    drawdecimal_line();
                }
            } else if (key == sk_Clear) {
                new_entry();
            } else if (key == sk_Left) {
                if (negative)
                    stack[idx] = os_RealNeg(&stack[idx]);
                if (!decimal) {
                    stack[idx] = os_RealDiv(&stack[idx], &rTen);
                } else
                    decimal = false;

                stack[idx] = os_RealFloor(&stack[idx]);

                if (negative)
                    stack[idx] = os_RealNeg(&stack[idx]);
                draw_line();
            } else if (key == sk_Up) {
                if (idx >= 1) {
                    delete_stack(idx - 1);
                    stack[idx - 1] = stack[idx];
                    stack[idx] = rZero;
                    idx--;
                }
            } else if (key == sk_Enter) {
                if (idx >= 99) {
                    new_problem();
                } else {
                    draw_stack(idx++);
                    L1[idx] = stack[idx - 1];
                    new_entry();
                }
            } else if (key == sk_Mode) {
                scimode = !scimode;
                draw_full_stack();
            } else if (key == sk_Del) {
                new_problem();
            } else if (key == sk_Add) {
                BINARY_OP(add);
            } else if (key == sk_Sub) {
                BINARY_OP(subtract);
            } else if (key == sk_Mul) {
                BINARY_OP(multiply);
            } else if (key == sk_Div) {
                BINARY_OP(divide);
            } else if (key == sk_Power) {
                BINARY_OP(os_RealPow);
            } else if (key == sk_Log) {
                UNARY_OP(log10Real);
            } else if (key == sk_Ln) {
                UNARY_OP(os_RealLog);
            } else if (key == sk_Sin) {
                UNARY_OP(cust_sin);
            } else if (key == sk_Cos) {
                UNARY_OP(cust_cos);
            } else if (key == sk_Tan) {
                UNARY_OP(cust_tan);
            } else if (key == sk_Square) {
                UNARY_OP(square);
            } else if (key == sk_Recip) {
                UNARY_OP(recip);
            } else if (key == sk_Comma) {
                BINARY_OP(scientificNotation);
            } else if (key == sk_Store) {
                if (!is_zero(stack[idx])) {
                    x = stack[idx];
                    hint(">");
                } else {
                    if (idx >= 1) {
                        x = stack[idx - 1];
                        hint(">");
                    }
                }
            } else if (key == sk_2nd) {
                constants_mode(true);
            } else if (key == sk_Prgm && idx >= 2) {
                // Swap keys
                real_t temp = stack[idx - 1];
                stack[idx - 1] = stack[idx - 2];
                stack[idx - 2] = temp;
                draw_stack_clear(idx - 2, true);
                draw_stack_clear(idx - 1, true);
                L1[idx - 1] = stack[idx - 1];
                L1[idx - 2] = stack[idx - 2];
            } else if (key == sk_Yequ) {
                os_ClrHome();
                os_SetCursorPos(0, 0);
                os_PutStrFull("Arjun's RPN Calculator");
                os_SetCursorPos(1, 0);
                os_PutStrFull("v2.3 (ASM)");
                os_SetCursorPos(3, 0);
                os_PutStrFull("Visit git.io/ti84rpn");
                os_SetCursorPos(4, 0);
                os_PutStrFull("for help and more info");
                while (os_GetCSC() == 0)
                    ;
                os_ClrHome();
                draw_full_stack();
                draw_line();
            } else if (key == sk_Window) {
                const char *keys[] = {
                    "^  Pop stack",          "<  Backspace",
                    "del  Clear stack",      "mode  Switch notation",
                    "sto>  Stores value",    "2nd sto>  Retrieves value",
                    "2nd apps  Deg/radians", "graph / 2nd mode  Exit",
                    "clear  Clear value",    "enter  Push value to stack",
                    ",  SCI binary operator"};
                draw_lines(keys);
            }
        }
    }

    return 0;
}
