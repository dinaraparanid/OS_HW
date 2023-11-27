#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <linux/input.h>

typedef struct input_event input_event;

typedef struct {
    /** Key codes (max is 6) */
    int keys[6];

    /** Number of keys in the shortcut */
    int size;

    /** On shortcut entered callback */
    void (*const on_shortcut)();
} shortcut;

const int KEY_RELEASED = 0;
const int KEY_PRESSED = 1;
const int KEY_REPEATED = 2;

/** Observable events */

const char* const events[3] = {
        "RELEASED",
        "PRESSED ",
        "REPEATED"
};

/** Successfully exits the program */

void on_shortcut_exit() { exit(EXIT_SUCCESS); }

/** E + X shortcut - successfully exits program */

shortcut shortcut_exit() {
    shortcut ex = {
            .keys = { KEY_E, KEY_X },
            .size = 2,
            .on_shortcut = on_shortcut_exit
    };

    return ex;
}

/** Prints "I passed the Exam!" message */

void on_shortcut_pass_exam() { puts("I passed the Exam!"); }

/** P + E shortcut - prints "I passed the Exam!" message */

shortcut shortcut_pass_exam() {
    shortcut ex = {
            .keys = { KEY_P, KEY_E },
            .size = 2,
            .on_shortcut = on_shortcut_pass_exam
    };

    return ex;
}

/** Prints "Get some cappuccino!" message */

void on_shortcut_cappuccino() { puts("Get some cappuccino!"); }

/** C + A + P shortcut - prints "Get some cappuccino!" message */

shortcut shortcut_cappuccino() {
    shortcut ex = {
            .keys = { KEY_C, KEY_A, KEY_P },
            .size = 3,
            .on_shortcut = on_shortcut_cappuccino
    };

    return ex;
}

/** Kills the program with an error */

void on_shortcut_bebra() {
    puts("Nuhay bebru!");
    exit(EXIT_FAILURE);
}

/** N + B shortcut - kills the program with an error */

shortcut shortcut_bebra() {
    shortcut ex = {
            .keys = { KEY_N, KEY_B },
            .size = 2,
            .on_shortcut = on_shortcut_bebra
    };

    return ex;
}

/**
 * Converts key's code to the symbol
 *
 * Example:
 * assert(key_str(KEY_A) == 'A')
 */

char key_str(const int key) {
    switch (key) {
        case KEY_A: return 'A';
        case KEY_B: return 'B';
        case KEY_C: return 'C';
        case KEY_E: return 'E';
        case KEY_N: return 'N';
        case KEY_P: return 'P';
        case KEY_X: return 'X';
        default: return '?';
    }
}

/**
 * Prints shortcut in for format:
 * K1 + K2 + ... + Kn
 */

void print_shortcut(const shortcut sh) {
    for (const int* key = sh.keys; key != sh.keys + sh.size - 1; ++key)
        printf("%c + ", key_str(*key));
    printf("%c", key_str(sh.keys[sh.size - 1]));
}

/**
 * Prints shortcut list in format:
 * <index>. <shortcut_keys>
 * @see print_shortcut()
 */

void print_shortcuts(
        const shortcut* shortcuts,
        const long shortcuts_size
) {
    for (long i = 0; i < shortcuts_size; ++i) {
        printf("%ld. ", i + 1);
        print_shortcut(shortcuts[i]);
        putchar('\n');
    }
}

/**
 * Checks if user has entered any required shortcuts.
 * If so, executes shortcut's callback
 */

void on_key_pressed(
        const int* const pressed_keys,
        const int pressed_keys_size,
        const shortcut* shortcuts,
        const long shortcuts_size
) {
    for (const shortcut* sh = shortcuts; sh != shortcuts + shortcuts_size; ++sh)
        if (
                sh->size == pressed_keys_size &&
                memcmp(pressed_keys, sh->keys, pressed_keys_size * sizeof(int)) == 0
        ) sh->on_shortcut();
}

int main() {
    const int kbd = open("/dev/input/by-path/platform-i8042-serio-0-event-kbd", O_RDONLY);

    if (kbd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    input_event ev;

    // All possible shortcuts
    const shortcut shortcuts[] = {
            shortcut_exit(),
            shortcut_pass_exam(),
            shortcut_cappuccino(),
            shortcut_bebra()
    };

    const long shortcuts_size = sizeof shortcuts / sizeof(shortcut);

    print_shortcuts(shortcuts, shortcuts_size);

    // Before processing shortcuts,
    // it is required to flush stdout,
    // because it may trigger callbacks
    fflush(stdout);

    // Writing everything to ex1.txt
    freopen("ex1.txt", "a", stdout);

    int pressed_keys[6], pressed_keys_size = 0;

    for (;;) {
        const ssize_t n = read(kbd, &ev, sizeof(input_event));

        if (n < sizeof(input_event)) {
            perror("read");
            return EXIT_FAILURE;
        }

        if (ev.type == EV_KEY && ev.value >= KEY_RELEASED && ev.value <= KEY_REPEATED) {
            printf("%s 0x%04x (%d)\n", events[ev.value], ev.code, ev.code);

            if (ev.value == KEY_PRESSED) {
                // Adding the key to the list if it is pressed
                pressed_keys[pressed_keys_size] = ev.code;
                pressed_keys_size = (pressed_keys_size + 1) % 6;
                on_key_pressed(pressed_keys, pressed_keys_size, shortcuts, shortcuts_size);
            } else if (ev.value == KEY_RELEASED) {
                // "Clearing" the list by seeking to the beginning of it
                pressed_keys_size = 0;
            }
        }
    }
}