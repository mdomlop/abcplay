// https://abcnotation.com/wiki/abc:standard:v2.1
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>  // Para nanosleep
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MUSIC "ABCDEFG abcdefg"
#define DEFDURATION 150  // milisegundos

// Defaults to EVDEV API:
#define DEVICE "/dev/input/by-path/platform-pcspkr-event-spkr"

// Colores de las notas
/* ANSI COLORS: */
#define RESET   "\x1b[0m"
#define ZCOLOR  "\x1b[1;100m"  // blueb

#define CCOLOR  "\x1b[40m\x1b[1;31m"  // rojo
#define DCOLOR  "\x1b[40m\x1b[1;32m"  // verde
#define ECOLOR  "\x1b[40m\x1b[1;33m"  // amarillo
#define FCOLOR  "\x1b[40m\x1b[1;34m"  // azul
#define GCOLOR  "\x1b[40m\x1b[1;35m"  // magenta
#define ACOLOR  "\x1b[40m\x1b[1;36m"  // cyan
#define BCOLOR  "\x1b[40m\x1b[1;37m"  // blanco

#define A 440.00  // A4

struct Note {
    char name;
    float freq;
    int acc;
    int octave;
    int duration;
    char color[20];
};


#define ABCNOTES "CDEFGABcdefgabcz"
#define ABCACCIDENTALS "^=_"     // Sostenido, natural y bemol
#define ABCOCTAVE ",'"           // Baja o sube una octava a la nota
#define ABCDURATION "/123456789" // Duración de las notas


// Función para emitir un sonido usando evdev
void beep(float freq, int duration_ms) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = duration_ms * 1000000;
    int fd = open(DEVICE, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo abrir el dispositivo de evdev");
        exit(EXIT_FAILURE);
    }

    struct input_event ev;
    ev.type = EV_SND; // Tipo de evento: sonido
    ev.code = SND_TONE; // Código del evento: tono
    ev.value = freq; // Frecuencia del tono

    // Enviar evento para iniciar el sonido
    if (write(fd, &ev, sizeof(ev)) == -1) {
        perror("Error al escribir el evento de sonido");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //usleep(duration_ms * 1000); // Duración del tono
    nanosleep(&ts, NULL);

    // Detener el sonido
    ev.value = 0; // Frecuencia 0 para apagar
    if (write(fd, &ev, sizeof(ev)) == -1) {
        perror("Error al apagar el sonido");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

int checkchar(char c, char *s) {
	/* Verifica que un caracter esté en una cadena */
	int i;
	int end = strlen(s);

	for (i=0; i<end; i++)
		if (c == s[i]) return 1;
    return 0;
}

void printnote(struct Note note) {
    //printf("%s%c:%f " RESET, note.color, note.name, note.freq);
    printf("%s%c" RESET, note.color, note.name);
}

int getduration(char *s, int size, int start) {
    int factor; // o divisor
    int i = start;
    int mysize = 0;  // Voy a meter los números en esta cadena.
    const int mymaxsize = 3;
    char mystring[3]; // 999 es un divisor suficientemente grande


    for (;i<size; i++) {
        if (checkchar(s[i], "123456789")) {  // FIXME: Ya sé que me falta el "/"
            mysize++;
            if (mysize > mymaxsize) {
                perror("Sorry, maximum duration is 999.");
                mysize--;
            }
        }
        else { break; }  // Ya no hay más números
    }

    // Cortamos la cadena.
    strncpy(mystring, s + start, i-start);
    factor = atoi(mystring);

    return factor;
}

float getfreq(char c, int acc, int octave) {
    /* Obtiene la frecuencia de una nota */
    const double semi_s = pow(2.0, 1.0 / 12.0);
    const int mynotes = 12;  // Número de notas en la escala cromática. Para octavar.
    int semitones = 0;
    float freq;

    if (c >= 'a' && c < 'h') { octave++; }
    c = toupper(c);
    if (c == 'Z') freq = 0.0;
    else {
        switch (c) {
            case 'A':
                semitones = 0;
                break;
            case 'B':
                semitones = 2;
                break;
            case 'C':
                semitones = 3;
                break;
            case 'D':
                semitones = 5;
                break;
            case 'E':
                semitones = 7;
                break;
            case 'F':
                semitones = 8;
                break;
            case 'G':
                semitones = 10;
                break;
            case '\0':
                printf("Saliendo...\n");
                return 0;
            default:
                printf("Nota no válida: '%c'\n", c);
                return 1;
        }
    }

    semitones += octave * mynotes + acc;

    freq = A * pow(semi_s, semitones);
    return freq;
}

int getacc(int index, char *s) {
    return 0;
}

int getoctave(int index, char *s) {

    return 0;
}

char * getcolor(char c) {
    char name = toupper(c);
        switch (name) {
            case 'C':
                return CCOLOR;
                break;
            case 'D':
                return DCOLOR;
                break;
            case 'E':
                return ECOLOR;
                break;
            case 'F':
                return FCOLOR;
                break;
            case 'G':
                return GCOLOR;
                break;
            case 'A':
                return ACOLOR;
                break;
            case 'B':
                return BCOLOR;
                break;
            case 'z':
                return ZCOLOR;
                break;
            case '\0':
                printf("Saliendo...\n");
                return ZCOLOR;
            default:
                printf("Nota no válida: '%c'\n", name);
                return ZCOLOR;
        }
    return ZCOLOR;
}


int play(char *s) {
	int i;
	int end = strlen(s);
	char c;
    //int divide = 0;  // Si es 1 divide por factor, si no, multiplica.

    struct Note note;

	for (i=0; i<end; i++) {
		c = s[i];
        note.duration = DEFDURATION;

        if (checkchar(c, ABCNOTES)) {
            // Asignamos la nota encontrada
            note.name = c;
            note.acc = getacc(i, s);
            note.octave = getoctave(i, s);
            note.freq = getfreq(note.name, note.acc, note.octave);
            strcpy(note.color, getcolor(note.name));

        }
        else if (checkchar(c, ABCDURATION)) {
            if (c != '/') note.duration = DEFDURATION * 2; // != Porque es más común multiplicar
            else note.duration = DEFDURATION / 2;
            printf("%c", c);
            continue;
        }
        else  { printf("%c", c); continue; }


        //printf("%c:", c);
        beep(note.freq, note.duration);
        printnote(note);
        fflush(stdout);
    }
    puts("");
    return 0;
}

int main() {
	play(MUSIC);
    return 0;
}
