int sum(int a, int b) { return a + b; }

int main() {
    int x = 10;
    {
        int x = 20;
        print(x);
    }
    print(x);
    int i;
    for (i = 1; i <= 3; i = i + 1) print(sum(i, 100));
    return 0;
}
