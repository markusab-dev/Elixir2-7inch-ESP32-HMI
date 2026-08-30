#include <Arduino.h>

#include "app/AppController.h"

namespace {
elixir::AppController app;
}

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
