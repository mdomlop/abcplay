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


#define DEFDURATION 100  // milisegundos

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


#define MAXSIZE 20

struct Note {
    char name;
    float freq;
    int acc;
    char accname;
    int octave;
    char octavename[MAXSIZE];
    int duration;  // milisegundos
    char color[MAXSIZE];
} note;


#define ABCNOTES "ABCDEFGabcdefg"
#define ABCACCIDENTALS "_=^"     // Sostenido, natural y bemol
#define ABCOCTAVE ",'"           // Baja o sube una octava a la nota
#define ABCDURATION "/123456789" // Duración de las notas


// Función para emitir un sonido usando evdev
void beep(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = note.duration * 1000000;
    int fd = open(DEVICE, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo abrir el dispositivo de evdev");
        exit(EXIT_FAILURE);
    }

    struct input_event ev;
    ev.type = EV_SND; // Tipo de evento: sonido
    ev.code = SND_TONE; // Código del evento: tono
    ev.value = note.freq; // Frecuencia del tono

    // Enviar evento para iniciar el sonido
    if (write(fd, &ev, sizeof(ev)) == -1) {
        perror("Error al escribir el evento de sonido");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //usleep(note.duration * 1000); // Duración del tono
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

int checkmaxsize(int n) {
    if (n<20) return 1;
    return 0;
}

int checkchar(char c, char *s) {
	/* Verifica que un caracter esté en una cadena */
	int i;
	int end = strlen(s);

	for (i=0; i<end; i++)
		if (c == s[i]) return 1;
    return 0;
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

int setfreq(void) {
    /* Obtiene la frecuencia de una nota */
    const double semi_s = pow(2.0, 1.0 / 12.0);
    const int mynotes = 12;  // Número de notas en la escala cromática. Para octavar.
    int semitones = 0;
    char c = toupper(note.name); // name aux

    if (note.name >= 'a' && c < 'h') { note.octave++; }
    if (c == 'Z') note.freq = 0.0;
    else {
        switch (c) {
            case 'A':
                semitones = 0;
                break;
            case 'B':
                semitones = 2;
                break;
            case 'C':
                semitones = -9;
                break;
            case 'D':
                semitones = -7;
                break;
            case 'E':
                semitones = -5;
                break;
            case 'F':
                semitones = -4;
                break;
            case 'G':
                semitones = -2;
                break;
            case '\0':
                printf("Saliendo...\n");
                return 1;
            default:
                printf("Nota no válida: '%c'\n", c);
                return 0;
        }
    }

    semitones += note.octave * mynotes + note.acc;

    note.freq = A * pow(semi_s, semitones);
    return 1;
}

int * getmultiacc(char *s, int size, int start) {  // FIXME, please
    /* Devuelve los semitonos a subir o bajar a la nota*/
    static int acc[2];
    int factor = 0;  // Para determinar si es sostenido o bemol
    int i = start + 1;  // i es la nota, los accidentales vienen después
    for (;i<size; i++) { // Bucle. Al parecer es válido: C^^
        if (s[i] == '^') factor += 1;
        else if (s[i] == '_') factor -= 1;
        else if (s[i] == '=') factor = 0;
        else {
            acc[0] = 0;
            acc[1] = start;
            return acc;
        }
    }
    acc[0] = factor;
    acc[1] = i;  // El índice por donde debe proseguir el análisis de la cadena s

    return acc;
}

int setacc(char *s, int start) {
    /* Establece los semitonos a subir o bajar a la nota*/
    int previous = start - 1;  // i es la nota, los accidentales vienen después
    if (previous >= 0) {
        char acc = s[previous];  // Sé que es ineficiente, pero claro.
        if      (acc == '^') { note.accname = acc; note.acc = 1;  return 0; }
        else if (acc == '_') { note.accname = acc; note.acc = -1; return 0; }
        else if (acc == '=') { note.accname = acc; note.acc = 0;  return 0; }
        else return 0;  // ¿Por qué no?
    }
    return 1;
}

int setduration(char *s, int size, int start) {
    /* Establece los semitonos a subir o bajar a la nota*/
    int next = start + 1;  // i es la nota, los accidentales vienen después
    int duration = 1;
    char c;
    int div = 0;

    if (next < size && checkchar(s[next], ABCOCTAVE)) next++;

    if (next < size && s[next] == '/') {
        div = 1;
        next++;
        duration = 2;
    }

    if (next < size && checkchar(s[next], "0123456789")) {
        c = s[next];
        duration = c-'0';  // to int
    }

    //printf("duration: %d\n", duration);
    if (div) { note.duration = DEFDURATION / duration; return 2; }
    else { note.duration = DEFDURATION * duration; return 1; }

    return 0;
}

int setmultiduration(char *s, int size, int start) {
    printf("#### Empiezo en index %d: %c #####\n", start, s[start]);
    /* Establece la duración de la nota */
    char str_int[10];
    int dur_idx = start;  // i es la nota, la duración viene después
    int steps = 0;
    int factor = 1;

    while (dur_idx + 1 < size) {
        if  (checkchar(s[dur_idx + 1], "0123456789")) {
            str_int[steps] = s[dur_idx + 1];
            dur_idx++; steps++;
        }
        else break;
    }

    factor = atoi(str_int);
    note.duration = DEFDURATION * factor;

    //printf("Igual: %c. Me voy\n", s[start]);
    return steps;
}



int setoctave(char *s, int size, int start) {
    int next;
    int octave = 0;
    int i = 0;

    for (next = start + 1; next < size; next++) {
        //if (checkmaxsize(i)) return 0;
        if (s[next] == '\'') { note.octavename[i] = s[next]; octave++; i++; }
        else if (s[next] == ',') { note.octavename[i] = s[next]; octave--; i++; }
        else break;
    }
    note.octavename[i] = '\0';
    //printf("Octave: %d\n", octave);
    //printf("Octavename: %s\n", note.octavename);
    note.octave = octave;
    return 0;
}

int setcolor(void) {
    char name = toupper(note.name);
        switch (name) {
            case 'C':
                strcpy(note.color, CCOLOR);
                break;
            case 'D':
                strcpy(note.color, DCOLOR);
                break;
            case 'E':
                strcpy(note.color, ECOLOR);
                break;
            case 'F':
                strcpy(note.color, FCOLOR);
                break;
            case 'G':
                strcpy(note.color, GCOLOR);
                break;
            case 'A':
                strcpy(note.color, ACOLOR);
                break;
            case 'B':
                strcpy(note.color, BCOLOR);
                break;
            case 'z':
                strcpy(note.color, ZCOLOR);
                break;
            case '\0':
                printf("Saliendo...\n");
                return 1;
            default:
                printf("Nota no válida: >%c<\n", note.name);
                return 0;
        }
    return 1;
}


void debugnote (char *s, int i) {
    printf("index: %d char: %c\nname: %c\nfreq: %f\n"
    "acc: %d\naccname: %c\noctave: %d\noctavename: %s\nduration: %d\n",
            i, s[i],
            note.name,
            note.freq,
            note.acc,
            note.accname,
            note.octave,
            note.octavename,
            note.duration
            );
}

int strcopy(char *sdest, char *sorig, int start, int end) {
    int size = strlen(sorig);
    int dest_idx = 0;
    if (start > size || end > size) return 0;
    for (;start<end; start++) {
        sdest[dest_idx] = sorig[start];
        dest_idx++;
    }
    printf("Copiado: %s\n", sdest);
    return dest_idx; // devuelve el número de caracteres copiados
}

void printnote(void) {
    printf("%s%c%c%s (%d): %f\n" RESET,
            note.color, note.accname, note.name, note.octavename,
            note.duration, note.freq);
    //printf("%s%c%c:%f\n" RESET, note.color, note.name, note.accname, note.freq);
    //printf("%s%c" RESET, note.color, note.name);
}

int play(char *s) {
	int i;
	int end = strlen(s);
	char c;
    //int divide = 0;  // Si es 1 divide por factor, si no, multiplica.

	for (i=0; i<end; i++) {
		c = s[i];
        note.name = '\0';
        note.freq = 0.0;
        note.acc  = 0;
        note.accname = '\0';
        note.octave = 0;
        note.duration = DEFDURATION;
        strcpy(note.color, RESET);

        if (checkchar(c, ABCNOTES)) {
            // Asignamos la nota encontrada
            note.name = c;
            setcolor();
            setacc(s, i);
            setoctave(s, end, i);
            setduration(s, end, i);
            setfreq();
            //debugnote(s, i);

        }
        else if (checkchar(c, ABCACCIDENTALS)) continue;
        else if (checkchar(c, ABCDURATION)) continue;
        else if (checkchar(c, ABCOCTAVE)) continue;
        else if (c == ' ') continue;
        else  {  // Carácter no reconocido
            printf(RESET "[Error] Caracter desconocido: >%c<\n", c);
            return 0;
        }


        printnote();
        beep();
        fflush(stdout);
    }
    puts("");
    return 1;
}

int main() {
    //FIXME No funciona c'2
    //char MUSIC[] = "^C,,,,,,_C,,,,,C,,,,C,,,C,,C,Ccc'c''c'''c''''c'''''c''''''";
    char MUSIC[] = "c''''/";
    //char MUSIC[] = "E2EB BAFD EBEB ADFD E2EB BAFA BcdB ADFA E2EB BAFD EBEB ADFD E2EB BAFA BcdB ADFA B2eB fBeB B2dB ADFA Bcef gfge dcdB ADFA B2GB FBEB B2dB ADFD B2GB FBDB dcdB ADFA B2GB FBEB B2dB ADFA BAGF EFGA BcdB ADFD";

	play(MUSIC);
    return 0;
}
