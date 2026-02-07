Riesgos reales de la app tal como está:

  Bajo riesgo (para probar entre conocidos):
  - Las contraseñas están hasheadas con bcrypt
  - Las queries usan parámetros (no hay SQL injection)
  - JWT para autenticación

  Lo que falta para producción:
  - No hay rate limiting — alguien podría spamear registros o intentos de login
  - No hay HTTPS propio (ngrok sí lo da)
  - El JWT_SECRET es el default — cualquiera que lea el repo puede fabricar tokens
  - No hay validación de email — pueden registrar emails falsos
  - El rol "profe" lo elige cualquiera al registrarse

  Para una prueba rápida entre compañeros con ngrok es seguro. Nadie va a descubrir tu URL aleatoria
  de ngrok a menos que la compartas.

  Si querés un mínimo de hardening rápido, puedo:
  - Cambiar el JWT_SECRET a uno random
  - Sacar la opción de registrarse como profe desde el frontend (solo vos lo hacés por DB)
  - Agregar rate limiting al login



  Para que otra persona lo pruebe necesitás exponer el puerto. La forma más rápida es con ngrok (túnel
   gratuito):

  # Instalar
  snap install ngrok

  # Crear túnel
  ngrok http 3000

Te da una URL pública tipo https://xxxx.ngrok-free.app que podés compartir. Mientras tengas Docker y ngrok corriendo, cualquiera puede entrar.








- Crear un script con sintaxis de la catedra tambien para verificar por sintaxis para ver si se la a escrito bien, ser super fino en la sintaxis. Por lo tanto el editor de antilez puede servir para terminarlo y colocarlo, para usarlo como editor propio...
- Sistema de niveles, arreglarlo...

