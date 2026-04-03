#include "app.h"

int main(void) {
    // Inicializa todos os periféricos e estados do sistema.
    app_init();
    // Mantém a aplicação executando continuamente.
    app_run_forever();
    return 0;
}
