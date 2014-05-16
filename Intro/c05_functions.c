#include <stdio.h>

int foo(int theNum);

int main() {
	int fNum, sNum, mNum;
	printf("Please enter a number: ");
	scanf("%d", &fNum);
	printf("Your number is: %d\n", fNum);
	sNum = foo(fNum);
	printf("Your number plus two is: %d\n", sNum);
}

int foo(int theNum) {
	int newNum;
	newNum = theNum + 2;
	return(newNum);
}