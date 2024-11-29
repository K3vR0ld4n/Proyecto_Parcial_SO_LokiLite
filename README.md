# Proyecto_Parcial_SO_LokiLite

Este README explica cómo utilizar el sistema de monitoreo de logs que hemos desarrollado. El sistema incluye un **agente**, un **cliente** y un **servidor**, y es capaz de monitorear logs de diferentes servicios de un sistema Linux y enviar alertas a través de WhatsApp cuando se superan ciertos umbrales.

---

### **Requisitos previos**:

- **Twilio API**: Para enviar alertas por WhatsApp, necesitas una cuenta de Twilio. Debes tener tu **`ACCOUNT_SID`**, **`AUTH_TOKEN`**, y los números de teléfono correspondientes.
- **Linux**: El proyecto está diseñado para ser ejecutado en sistemas basados en Linux con `journalctl` disponible para acceder a los logs del sistema.
- **cURL**: Se utiliza la librería `libcurl` para enviar los mensajes de alerta a través de la API de Twilio desde C. Asegúrate de tenerla instalada.

---

### **Estructura del proyecto**:

- **agente.c**: Monitorea los logs de los servicios que se pasan como argumentos y cuenta cuántos logs hay para diferentes niveles de prioridad.
- **cliente.c**: Envía los datos de los logs generados por el **agente** al **servidor**.
- **servidor.c**: Recibe los datos del cliente, verifica si se superan los umbrales de los logs para cada servicio y envía una alerta si es necesario.
- **prueba_estres.c**: Un ejecutable que simula un servicio generando múltiples logs para probar el sistema de alertas. Este programa debe ejecutarse con privilegios de administrador para poder escribir logs.

### **Cómo ejecutar el proyecto**:

#### 1. **Compilación**:

Primero, compila todos los archivos del proyecto utilizando el `Makefile`:

```bash
make
```

Esto generará los ejecutables **`agente`**, **`cliente`**, **`servidor`**, y **`prueba_estres`**.

#### 2. **Ejecución**:

- **Ejecuta el servidor**:
  
  El servidor espera recibir datos del cliente y procesarlos. Ejecuta el servidor en una terminal:

  ```bash
  ./servidor
  ```

- **Ejecuta el cliente**:

  El cliente ejecuta al **agente** en un intervalo y pasa los resultados al servidor. Para ejecutar el cliente, debes proporcionar los servicios que deseas monitorear como argumentos. Por ejemplo:

  ```bash
  ./cliente sshd cron apache2
  ```

  Puedes pasar tantos servicios como desees monitorear.

- **Ejecuta el programa de prueba de estrés**:

  Para probar el sistema de alertas y ver cómo se comporta con logs generados por un servicio ficticio, ejecuta el siguiente comando:

  ```bash
  sudo ./prueba_estres
  ```

  **Nota**: Este programa debe ejecutarse con privilegios de administrador (usando `sudo`) para generar los logs necesarios.

### **Configuración de Twilio para enviar alertas por WhatsApp**:

Antes de ejecutar el servidor, debes configurar las variables de entorno con los datos necesarios de Twilio. Agrega las siguientes líneas a tu archivo **`.bashrc`** o **`.bash_profile`**:

1. Abre tu archivo de configuración (dependiendo de tu shell, usa **.bashrc** o **.bash_profile**):
   
   ```bash
   nano ~/.bashrc  # o ~/.bash_profile
   ```

2. Agrega las siguientes líneas al final del archivo:

   ```bash
   export TWILIO_ACCOUNT_SID="tu_Twilio_account_SID"
   export TWILIO_AUTH_TOKEN="tu_Twilio_auth_token"
   export TWILIO_WHATSAPP_NUMBER="whatsapp:numero_Twilio(por lo general +14155238886)"
   export TWILIO_RECIPIENT_WHATSAPP_NUMBER="whatsapp:tu_numero"
   ```

   - **`TWILIO_ACCOUNT_SID`**: El SID de tu cuenta de Twilio.
   - **`TWILIO_AUTH_TOKEN`**: El token de autenticación de Twilio.
   - **`TWILIO_WHATSAPP_NUMBER`**: El número de WhatsApp de Twilio desde el cual se enviarán las alertas.
   - **`TWILIO_RECIPIENT_WHATSAPP_NUMBER`**: El número de WhatsApp del destinatario (tu número o el número al cual deseas recibir la alerta).

3. Guarda el archivo y luego carga los cambios:

   ```bash
   source ~/.bashrc  # o source ~/.bash_profile
   ```

   Esto cargará las nuevas variables de entorno.


### **Lógica de las alertas**:

El **servidor** gestionará las alertas de la siguiente manera:
1. **Envío de alertas**: El servidor enviará alertas cuando el número de logs de un servicio supere un umbral determinado.
2. **Máximo de 5 alertas por servicio**: Por cada servicio, se pueden enviar hasta 5 alertas. Una vez que se alcanzan las 5 alertas, el umbral se duplicara.
3. **Restablecimiento del umbral**: El servidor continuará monitorizando los servicios. Si el nuevo umbral se supera, se restablece el contador de alertas y se pueden enviar otras 5 alertas antes de que el umbral vuelva a incrementarse.


### **Dependencias**:

- **Twilio SDK**: El servidor usa la API de Twilio para enviar alertas por WhatsApp. Asegúrate de tener configuradas las variables de entorno mencionadas anteriormente.
  
- **Librerías necesarias**: El proyecto utiliza semáforos para sincronización y **`libcurl`** para enviar las alertas a través de la API de Twilio. Asegúrate de tener `libcurl` instalada en tu sistema.

  Para instalar `libcurl` en sistemas basados en Debian/Ubuntu, puedes usar:

  ```bash
  sudo apt-get install libcurl4-openssl-dev
  ```

### **Comandos útiles**:

- **Limpiar los archivos generados**:
  Si deseas limpiar los archivos compilados (ejecutables), puedes usar:

  ```bash
  make clean
  ```
