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


# Register a los Profes: 

PROFE_EMAIL=tumail@gmail.com

  La clave se envía solo a ese email. Nadie más la recibe.

  Entonces el flujo es:

  1. Vos recibís la clave en tu mail cada 15 min
  2. Cuando querés que alguien sea profe, le pasás la clave por privado (WhatsApp, en persona, etc.)
  3. Esa persona tiene 15 min para usarla, después ya no sirve

  Un alumno no puede "fingir" ser profe porque:
  - No tiene acceso a tu email
  - La clave cambia cada 15 min
  - Si pone una clave incorrecta le dice "Clave incorrecta o expirada"

  Básicamente vos sos el único que controla quién puede ser profe, porque vos decidís a quién le compartís la clave.


Deshacer lo de clave en register porque lo puedo manejar de mi backend despues para que pueda darle admin o normal user
