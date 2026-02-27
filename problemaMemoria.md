# Escenario


En un hospital en la zona del Chaco, hay un pueblo con 500 personas que dependen de un solo hospital.
Para ser eficientes en la atención en las salas de emergencias, cada hora se registran 10 pacientes con mediciones de presión arterial y frecuencia cardíaca.
# Buffer Circular:
Se usaria un algoritmo de estructura de datos llamado tambien cola circular en el que se guarda en memoria y permite almacenar datos de manera ciclica.
## Idea Principal:
Imaginemos que tenemos un arreglo de tamano fijo, por ejemplo memoria[240] para guardar 240 lecturas de un paciente en 1 dia de 24 horas.

# Problema:

El problema surge porque hay solo dos médicos de turno cargando los datos de los pacientes en el sistema, y este debe ser rápido y confiable.

Procedimiento

El sistema almacena temporalmente las lecturas en la memoria RAM del procesador antes de enviarlas a la base de datos central.

Sin embargo, la memoria es limitada: solo puede guardar los datos de las últimas 24 horas (240 registros por paciente, ya que son 10 pacientes × 24 lecturas).
# Que pasaria si el buffer circular esta con los 240 pacientes ya y llenaron la memoria del total. Por ejemplo entra en urgencias una senora que esta por dar a luz y no puede esperar mas.
En estos caso lo que se hace: 
Como la RAM esta limitada a 240 registros, al llegar el paciente numero 241 pasa lo siguiente, el sistema sobrescribe el registro mas antiguo

Si la computadora se apaga, existe un respaldo automático que permite que, al reiniciarse, los datos se vuelvan a cargar desde la copia en la base de datos, para luego sincronizar con la base principal.






