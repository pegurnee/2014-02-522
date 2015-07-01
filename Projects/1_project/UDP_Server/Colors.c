
#include <stdio.h>
#include "Colors.h"

void practiceColors() {
    printf("%sred\n", KRED);
    printf("%sgreen\n", KGRN);
    printf("%syellow\n", KYEL);
    printf("%sblue\n", KBLU);
    printf("%smagenta\n", KMAG);
    printf("%scyan\n", KCYN);
    printf("%swhite\n", KWHT);
    printf("%snormal\n", KRES);
}

void printRed(char *str) {
    printf("%s%s%s", KRED, str, KRES);
}

void printGrn(char *str) {
    printf("%s%s%s", KGRN, str, KRES);
}

void printYel(char *str) {
    printf("%s%s%s", KYEL, str, KRES);
}

void printBlu(char *str) {
    printf("%s%s%s", KBLU, str, KRES);
}

void printMag(char *str) {
    printf("%s%s%s", KMAG, str, KRES);
}

void printCyn(char *str) {
    printf("%s%s%s", KCYN, str, KRES);
}

void printWht(char *str) {
    printf("%s%s%s", KWHT, str, KRES);
}

void colorRed() {
    printf("%s", KRED);
}

void colorGrn() {
    printf("%s", KGRN);
}

void colorYel() {
    printf("%s", KYEL);
}

void colorBlu() {
    printf("%s", KBLU);
}

void colorMag() {
    printf("%s", KMAG);
}

void colorCyn() {
    printf("%s", KCYN);
}

void colorWht() {
    printf("%s", KWHT);
}

void colorRes() {
    printf("%s", KRES);
}
