// web_routes.ino
// Registro de rutas HTTP servidas desde SPIFFS.
//
// Antes había 7 handlers casi idénticos en setup() (abrir archivo de SPIFFS,
// chequear error, streamFile, cerrar). Se dejó un helper serveFile() para
// las páginas HTML (cuya URL no coincide 1:1 con la ruta en SPIFFS), y
// server.serveStatic() para /CSS y /JS, que sirve carpetas enteras sin
// necesidad de un handler por archivo -- cualquier archivo nuevo que se
// agregue a data/CSS o data/JS queda servido automáticamente.

// Sirve un único archivo de SPIFFS en una URL dada, con su mime type.
static void serveFile(const char* urlPath, const char* filePath, const char* mime) {
    server.on(urlPath, HTTP_GET, [filePath, mime]() {
        File file = SPIFFS.open(filePath, "r");
        if (!file) {
            Serial.printf("❌ Error al abrir %s\n", filePath);
            server.send(500, "text/plain", String("Error al abrir ") + filePath);
            return;
        }
        server.streamFile(file, mime);
        file.close();
    });
}

void registerHttpRoutes() {
    // Páginas HTML (la URL no es un espejo directo de la carpeta data/html)
    serveFile("/", "/html/index.html", "text/html");
    serveFile("/panelEstudiante.html", "/html/panelEstudiante.html", "text/html");
    serveFile("/panelDocente.html", "/html/panelDocente.html", "text/html");
    serveFile("/generaInforme.html", "/html/generaInforme.html", "text/html");

    // CSS y JS: se sirven directo desde SPIFFS, un solo handler cubre
    // toda la carpeta (incluye archivos que se agreguen a futuro).
    server.serveStatic("/CSS", SPIFFS, "/CSS");
    server.serveStatic("/JS", SPIFFS, "/JS");
}
