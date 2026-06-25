let startTime = Date.now(); // Tiempo inicial
let elapsedTime = 0; // Tiempo transcurrido acumulado
let intervalId; // ID del intervalo para poder limpiarlo luego
let min=sec=0;
let temporizadorActivo = false;

// Función para actualizar el tiempo y la barra de progreso
function actualizarTiempo() {
    const currentTime = Date.now();
    const diff = currentTime - startTime + elapsedTime;
    const minutes = Math.floor(diff / 60000);
    const seconds = Math.floor((diff % 60000) / 1000);
    let segTot = 0;
    let seguPrueba = parseInt(document.getElementById('duracionPrueba').value) * 60;

    // Actualizar el DOM
    document.getElementById('min').textContent = String(minutes).padStart(2, '0');
    document.getElementById('sec').textContent = String(seconds).padStart(2, '0');
    min = parseInt(minutes);
    sec = parseInt(seconds);

    segTot = min * 60 + sec;
    // Detener el temporizador si se alcanza el tiempo máximo
    if (segTot >= seguPrueba) {
        clearInterval(intervalId); // Detener el intervalo
        //porcentaje = 100; // Asegurar que la barra de progreso llegue al 100%
        document.getElementById('stop').click(); // Simular clic en el botón de detener
    }
}

// Función para iniciar el temporizador
/*function iniciarTemporizador() {
    startTime = Date.now(); // Reiniciar el tiempo inicial
    intervalId = setInterval(actualizarTiempo, 1000); // Actualizar cada segundo
}*/
function detenerTemporizador() {
    console.log("Deteniendo temporizador...");
    clearInterval(intervalId); // Detener el intervalo
    elapsedTime += Date.now() - startTime; // Acumular el tiempo transcurrido
    console.log("Tiempo acumulado:", elapsedTime);
    let temporizadorActivo = false;

}

function iniciarTemporizador() {
    console.log("Iniciando temporizador...");
    startTime = Date.now(); // Reiniciar el tiempo inicial
    intervalId = setInterval(actualizarTiempo, 1000); // Actualizar cada segundo
    temporizadorActivo = true;
}
function reiniciarTemporizador(){
    document.getElementById('min').textContent=0;
    document.getElementById('sec').textContent=0;
}