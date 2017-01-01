#include <stdio.h>
// Comment Comment
void dummy() {
    int t;
    t = 1;
    if (t == 1) {
        ;
    } else {
        ;
    }
    if (t == 0) {
        ;
    } else {
        ;
    }
    return; // in-line comment
}

int fibonacci(int i){
    if (i <= 1) 
    {
        return 1;
    }
    return fibonacci(i-1) + fibonacci(i-2);
}

int main(){
    int i;
    i=0;
    while (i < 10) {
        printf("fibonacci(%d) = %d\n", i, fibonacci(i));
        i = i + 1;
    }
    dummy();

    return 0;
}
