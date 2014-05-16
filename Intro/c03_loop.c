#include <stdio.h>

int main() {
	int count;
	puts("Please enter a number: ");
	scanf("%d", &count);
	printf("The number is %d \n", count);
	for (; count >= 0; --count) {
		printf("The number is now %d \n", count);
		if (count == 0) {
			printf("BOOM!\n");
		}
	}
}
