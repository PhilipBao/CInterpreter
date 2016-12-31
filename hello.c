#include <stdio.h>
// Comment Comment
void dummy() {
    return; // in-line comment
}

int fibonacci(int i){
    if (i <= 1) 
    {
        return 1;
    }
    if (i == 2) printf("i == 2");
    if (i == 3) dummy();
    return fibonacci(i-1) + fibonacci(i-2);
}

int main(){
    int i;
    i = 0;
    int limit = 10;
    while (i < limit) {
        printf("fibonacci(%2d) = %d\n", i, fibonacci(i));
        i = i + 1;
    }
    return 0;
}
