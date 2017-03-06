
int main(int argc, char* argv) {
    int x = 5;
    int y = 10;
    if (x + y > 16) {
        int z = 7;
        x = z - y;
    } else x = 2;
    return x;
}
