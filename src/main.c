#include "stdint.h"
#include "stdio.h"
#include <math.h>
#include <string.h>
#include <tice.h>

// <fileioc.h>
#define ti_X ("\x58\0\0")
#define ti_L1 ("\x5D\x0\0")

float stack[101]; // We want much more precision than real_t can give us

// real_t stack[101];
char buffer[50];
uint8_t idx;
bool decimal;
bool negative;
bool constantsmode = false;
bool scimode = false;
bool radians = true;
float decimalfactor;

float ln10, pi, e;

void init_constants() {
    ln10 = logf(10);
    pi = acosf(-1);
    e = expf(1);
}

float x, L1[100];

real_t r_0, r_1, r_2, r_3, r_4, r_5, r_6, r_7, r_8, r_9, r_10, r_n1, r_ln10, r_pi, r_e;

void init_real_constants() {
    r_0 = os_Int24ToReal(0);
    r_1 = os_Int24ToReal(1);
    r_2 = os_Int24ToReal(2);
    r_3 = os_Int24ToReal(3);
    r_4 = os_Int24ToReal(4);
    r_5 = os_Int24ToReal(5);
    r_6 = os_Int24ToReal(6);
    r_7 = os_Int24ToReal(7);
    r_8 = os_Int24ToReal(8);
    r_9 = os_Int24ToReal(9);
    r_10 = os_Int24ToReal(10);
    r_n1 = os_Int24ToReal(-1);
    r_ln10 = os_RealLog(&r_10);
    r_pi = os_FloatToReal(3.141593);
    r_e = os_RealExp(&r_1);
}

void float_to_str(float f, char *buffer) {
    real_t r = os_FloatToReal(f);
    os_RealToStr(buffer, &r, 0, 1, -1);
}

void draw_line() {
    strcpy(buffer, "                    ");
    float_to_str(stack[idx], buffer);
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
        float_to_str(stack[row], buffer);
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
    stack[idx] = 0;
    hint(" ");
    draw_line();
    // The new line doesn't overwrite the second char so manually fix that
    os_SetCursorPos(9, 1);
    os_PutStrFull(" ");
}

void new_problem() {
    idx = 0;
    os_ClrHome();
    buffer[0] = 0;
    new_entry();
}

