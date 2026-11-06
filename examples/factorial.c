int fact(int n) {
    if (n <= 1) return 1;
    else return n * fact(n - 1);
}

int main() {
    int s = 0;
    int i;
    for (i = 0; i < 3; i = i + 1) s = s + i;
    while (s > 0) { s = s - 1; }
    return fact(5);
}
