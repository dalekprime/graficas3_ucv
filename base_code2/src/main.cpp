#include "3DViewer.h"
#include <iostream>

int main() {
    C3DViewer main;
    if (!main.setup()) {
        fprintf(stderr, "Failed to setup C3DViewer\n");
        return -1;
    }
    main.mainLoop();
    return 0;
}
