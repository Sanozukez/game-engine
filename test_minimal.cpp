// Test minimal
#include <iostream>
#include <Windows.h>

int main() {
    MessageBoxA(NULL, "Test executable works!", "Success", MB_OK);
    std::cout << "Hello from minimal test!" << std::endl;
    return 0;
}
