/* ==========================================================================
   PEPE — Cliente WebSocket compartido
   --------------------------------------------------------------------------
   Conexión única a la ESP32 (ws://192.168.10.1:81), reutilizada por todas
   las páginas que necesitan datos en vivo (panelDocente, panelEstudiante,
   parametros...). Antes cada página copiaba y pegaba su propio bloque de
   conexión; ahora está en un solo lugar.

   USO:
     const pepe = crearConexionPepe({
       onStatusChange: (estado) => { ... },  // 'conectando' | 'conectado' | 'error'
     });
     pepe.on('ventilacion', (data) => { ... });
     pepe.send({ type: 'envioComandoaESP', estadoConexion: 'start' });

   Si la ESP32 ya tiene 3 conexiones activas, rechaza la conexión nueva
   apenas se abre — por eso 'onStatusChange' informa 'error' también en
   ese caso (no hay forma de distinguirlo de un error de red genérico
   desde el lado del navegador, así que el mensaje que se muestre al
   usuario conviene que sea genérico: "no se pudo conectar").
   ========================================================================== */

function crearConexionPepe({ onStatusChange = () => {} } = {}) {
  const socket = new WebSocket('ws://192.168.10.1:81');
  const listeners = {};

  socket.onopen = () => onStatusChange('conectado');
  socket.onerror = () => onStatusChange('error');
  socket.onclose = () => onStatusChange('error');

  socket.onmessage = (event) => {
    let data;
    try {
      data = JSON.parse(event.data);
    } catch (e) {
      console.warn('Mensaje WebSocket no es JSON válido:', event.data);
      return;
    }
    const callbacks = listeners[data.type];
    if (callbacks) callbacks.forEach((cb) => cb(data));
  };

  return {
    /** Suscribirse a un tipo de mensaje (ver el listado de "type" que manda funciones.ino) */
    on(type, callback) {
      if (!listeners[type]) listeners[type] = [];
      listeners[type].push(callback);
    },

    /** Mandar un mensaje a la ESP32 (se serializa a JSON automáticamente) */
    send(obj) {
      if (socket.readyState === WebSocket.OPEN) {
        socket.send(JSON.stringify(obj));
      } else {
        console.warn('No se pudo enviar, el socket no está abierto:', obj);
      }
    },

    raw: socket,
  };
}