#define BINARY_OP(os_func)                                                             \
    do {                                                                               \
        if (stack[idx] != 0) {                                                         \
            if (idx >= 1) {                                                            \
                stack[idx - 1] = os_func(stack[idx - 1], stack[idx]);                  \
                draw_stack_clear(idx - 1, true);                                       \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        } else {                                                                       \
            if (idx >= 2) {                                                            \
                stack[idx - 2] = os_func(stack[idx - 2], stack[idx - 1]);              \
                draw_stack_clear(idx - 2, true);                                       \
                delete_stack(idx - 1);                                                 \
                idx--;                                                                 \
                L1[idx];                                                               \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        }                                                                              \
    } while (false)

#define UNARY_OP(os_func)                                                              \
    do {                                                                               \
        if (stack[idx] != 0) {                                                         \
            stack[idx] = os_func(stack[idx]);                                          \
            draw_line();                                                               \
        } else {                                                                       \
            if (idx >= 1) {                                                            \
                stack[idx - 1] = os_func(stack[idx - 1]);                              \
                draw_stack_clear(idx - 1, true);                                       \
                L1[idx] = stack[idx - 1];                                              \
                new_entry();                                                           \
            }                                                                          \
        }                                                                              \
    } while (false)

#define REAL_TRIG(name, os_func)                                                       \
    real_t name(real_t *a) {                                                           \
        real_t t;                                                                      \
        if (radians)                                                                   \
            t = *a;                                                                    \
        else                                                                           \
            t = os_RealDegToRad(a);                                                    \
        return os_func(&t);                                                            \
    }

REAL_TRIG(degRadSin, os_RealSinRad)
REAL_TRIG(degRadCos, os_RealCosRad)
REAL_TRIG(degRadTan, os_RealTanRad)

#define REAL_INVTRIG(name, os_func)                                                    \
    real_t name(real_t *a) {                                                           \
        real_t t = os_func(a);                                                         \
        if (!radians)                                                                  \
            t = os_RealRadToDeg(&t);                                                   \
        return t;                                                                      \
    }

REAL_INVTRIG(radDegAsin, os_RealAsinRad)
REAL_INVTRIG(radDegAcos, os_RealAcosRad)
REAL_INVTRIG(radDegAtan, os_RealAtanRad)

real_t realLogBase10(real_t *a) {
    real_t t = os_RealLog(a);
    return os_RealDiv(&t, &r_ln10);
}

float square(float a) { return a * a; }

float tenPow(float a) { return powf(10, a); }

float scientificNotation(float a, float b) { return a * powf(10, b); }

float add(float a, float b) { return a + b; }

float subtract(float a, float b) { return a - b; }

float multiply(float a, float b) { return a * b; }

float divide(float a, float b) { return a / b; }

float recip(float a) { return powf(a, -1); }

int main() {
    uint8_t key;

    init_real_constants();
    init_constants();
    new_problem();

    while (!((key = os_GetCSC()) == sk_Graph || (constantsmode && key == sk_Mode))) {
        if (constantsmode) {
            if (key == sk_Power) {
                stack[idx] = pi;
                constants_mode(false);
                draw_line();
            } else if (key == sk_Div) {
                stack[idx] = e;
                constants_mode(false);
                draw_line();
            } else if (key == sk_Store) {
                x = stack[idx];
                constants_mode(false);
                draw_line();
            } else if (key == sk_Log) {
                UNARY_OP(tenPow);
                constants_mode(false);
            } else if (key == sk_Ln) {
                UNARY_OP(expf);
                constants_mode(false);
            } else if (key == sk_Sin) { // asin and cos no longer switched intentionally
                UNARY_OP(asinf);        // See CE-Programming/toolchain PR #358
                constants_mode(false);
            } else if (key == sk_Cos) {
                UNARY_OP(acosf);
                constants_mode(false);
            } else if (key == sk_Tan) {
                UNARY_OP(atanf);
                constants_mode(false);
            } else if (key == sk_Square) {
                UNARY_OP(sqrtf);
                constants_mode(false);
            } else if (key == sk_Comma) {
                BINARY_OP(scientificNotation);
                constants_mode(false);
            } else if (key == sk_Apps) {
                radians = !radians;
                constants_mode(false);
                hint(radians ? "r" : "d");
            } else if (key == sk_2nd) {
                constants_mode(false);
            } else if (key == sk_Del) {
                constants_mode(false);
                new_problem();
            }
        } else {
            if (key == sk_0 || key == sk_1 || key == sk_2 || key == sk_3 ||
                key == sk_4 || key == sk_5 || key == sk_6 || key == sk_7 ||
                key == sk_8 || key == sk_9) {
                if (!decimal) {
                    stack[idx] = stack[idx] * 10;
                    float toAdd = 0;
                    if (key == sk_1)
                        toAdd = 1;
                    if (key == sk_2)
                        toAdd = 2;
                    if (key == sk_3)
                        toAdd = 3;
                    if (key == sk_4)
                        toAdd = 4;
                    if (key == sk_5)
                        toAdd = 5;
                    if (key == sk_6)
                        toAdd = 6;
                    if (key == sk_7)
                        toAdd = 7;
                    if (key == sk_8)
                        toAdd = 8;
                    if (key == sk_9)
                        toAdd = 9;
                    if (!negative)
                        stack[idx] = stack[idx] + toAdd;
                    else
                        stack[idx] = stack[idx] - toAdd;
                    draw_line();
                } else {
                    float toAdd = 0;
                    if (key == sk_1)
                        toAdd = 1;
                    if (key == sk_2)
                        toAdd = 2;
                    if (key == sk_3)
                        toAdd = 3;
                    if (key == sk_4)
                        toAdd = 4;
                    if (key == sk_5)
                        toAdd = 5;
                    if (key == sk_6)
                        toAdd = 6;
                    if (key == sk_7)
                        toAdd = 7;
                    if (key == sk_8)
                        toAdd = 8;
                    if (key == sk_9)
                        toAdd = 9;
                    toAdd = toAdd * decimalfactor;
                    if (!negative)
                        stack[idx] = stack[idx] + toAdd;
                    else
                        stack[idx] = stack[idx] - toAdd;
                    decimalfactor = decimalfactor / 10;

                    draw_line();
                }
            } else if (key == sk_Chs) {
                stack[idx] = -(stack[idx]);
                negative = !negative;
                draw_line();
            } else if (key == sk_DecPnt) {
                if (!decimal) {
                    decimal = true;
                    decimalfactor = 0.1;
                    drawdecimal_line();
                }
            } else if (key == sk_Clear) {
                new_entry();
            } else if (key == sk_Left) {
                if (negative)
                    stack[idx] = -(stack[idx]);
                if (!decimal) {
                    stack[idx] = stack[idx] / 10;
                } else
                    decimal = false;
                stack[idx] = floorf(stack[idx]);
                if (negative)
                    stack[idx] = -(stack[idx]);
                draw_line();
            } else if (key == sk_Up) {
                if (idx >= 1) {
                    delete_stack(idx - 1);
                    stack[idx - 1] = stack[idx];
                    stack[idx] = 0;
                    idx--;
                    L1[idx];
                }
            } else if (key == sk_Enter) {
                if (idx >= 99) {
                    new_problem();
                } else {
                    draw_stack(idx++);
                    L1[idx];
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
                BINARY_OP(powf);
            } else if (key == sk_Log) {
                UNARY_OP(log10f);
            } else if (key == sk_Ln) {
                UNARY_OP(logf);
            } else if (key == sk_Sin) {
                UNARY_OP(sin);
            } else if (key == sk_Cos) {
                UNARY_OP(cos);
            } else if (key == sk_Tan) {
                UNARY_OP(tan);
            } else if (key == sk_Square) {
                UNARY_OP(square);
            } else if (key == sk_Recip) {
                UNARY_OP(recip);
            } else if (key == sk_Comma) {
                BINARY_OP(scientificNotation);
            } else if (key == sk_Store) {
                if (stack[idx] != 0) {
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
                float temp = stack[idx - 1];
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
                os_ClrHome();

                uint24_t font_height = os_FontGetHeight() + 3;
                uint24_t spacing = font_height * 2; // So starts past bar
                const char *keys[] = {
                    "^  Pop stack",          "<  Backspace",
                    "del  Clear stack",      "mode  Switch notation",
                    "sto>  Stores value",    "2nd sto>  Retrieves value",
                    "2nd apps  Deg/radians", "graph / 2nd mode  Exit",
                    "clear  Clear value",    "enter  Push value to stack",
                    ",  SCI binary operator"};

                for (int i = 0; i < 11; i++) {
                    os_FontDrawText(keys[i], 0, spacing);
                    spacing += font_height;
                }

                while (os_GetCSC() == 0)
                    ;

                os_ClrHome();
                draw_full_stack();
                draw_line();
            }
        }
    }

    return 0;
}
